#ifdef ENABLE_MP4

#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Common/Config.h"
#include "RecordReaderMp4.h"
#include "Logger.h"
#include "Util/String.hpp"
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
RecordReaderMp4::RecordReaderMp4(const string& path)
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

RecordReaderMp4::~RecordReaderMp4()
{
    logDebug << "~RecordReaderMp4(): " << _filePath;
}

bool RecordReaderMp4::initMp4()
{
    weak_ptr<RecordReaderMp4> wSelf = dynamic_pointer_cast<RecordReaderMp4>(shared_from_this());
    static string rootPath = Config::instance()->getAndListen([](const json &config){
        rootPath = Config::instance()->get("Record", "rootPath");
    }, "Record", "rootPath");

    auto abpath = File::absolutePath(_filePath, rootPath);

    _mp4Reader = make_shared<Mp4FileReader>(abpath);
    _mp4Reader->setOnFrame([wSelf](const FrameBuffer::Ptr &frame){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        lock_guard<mutex> lck(self->_mtxFrameList);
        self->_frameList.push_back(frame);
        // self->_onFrame(frame);
        // self->_lastFrameTime = frame->dts();
    });
    _mp4Reader->setOnReady([wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        if (self->_onReady) {
            self->_onReady();
        }
    });
    _mp4Reader->setOnTrackInfo([wSelf](const TrackInfo::Ptr &trackInfo){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        trackInfo->duration_ = self->getDuration();
        logInfo << "mp4 reader add track: " << trackInfo->codec_;
        if (self->_onTrackInfo) {
            self->_onTrackInfo(trackInfo);
        }
    });

    if (!_mp4Reader->open()) {
        return false;
    }
    if (!_mp4Reader->init()) {
        return false;
    }

    if (_mp4Reader->mov_reader_getinfo() < 0) {
        return false;
    }

    return true;
}

bool RecordReaderMp4::start()
{
    logInfo << " start reader mp4";
    _clock.update();

    RecordReader::start();
    if (!_mp4Reader) {
        initMp4();
    }

    weak_ptr<RecordReaderMp4> wSelf = dynamic_pointer_cast<RecordReaderMp4>(shared_from_this());
    _loop->addTimerTask(40, [wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return 0;
        }

        if (self->_isPause) {
            return 40;
        }

        // if (self->_lastFrameTime < self->_baseDts/* || self->_lastFrameTime > self->_baseDts + 500*/) {
        //     self->_baseDts = self->_lastFrameTime;
        //     self->_clock.update();

        //     return 1;
        // }
        auto now = self->_clock.startToNow();
        int sleepTime = 40;

        logDebug << "start to read mp4";
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
                     << ", now: " << now;
            uint64_t dtsDiff = 0;
            if (frame->dts() > self->_baseDts) {
                dtsDiff = (frame->dts() - self->_baseDts) / self->_scale;
            }
            if (dtsDiff <= now || frame->dts() > self->_lastFrameTime + 500) {
                if (self->_onFrame) {
                    self->_frameList.pop_front();
                    self->_lastFrameTime = frame->dts();
                    frame->_dts /= self->_scale;
                    frame->_pts /= self->_scale;
                    logDebug << "on frame from mp4";
                    // if (frame->getTrackType() == VideoTrackType) {
                    //     FILE* fp = fopen("test.h266", "ab+");
                    //     fwrite(frame->data(), 1, frame->size(), fp);
                    //     fclose(fp);
                    // }
                    self->_onFrame(frame);
                } else {
                    return 0;
                }
            } else {
                // sleepTime = int(dtsDiff - now);
                break;
            }
        }

        logDebug << "self->_frameList size: " << self->_frameList.size();
        if (self->_frameList.size() < 25 && !self->_finish) {
            for (int i = 0; i < 25; ++i) {
                auto task = make_shared<WorkTask>();
                task->priority_ = 100;
                int curLoopCount = self->_curLoopCount;
                task->func_ = [wSelf, curLoopCount](){
                    auto self = wSelf.lock();
                    if (!self || curLoopCount != self->_curLoopCount) {
                        return ;
                    }

                    auto reader = self->_mp4Reader;
                    if (!reader) {
                        return ;
                    }
                    // logInfo << "read mp4";
                    if (!reader->mov_reader_read2()) {
                        if (++self->_curLoopCount >= self->_loopCount) {
                            logInfo << "mov_reader_read2 failed, stop";
                            self->_finish = true;
                        } else {
                            logInfo << "mov_reader_read2 failed, start another loop";
                            // self->initMp4();
                            self->seek(0);
                        }
                    }
                };
                self->_workLoop->addOrderTask(task);
            }
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

void RecordReaderMp4::release()
{
    logInfo << "RecordReaderMp4::release()";

    _timerTask->quit = true;
    _mp4Reader = nullptr;
    _baseDts = 0;
}

void RecordReaderMp4::stop()
{
    RecordReader::stop();
}

void RecordReaderMp4::close()
{
    RecordReader::close();
}

void RecordReaderMp4::seek(uint64_t timeStamp)
{
    timeStamp += _mp4Reader->getFirstDts();
    weak_ptr<RecordReaderMp4> wSelf = dynamic_pointer_cast<RecordReaderMp4>(shared_from_this());
    auto task = make_shared<WorkTask>();
    task->priority_ = 100;
    task->func_ = [wSelf, timeStamp](){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
    //     logInfo << "read mp4";
        if (self->_mp4Reader->mov_reader_seek((int64_t*)&timeStamp) < 0) {
            logInfo << "mov_reader_seek to " << timeStamp << " failed, stop";
            self->stop();
            return ;
        }

        lock_guard<mutex> lck(self->_mtxFrameList);
        self->_frameList.clear();

        self->_clock.update();
        self->_baseDts = timeStamp;
    };
    _workLoop->addOrderTask(task);
}

void RecordReaderMp4::pause(bool isPause)
{
    _isPause = isPause;
}

void RecordReaderMp4::scale(float scale)
{
    _scale = scale;
    _clock.update();
    _baseDts = _lastFrameTime;
}

uint64_t RecordReaderMp4::getDuration()
{
    if (!_mp4Reader) {
        initMp4();
    }

    return _mp4Reader->mov_reader_getduration();
}

#endif