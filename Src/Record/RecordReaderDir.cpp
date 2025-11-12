#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Common/Config.h"
#include "RecordReaderDir.h"
#include "Logger.h"
#if defined(_WIN32)
#include "Util/String.hpp"
#else
#include "Util/String.hpp"
#endif
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
RecordReaderDir::RecordReaderDir(const string& path)
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
    // 获取点播文件的路径
    _filePath = tmpPath.substr(0, pos);
    // 获取循环次数
    _loopCount = stoi(tmpPath.substr(pos + 1));
}

RecordReaderDir::~RecordReaderDir()
{
}

bool RecordReaderDir::start()
{
    logDebug << "start reader dir";
    RecordReader::start();

    bool first = true;
    weak_ptr<RecordReaderDir> wSelf = dynamic_pointer_cast<RecordReaderDir>(shared_from_this());
    File::scanDir(_filePath, [this, wSelf, &first](const string& path, bool isDir) {
        if (isDir) {
            return false;
        }

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
            // logInfo << "on frame vod dir";
            FrameBuffer::Ptr frameOut = frame;
            frameOut->_index = frameOut->_trackType;
            auto self = wSelf.lock();
            if (self && self->_onFrame) {
                self->_onFrame(frameOut);
            }
        });

        if (first) {
            first = false;
            reader->setOnTrackInfo([wSelf](const TrackInfo::Ptr &trackInfo){
                auto self = wSelf.lock();
                if (self && self->_onTrackInfo) {
                    TrackInfo::Ptr trackInfoOut = trackInfo;
                    trackInfoOut->index_ = trackInfoOut->trackType_ == "video" ? VideoTrackType : AudioTrackType;
                    self->_onTrackInfo(trackInfo);
                }
            });
        }

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
        _duration += reader->getDuration();
        
        return true;
    });

    if (_vecDemuxer.size() == 0) {
        logWarn << "_vecDemuxer is empty";
        return false;
    }

    _reader = _vecDemuxer[_dirIndex++];
    if (!_reader) {
        logWarn << "reader is empty";
        return false;
    }

    logDebug << "start demuxer reader start";
    return _reader->start();
}


void RecordReaderDir::stop()
{
    RecordReader::stop();
}

void RecordReaderDir::close()
{
    RecordReader::close();
}

void RecordReaderDir::seek(uint64_t timeStamp)
{
    logDebug << "seek dir reader";
    int index = 0;
    for (auto &reader : _vecDemuxer) {
        auto duration = reader->getDuration();
        if (timeStamp < duration) {
            if (_reader != reader) {
                logInfo << "release reader";
                _reader->release();
                _reader = reader;
                _reader->start();
            }
            reader->seek(timeStamp);
            break;
        }
        ++index;

        timeStamp -= duration;
    }

    _dirIndex = index + 1;
}

void RecordReaderDir::pause(bool isPause)
{
    if (_reader) {
        _reader->pause(isPause);
    }
}

void RecordReaderDir::scale(float scale)
{
    if (_reader) {
        _reader->scale(scale);
    }
}

uint64_t RecordReaderDir::getDuration()
{
    return _duration;
}