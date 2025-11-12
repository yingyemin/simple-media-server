#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#ifdef _WIN32
#include <direct.h>
#include <io.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "Common/Config.h"
#include "RecordReader.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "WorkPoller/WorkLoopPool.h"
#include "RecordReaderMp4.h"
#include "RecordReaderPs.h"
#include "RecordReaderDir.h"
#include "RecordReaderRecord.h"
#include "RecordReaderFlv.h"

mutex RecordReader::_mtxRecordMenu;
RecordReader::RecordMenuMap RecordReader::_recordMenu;

using namespace std;

RecordReader::RecordReader(const string& path)
{

}

RecordReader::~RecordReader()
{
    close();
}

void RecordReader::init()
{
    RecordReaderBase::registerCreateFunc([](const string& path) -> RecordReaderBase::Ptr {
        if (startWith(path, "/dir")) {
            return make_shared<RecordReaderDir>(path);
        } else if (startWith(path, "/record")) {
            return make_shared<RecordReaderRecord>(path);
        }
        
        string ext= path.substr(path.rfind('.') + 1);
        ext = ext.substr(0, ext.find_first_of("/"));
#ifdef ENABLE_MPEG
        if (!strcasecmp(ext.data(), "ps") || !strcasecmp(ext.data(), "mpeg")) {
            return make_shared<RecordReaderPs>(path);
        }
#endif
#ifdef ENABLE_MP4
        if (!strcasecmp(ext.data(), "mp4")) {
            return make_shared<RecordReaderMp4>(path);
        }
#endif
#ifdef ENABLE_RTMP
        if (!strcasecmp(ext.data(), "flv")) {
            return make_shared<RecordReaderFlv>(path);
        }
#endif
        return nullptr;
    });
}

bool RecordReader::start()
{
    weak_ptr<RecordReader> wSelf = shared_from_this();
    _workLoop = WorkLoopPool::instance()->getLoopByCircle();
    _loop = EventLoop::getCurrentLoop();
    _clock.start();

    return true;
}

void RecordReader::stop()
{
    close();
}

void RecordReader::close()
{
    if (_onClose) {
        _onClose();
    }
    _onClose = nullptr;
    _onTrackInfo = nullptr;
    _onReady = nullptr;
    _onFrame = nullptr;
}

void RecordReader::setOnTrackInfo(const function<void(const TrackInfo::Ptr& trackInfo)>& cb)
{
    _onTrackInfo = cb;
}

void RecordReader::setOnReady(const function<void()>& cb)
{
    _onReady = cb;
}

void RecordReader::setOnClose(const function<void()>& cb)
{
    _onClose = cb;
}

void RecordReader::setOnFrame(const function<void(const FrameBuffer::Ptr& frame)>& cb)
{
    _onFrame = cb;
}

static void traverse_directory(const std::string& path, RecordReader::RecordMenuMap& file_map) {
    static string recordPath = Config::instance()->getAndListen([](const string& value){
        recordPath = Config::instance()->get("Record", "rootPath", "", "", "./");
    }, "Record", "rootPath", "", "", "./");

#ifdef _WIN32
    struct _finddata_t file_info;
    intptr_t handle = _findfirst((path + "/*").c_str(), &file_info);
    if (handle == -1) return;
    
    do {
        if (strcmp(file_info.name, ".") == 0 || strcmp(file_info.name, "..") == 0) 
            continue;
            
        std::string full_path = path + "/" + file_info.name;
        if (file_info.attrib & _A_SUBDIR) {
            traverse_directory(full_path, file_map);
        } else {
            std::vector<std::string> parts;
            size_t pos = 0;
            std::string temp = full_path;
            while ((pos = temp.find('/')) != std::string::npos) {
                parts.push_back(temp.substr(0, pos));
                temp.erase(0, pos + 1);
            }
            parts.push_back(temp);
            
            if (parts.size() >= 4) {
                std::string filename = parts.back();
                parts.pop_back();
                std::string day = parts.back();
                parts.pop_back();
                std::string month = parts.back();
                parts.pop_back();
                std::string year = parts.back();
                parts.pop_back();
                
                std::string base_path;
                for (const auto& part : parts) {
                    base_path += part + "/";
                }
                if (!base_path.empty()) {
                    base_path.pop_back();
                }
                
                file_map[base_path][year][month][day].push_back(filename);
            }
        }
    } while (_findnext(handle, &file_info) == 0);
    _findclose(handle);
#else
    DIR* dir = opendir(path.c_str());
    if (!dir) return;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
            
        std::string full_path = path + "/" + entry->d_name;
        struct stat stat_buf;
        if (stat(full_path.c_str(), &stat_buf) != 0) continue;
        
        if (S_ISDIR(stat_buf.st_mode)) {
            traverse_directory(full_path, file_map);
        } else {
            std::vector<std::string> parts;
            size_t pos = 0;
            std::string temp = full_path;
            while ((pos = temp.find('/')) != std::string::npos) {
                parts.push_back(temp.substr(0, pos));
                temp.erase(0, pos + 1);
            }
            parts.push_back(temp);
            
            if (parts.size() >= 4) {
                std::string filename = parts.back();
                parts.pop_back();
                std::string day = parts.back();
                parts.pop_back();
                std::string month = parts.back();
                parts.pop_back();
                std::string year = parts.back();
                parts.pop_back();
                
                std::string base_path;
                for (const auto& part : parts) {
                    base_path += part + "/";
                }
                if (!base_path.empty()) {
                    base_path.pop_back();
                }

                base_path = base_path.substr(recordPath.size());
                
                file_map[base_path][year][month][day].push_back(filename);
            }
        }
    }
    closedir(dir);
