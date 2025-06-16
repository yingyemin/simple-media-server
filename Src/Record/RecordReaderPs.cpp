#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Common/Config.h"
#include "RecordReaderPs.h"
#include "Logger.h"
#include "Util/String.h"
#include "WorkPoller/WorkLoopPool.h"

using namespace std;

// 本地文件点播的url格式 rtmp://127.0.0.1/file/vodId/live/test.mp4/3
// 录像回放的url格式 rtmp://127.0.0.1/record/vodId/live/test/{starttime}/{endtime}
// 云端录像回放的url格式 rtmp://127.0.0.1/cloud/vodId/live/test/{starttime}/{endtime}，云端录像回放需要向管理服务拿一下云端地址
// 目录点播 rtmp://127.0.0.1/dir/vodId/live/test/3

// 一级路径 file/record/cloud/dir表示点播类型。
// 二级目录 vodId唯一标识，业务层控制点播url的唯一性
// 最后一级 3，表示循环次数，0表示无限循环
// 录像回放暂不设置循环参数
RecordReaderPs::RecordReaderPs(const string& path)
    :RecordReader(path)
{
    // 去掉第一层目录
    auto tmpPath = path.substr(path.find_first_of("/", 1) + 1);
    // 去掉第二层目录
    tmpPath = tmpPath.substr(tmpPath.find_first_of("/", 1) + 1);
    // 找到最后一层目录的位置
    int pos = tmpPath.find_last_of("/");
    // 获取点播文件的路径
    _filePath = tmpPath.substr(0, pos);
    // 获取循环次数
    _loopCount = stoi(tmpPath.substr(pos + 1));
}

RecordReaderPs::~RecordReaderPs()
{
}

bool RecordReaderPs::start()
{
    logInfo << "start ps reader";
    _clock.update();

    RecordReader::start();
    if (!_demuxer) {
        initPs();
    }

    if (!_duration) {
        getDurationFromFile();
    }

    weak_ptr<RecordReaderPs> wSelf = dynamic_pointer_cast<RecordReaderPs>(shared_from_this());
    _loop->addTimerTask(40, [wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return 0;
        }

        if (self->_isPause) {
            return 40;
        }

        auto now = self->_clock.startToNow();
        int sleepTime = 40;

        logTrace << "start to read ps";
        lock_guard<mutex> lck(self->_mtxFrameList);

        logTrace << "self->_frameList size: " << self->_frameList.size();
        while (!self->_frameList.empty()) {
            if (self->_isPause) {
                return 40;
            }
            auto frame = self->_frameList.front();
            // if (self->_lastFrameTime < self->_baseDts || frame->dts() > self->_lastFrameTime + 500) {
            //     self->_baseDts = self->_lastFrameTime;
            //     self->_lastFrameTime = frame->dts();
            //     self->_clock.update();

            //     return 1;
            // }
            logTrace << "frame->dts(): " << frame->dts() << ", self->_baseDts: " << self->_baseDts
                     << ", now: " << now << ", type: " << frame->getTrackType() 
                     << ", index: " << frame->getTrackIndex();
            uint64_t dtsDiff = 0;
            if (frame->dts() < self->_baseDts) {
                self->_frameList.pop_front();
                continue;
            }
            dtsDiff = (frame->dts() - self->_baseDts) / self->_scale;
            if (dtsDiff <= now || frame->dts() > self->_lastFrameTime + 500) {
                if (self->_onFrame) {
                    self->_frameList.pop_front();
                    self->_lastFrameTime = frame->dts();
                    frame->_dts /= self->_scale;
                    frame->_pts /= self->_scale;
                    self->_onFrame(frame);
                }
            } else {
                // sleepTime = int(dtsDiff - now);
                break;
            }
        }

        logDebug << "self->_frameList size: " << self->_frameList.size();
        if (self->_frameList.size() < 25 && !self->_isReading) {
            // for (int i = 0; i < 25; ++i) {
                auto task = make_shared<WorkTask>();
                task->priority_ = 100;
                task->func_ = [wSelf](){
                    auto self = wSelf.lock();
                    if (!self) {
                        return ;
                    }
                    auto reader = self->_demuxer;
                    if (!reader) {
                        return ;
                    }

                    reader->onPsStream(nullptr, 0, 0, 0);

                    auto buffer = self->_file.read();
                    if (!buffer) {
                        // self->close();
                        if (++self->_curLoopCount >= self->_loopCount) {
                            logInfo << "ps file read failed, stop";
                            self->_finish = true;
                            return ;
                        } else {
                            logInfo << "ps file read, start another loop";
                            self->_file.seek(0);
                        }
                    }
                    logDebug << "start on ps stream";
                    reader->onPsStream(buffer->data(), buffer->size(), 0, 0);
                    self->_isReading = false;
                };
                self->_workLoop->addOrderTask(task);
                self->_isReading = true;
            // }
        }

        if (self->_finish && self->_frameList.empty()) {
            self->stop();
            return 0;
        }

        return sleepTime > 0 ? sleepTime : 1;
    }, [wSelf](bool success, const TimerTask::Ptr& task){
        auto self = wSelf.lock();
        if (self) {
            self->_timerTask = task;
        }
    });

    return true;
}

