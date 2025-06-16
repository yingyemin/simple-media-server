#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Common/Config.h"
#include "RecordReaderRecord.h"
#include "Logger.h"
#include "Util/String.h"
#include "WorkPoller/WorkLoopPool.h"
#include "RecordReaderMp4.h"
#include "RecordReaderPs.h"

using namespace std;

// 本地文件点播的url格式 rtmp://127.0.0.1/file/vodId/live/test.mp4/3
// 录像回放的url格式 rtmp://127.0.0.1/record/vodId/live/test/{starttime}/{endtime}
// 云端录像回放的url格式 rtmp://127.0.0.1/cloud/vodId/live/test/{starttime}/{endtime}，云端录像回放需要向管理服务拿一下云端地址
// 目录点播 rtmp://127.0.0.1/dir/vodId/live/test/3

// 一级路径 file/record/cloud/dir表示点播类型。
// 二级目录 vodId唯一标识，业务层控制点播url的唯一性
// 最后一级 3，表示循环次数，0表示无限循环
// 录像回放暂不设置循环参数
RecordReaderRecord::RecordReaderRecord(const string& path)
    :RecordReader(path)
{
    // 去掉第一层目录
    auto tmpPath = path.substr(path.find_first_of("/", 1) + 1);
    _recordType = path.substr(0, path.find_first_of("/", 1));
    // 去掉第二层目录
    _vodId = tmpPath.substr(0, tmpPath.find_first_of("/", 1));
    tmpPath = tmpPath.substr(tmpPath.find_first_of("/", 1) + 1);
    // 找到最后一层目录的位置
    int pos = tmpPath.find_last_of("/");
    // 获取循环次数
    _endTime = stoull(tmpPath.substr(pos + 1));
    tmpPath = tmpPath.substr(0, pos);
    // 找到最后第二层目录的位置
    pos = tmpPath.find_last_of("/");
    _startTime = stoull(tmpPath.substr(pos + 1));
    // 获取点播文件的路径
    _filePath = tmpPath.substr(0, pos);
}

RecordReaderRecord::~RecordReaderRecord()
{
}