#endif
}

void RecordReader::loadRecordMenu()
{
    lock_guard<mutex> lck(_mtxRecordMenu);

    _recordMenu.clear();

    static string recordPath = Config::instance()->getAndListen([](const string& value){
        recordPath = Config::instance()->get("Record", "rootPath", "", "", "./");
    }, "Record", "rootPath", "", "", "./");

    traverse_directory(recordPath, _recordMenu);
}

RecordReader::RecordMenuMap RecordReader::getRecordList(const string& path, const string& year, 
                                            const string& month, const string& day)
{
    lock_guard<mutex> lck(_mtxRecordMenu);

    RecordMenuMap recordList;
    if (_recordMenu.find(path) == _recordMenu.end()) {
        return _recordMenu;
    }
    recordList[path] = _recordMenu[path];
    if (year != "") {
        if (recordList[path].find(year) == recordList[path].end()) {
            return recordList;
        }
        recordList[path][year] = _recordMenu[path][year];
    }
    if (month != "") {
        if (recordList[path][year].find(month) == recordList[path][year].end()) {
            return recordList;
        }
        recordList[path][year][month] = _recordMenu[path][year][month];
    }
    if (day != "") {
        if (recordList[path][year][month].find(day) == recordList[path][year][month].end()) {
            return recordList;
        }
        recordList[path][year][month][day] = _recordMenu[path][year][month][day];
    }
    return recordList;
}

void RecordReader::addRecordInfo(const string& path, const string& year, const string& month, const string& day, const string& fileName)
{
    lock_guard<mutex> lck(_mtxRecordMenu);

    // if (_recordMenu.find(path) == _recordMenu.end()) {
    //     _recordMenu[path];
    // }
    // if (_recordMenu[path].find(year) == _recordMenu[path].end()) {
    //     _recordMenu[path][year];
    // }
    // if (_recordMenu[path][year].find(month) == _recordMenu[path][year].end()) {
    //     _recordMenu[path][year][month];
    // }
    // if (_recordMenu[path][year][month].find(day) == _recordMenu[path][year][month].end()) {
    //     _recordMenu[path][year][month][day];
    // }
    _recordMenu[path][year][month][day].push_back(fileName);
}

void RecordReader::delRecordInfo(const string& path, const string& year, const string& month, const string& day, const string& fileName)
{
    lock_guard<mutex> lck(_mtxRecordMenu);

    if (_recordMenu.find(path) == _recordMenu.end()) {
        return;
    }
    if (_recordMenu[path].find(year) == _recordMenu[path].end()) {
        return;
    }
    if (_recordMenu[path][year].find(month) == _recordMenu[path][year].end()) {
        return;
    }
    if (_recordMenu[path][year][month].find(day) == _recordMenu[path][year][month].end()) {
        return;
    }
    auto& vec = _recordMenu[path][year][month][day];
    vec.erase(remove(vec.begin(), vec.end(), fileName), vec.end());
}