void RecordReaderPs::release()
{
    _timerTask->quit = true;
    _demuxer->clear();
    _file.seek(0);
    _baseDts = 0;
}

void RecordReaderPs::stop()
{
    RecordReader::stop();
}

void RecordReaderPs::close()
{
    RecordReader::close();
}

void RecordReaderPs::seek(uint64_t timeStamp)
{
    logInfo << "seek ps: " << timeStamp;
    logInfo << "_firstDts: " << _firstDts;
    timeStamp += _firstDts;

    weak_ptr<RecordReaderPs> wSelf = dynamic_pointer_cast<RecordReaderPs>(shared_from_this());
    auto task = make_shared<WorkTask>();
    task->priority_ = 100;
    task->func_ = [wSelf, timeStamp](){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        self->_file.seek(0);
        self->_demuxer->clear();

        lock_guard<mutex> lck(self->_mtxFrameList);
        self->_frameList.clear();

        self->_clock.update();
        self->_baseDts = timeStamp;

        while (true) {
            auto buffer = self->_file.read();
            if (!buffer) {
                self->close();
                return ;
            }
            logDebug << "start seek ps stream";
            if (self->_demuxer->seek(buffer->data(), buffer->size(), timeStamp, 0) == 0) {
                break;
            }
        }
        logDebug << "seek ps stream success";
    };
    _workLoop->addOrderTask(task);
}

void RecordReaderPs::pause(bool isPause)
{
    _isPause = isPause;
}

void RecordReaderPs::scale(float scale)
{
    _scale = scale;
    _clock.update();
    _baseDts = _lastFrameTime;
}

uint64_t RecordReaderPs::getDuration()
{
    logInfo << "get duration";
    if (_duration == 0) {
        if (!_demuxer) {
            initPs();
        }
        getDurationFromFile();
    }
    return _duration;
}

void RecordReaderPs::initPs()
{
    weak_ptr<RecordReaderPs> wSelf = dynamic_pointer_cast<RecordReaderPs>(shared_from_this());
    static string rootPath = Config::instance()->getAndListen([](const json &config){
        rootPath = Config::instance()->get("Record", "rootPath");
    }, "Record", "rootPath");

    string ext= _filePath.substr(_filePath.rfind('.') + 1);
    auto abpath = File::absolutePath(_filePath, rootPath);

    if (!_file.open(abpath, "rb+")) {
        return ;
    }
    _file.seek(0);

    _demuxer = make_shared<PsDemuxer>();
    _demuxer->setOnDecode([wSelf](const FrameBuffer::Ptr &frame){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        if (self->_state == 1) {
            self->_firstDts = frame->dts();
            self->_state = 3;
            logDebug << "self->_firstDts: " << self->_firstDts;
            return ;
        } else if (self->_state == 2) {
            self->_duration = frame->dts() - self->_firstDts;
            return ;
        }
        lock_guard<mutex> lck(self->_mtxFrameList);
        self->_frameList.push_back(frame);
    });
    _demuxer->setOnReady([wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        if (self->_onReady) {
            self->_onReady();
        }
    });
    _demuxer->setOnTrackInfo([wSelf](const TrackInfo::Ptr &trackInfo){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        
        trackInfo->duration_ = self->_duration;
        if (self->_onTrackInfo) {
            self->_onTrackInfo(trackInfo);
        }
    });
}

void RecordReaderPs::getDurationFromFile()
{
    _state = 1;
    auto buffer = _file.read();
    if (!buffer) {
        close();
        return ;
    }
    logInfo << "seek ps stream start";
    _demuxer->onPsStream(buffer->data(), buffer->size(), 0, 0);
    _demuxer->clear();

    _state = 2;
    if (_file.getFileSize() > 1024 * 1024) {
        _file.seek(_file.getFileSize() - 1024 * 1024);
        auto buffer = _file.read();
        if (!buffer) {
            close();
            return ;
        }
        logInfo << "seek ps stream end";
        _demuxer->onPsStream(buffer->data(), buffer->size(), 0, 0);
    }

    _state = 0;
    _file.seek(0);
    _demuxer->clear();
}