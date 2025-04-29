#ifdef ENABLE_MP4

#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Common/Config.h"
#include "RecordReaderMp4.h"
#include "Logger.h"
#include "Util/String.h"
#include "WorkPoller/WorkLoopPool.h"

using namespace std;

// 本地文件点播的url格式 rtmp://127.0.0.1/file/live/test.mp4/3
// 录像回放的url格式 rtmp://127.0.0.1/record/live/test/{starttime}/{endtime}
// 云端录像回放的url格式 rtmp://127.0.0.1/cloud/live/test/{starttime}/{endtime}，云端录像回放需要向管理服务拿一下云端地址
// 目录点播 rtmp://127.0.0.1/dir/live/test/3

// 一级路径 file/record/cloud/dir表示点播类型。
// 最后一级 3，表示循环次数，0表示无限循环
// 录像回放暂不设置循环参数
RecordReaderMp4::RecordReaderMp4(const string& path)
    :RecordReader(path)
{
    auto tmpPath = path.substr(path.find_first_of("/", 1) + 1);
    int pos = tmpPath.find_last_of("/");
    _filePath = tmpPath.substr(0, pos);
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
    RecordReader::start();
    initMp4();

    weak_ptr<RecordReaderMp4> wSelf = dynamic_pointer_cast<RecordReaderMp4>(shared_from_this());
    _loop->addTimerTask(40, [wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return 0;
        }

        if (self->_isPause) {
            return 40;
        }

        logDebug << "start to read mp4";

        while (true) {
            logDebug << "self->_frameList size: " << self->_frameList.size();
            if (!self->_frameList.empty()) {
                if (self->_onFrame) {
                    auto frame = self->_frameList.front();
                    self->_frameList.pop_front();
                    self->_onFrame(frame);
                    self->_lastFrameTime = frame->dts();

                    // FILE* fp = fopen("testvodmp4.h264", "ab+");
                    // fwrite(frame->data(), 1, frame->size(), fp);
                    // fclose(fp);
                }
                break;
            }
            auto task = make_shared<WorkTask>();
            task->priority_ = 100;
            task->func_ = [wSelf](){
                auto self = wSelf.lock();
                if (!self) {
                    return ;
                }
            //     logInfo << "read mp4";
                if (!self->_mp4Reader->mov_reader_read2()) {
                    if (++self->_curLoopCount >= self->_loopCount) {
                        logInfo << "mov_reader_read2 failed, stop";
                        self->stop();
                    } else {
                        logInfo << "mov_reader_read2 failed, start another loop";
                        self->initMp4();
                    }
                }
            };
            self->_workLoop->addOrderTask(task);

            return 10;
        }

        auto now = self->_clock.startToNow();
        if (self->_lastFrameTime < self->_baseDts || self->_lastFrameTime > self->_baseDts + 500) {
            self->_baseDts = self->_lastFrameTime;
            self->_clock.update();

            return 10;
        }

        auto dtsDiff = (self->_lastFrameTime - self->_baseDts) / self->_scale;

        if (dtsDiff <= now) {
            return 1;
        } else {
            return int(dtsDiff - now);
        }
    }, nullptr);

    return true;
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
    if (_mp4Reader) {
        return _mp4Reader->mov_reader_getduration();
    }

    return 0;
}

#endif