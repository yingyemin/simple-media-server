#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Common/Config.h"
#include "RecordReader.h"
#include "Logger.h"
#include "Util/String.h"
#include "Mpeg/PsDemuxer.h"
#include "Mpeg/TsDemuxer.h"
#include "Mp4/Mp4FileReader.h"
#include "WorkPoller/WorkLoopPool.h"

using namespace std;

RecordReader::RecordReader(const string& path)
    :_filePath(path)
{

}

RecordReader::~RecordReader()
{
    close();
}

void RecordReader::init()
{
    RecordReaderBase::registerCreateFunc([](const string& path){
        return make_shared<RecordReader>(path);
    });
}

bool RecordReader::start()
{
    weak_ptr<RecordReader> wSelf = shared_from_this();
    _workLoop = WorkLoopPool::instance()->getLoopByCircle();
    _loop = EventLoop::getCurrentLoop();

    static string rootPath = Config::instance()->getAndListen([](const json &config){
        rootPath = Config::instance()->get("Record", "rootPath");
    }, "Record", "rootPath");

    string ext= _filePath.substr(_filePath.rfind('.') + 1);
    auto abpath = File::absolutePath(_filePath, rootPath);
    if (!strcasecmp(ext.data(), "ps") || !strcasecmp(ext.data(), "mpeg")) {
        if (!_file.open(abpath, "rb+")) {
            return false;
        }
        auto demuxer = make_shared<PsDemuxer>();
        demuxer->setOnDecode([wSelf](const FrameBuffer::Ptr &frame){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }
            self->_frameList.push_back(frame);
        });
        demuxer->setOnReady([wSelf](){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }
            if (self->_onReady) {
                self->_onReady();
            }
        });
        demuxer->setOnTrackInfo([wSelf](const TrackInfo::Ptr &trackInfo){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }
            if (self->_onTrackInfo) {
                self->_onTrackInfo(trackInfo);
            }
        });
        _loop->addTimerTask(40, [wSelf, demuxer](){
            auto self = wSelf.lock();
            if (!self) {
                return 0;
            }

            while (true) {
                logInfo << "self->_frameList size: " << self->_frameList.size();
                if (!self->_frameList.empty()) {
                    if (self->_onFrame) {
                        auto frame = self->_frameList.front();
                        self->_frameList.pop_front();
                        self->_onFrame(frame);
                    }
                    break;
                }
                auto task = make_shared<WorkTask>();
                task->priority_ = 100;
                task->func_ = [wSelf, demuxer](){
                    auto self = wSelf.lock();
                    if (!self) {
                        return ;
                    }
                    auto buffer = self->_file.read();
                    if (!buffer) {
                        self->close();
                        return ;
                    }
                    logInfo << "start on ps stream";
                    demuxer->onPsStream(buffer->data(), buffer->size(), 0, 0);
                };
                self->_workLoop->addOrderTask(task);

                return 10;
            }

            if (self->_lastFrameTime == 0) {
                self->_lastFrameTime = TimeClock::now();
                return 40;
            }

            auto now = TimeClock::now();
            auto take = now - self->_lastFrameTime;
            self->_lastFrameTime = now;
            logInfo << "take is: " << take;
            if (take >= 40) {
                return 1;
            } else {
                return 40 - (int)take;
            }
        }, nullptr);
    } else if (!strcasecmp(ext.data(), "mp4")) {
        return readMp4(abpath);
    } else {
        return false;
    }

    return true;
}

bool RecordReader::readMp4(const string& path)
{
    weak_ptr<RecordReader> wSelf = shared_from_this();

    auto demuxer = make_shared<Mp4FileReader>(path);
    demuxer->setOnFrame([wSelf](const FrameBuffer::Ptr &frame){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        // self->_frameList.push_back(frame);
        self->_onFrame(frame);
    });
    demuxer->setOnReady([wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        if (self->_onReady) {
            self->_onReady();
        }
    });
    demuxer->setOnTrackInfo([wSelf](const TrackInfo::Ptr &trackInfo){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        if (self->_onTrackInfo) {
            self->_onTrackInfo(trackInfo);
        }
    });

    if (!demuxer->open()) {
        return false;
    }
    if (!demuxer->init()) {
        return false;
    }

    if (demuxer->mov_reader_getinfo() < 0) {
        return false;
    }

    _loop->addTimerTask(40, [wSelf, demuxer](){
        auto self = wSelf.lock();
        if (!self) {
            return 0;
        }
        logInfo << "start to read mp4";

        // while (true) {
            // logInfo << "self->_frameList size: " << self->_frameList.size();
            // if (!self->_frameList.empty()) {
            //     if (self->_onFrame) {
            //         auto frame = self->_frameList.front();
            //         self->_frameList.pop_front();
            //         self->_onFrame(frame);

            //         // FILE* fp = fopen("testvodmp4.h264", "ab+");
            //         // fwrite(frame->data(), 1, frame->size(), fp);
            //         // fclose(fp);
            //     }
            //     break;
            // }
            // auto task = make_shared<WorkTask>();
            // task->priority_ = 100;
            // task->func_ = [wSelf, demuxer](){
            //     auto self = wSelf.lock();
            //     if (!self) {
            //         return ;
            //     }
            //     logInfo << "read mp4";
                if (!demuxer->mov_reader_read2()) {
                    self->stop();
                    return 0;
                }
            // };
            // self->_workLoop->addOrderTask(task);

        //     return 10;
        // }

        auto now = TimeClock::now();
        if (self->_lastFrameTime == 0) {
            self->_lastFrameTime = now;
            return 40;
        }

        self->_lastFrameTime += 40;

        if (self->_lastFrameTime < now) {
            return 1;
        } else {
            return int(self->_lastFrameTime - now);
        }

        // auto take = now - start;
        // // self->_lastFrameTime = now;
        // logInfo << "take is: " << take;
        // if (take >= 40) {
        //     return 1;
        // } else {
        //     return 40 - (int)take;
        // }
    }, nullptr);

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