bool RecordReaderRecord::start()
{
    logDebug << "start reader dir";
    RecordReader::start();

    static string rootPath = Config::instance()->getAndListen([](const json &config){
        rootPath = Config::instance()->get("Record", "rootPath");
    }, "Record", "rootPath");

    auto startTm = TimeClock::localtime(_startTime);
    auto endTm = TimeClock::localtime(_endTime);
    vector<string> dirList;

    for (auto startY = startTm.tm_year; startY <= endTm.tm_year; ++startY) {
        string pathY = rootPath + _filePath + "/" + to_string(startY);
        for (auto startM = startTm.tm_mon; startM <= endTm.tm_mon; ++startM) {
            string pathM = pathY + "/" + to_string(startM);
            for (auto startD = startTm.tm_mday; startD <= endTm.tm_mday; ++startD) {
                string pathD = pathM + "/" + to_string(startD);
                dirList.push_back(pathD);
            }
        }
    }

    weak_ptr<RecordReaderRecord> wSelf = dynamic_pointer_cast<RecordReaderRecord>(shared_from_this());

    for (auto& dir : dirList) {
        File::scanDir(dir, [this, wSelf](const string& path, bool isDir) {
            if (isDir) {
                return false;
            }

            std::string filename = Path::getFileName(path);
            uint64_t fileStartTime = stoull(filename);

            RecordReader::Ptr reader;
            string filePath = _recordType + "/" + _vodId + "/" + path + "/" + to_string(_loopCount);
            logInfo << "create reader: " << filePath;
            if (filePath.find(".mp4") != string::npos) { 
                reader = make_shared<RecordReaderMp4>(filePath);
            } else if (filePath.find(".ps") != string::npos) {
                reader = make_shared<RecordReaderPs>(filePath);
            }

            if (!reader) {
                return false;
            }

            reader->setOnFrame([wSelf](const FrameBuffer::Ptr &frame){
                logTrace << "on frame vod record";
                FrameBuffer::Ptr frameOut = frame;
                frameOut->_index = frameOut->_trackType;
                auto self = wSelf.lock();
                if (self && self->_onFrame) {
                    self->_onFrame(frameOut);
                }
            });

            // reader->setOnTrackInfo([wSelf](const TrackInfo::Ptr &trackInfo){
            //     auto self = wSelf.lock();
            //     if (self && self->_onTrackInfo) {
            //         TrackInfo::Ptr trackInfoOut = trackInfo;
            //         trackInfoOut->index_ = trackInfoOut->trackType_ == "video" ? VideoTrackType : AudioTrackType;
            //         self->_onTrackInfo(trackInfo);
            //     }
            // });

            reader->setOnClose([wSelf](){
                auto self = wSelf.lock();
                if (!self) {
                    return ;
                }
                self->_loop->async([wSelf](){
                    auto self = wSelf.lock();
                    if (!self) {
                        return ;
                    }
                    logInfo << "reader dir on close";
                    logInfo << "self->_dirIndex: " << self->_dirIndex << ", self->_vecDemuxer.size()" << self->_vecDemuxer.size();

                    if (self->_dirIndex >= self->_vecDemuxer.size()) {
                        logInfo << "self->_dirIndex: " << self->_dirIndex << ", self->_loopCount: " << self->_loopCount;
                        if (++self->_dirLoopCount < self->_loopCount) {
                            self->_dirIndex = 0;
                            self->_reader = self->_vecDemuxer[self->_dirIndex++];
                            self->_reader->start();

                            return ;
                        } else if (self->_onClose) {
                            self->_onClose();
                        }
                    } else {
                        self->_reader = self->_vecDemuxer[self->_dirIndex++];
                        self->_reader->start();
                    }
                }, true);
            });

            _vecDemuxer.emplace_back(reader);
            _vecFileStartTime.emplace_back(fileStartTime);
            
            return true;
        });
    }

    if (_vecDemuxer.size() == 0) {
        logWarn << "_vecDemuxer is empty";
        return false;
    }

    for (auto &reader : _vecDemuxer) {
        if (_startTime < _vecFileStartTime[_dirIndex]) {
            if (_dirIndex > 0) {
                _dirIndex -= 1;
            }
            break;
        }
        ++_dirIndex;
    }

    _reader = _vecDemuxer[_dirIndex++];
    if (!_reader) {
        logWarn << "reader is empty";
        return false;
    }

    _reader->setOnTrackInfo([wSelf](const TrackInfo::Ptr &trackInfo){
        logInfo << "add track: " << trackInfo->codec_;
        auto self = wSelf.lock();
        if (self && self->_onTrackInfo) {
            TrackInfo::Ptr trackInfoOut = trackInfo;
            trackInfoOut->index_ = trackInfoOut->trackType_ == "video" ? VideoTrackType : AudioTrackType;
            self->_onTrackInfo(trackInfo);
        }
    });

    logDebug << "start demuxer reader start";
    if (!_reader->start()) {
        return false;
    }

    logInfo << "startTime: " << _startTime;
    logInfo << "file startTime: " << _vecFileStartTime[_dirIndex - 1];
    _reader->seek(_startTime - _vecFileStartTime[_dirIndex - 1]);

    return true;
}


void RecordReaderRecord::stop()
{
    RecordReader::stop();
}

void RecordReaderRecord::close()
{
    RecordReader::close();
}

void RecordReaderRecord::seek(uint64_t timeStamp)
{
    logDebug << "seek record reader";
    int index = 0;
    for (auto &reader : _vecDemuxer) {
        if (timeStamp < _vecFileStartTime[index]) {
            if (index > 0) {
                index -= 1;
            }
            if (_reader != reader) {
                logInfo << "release reader";
                _reader->release();
                _reader = reader;
                _reader->start();
            }
            reader->seek(timeStamp - _vecFileStartTime[index]);
            break;
        }
        ++index;
    }

    _dirIndex = index + 1;
}

void RecordReaderRecord::pause(bool isPause)
{
    if (_reader) {
        _reader->pause(isPause);
    }
}

void RecordReaderRecord::scale(float scale)
{
    if (_reader) {
        _reader->scale(scale);
    }
}

uint64_t RecordReaderRecord::getDuration()
{
    return _endTime - _startTime;
}