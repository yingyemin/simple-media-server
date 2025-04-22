#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Common/Config.h"
#include "RecordReaderMp4.h"
#include "Logger.h"
#include "Util/String.h"
#include "Mp4/Mp4FileReader.h"
#include "WorkPoller/WorkLoopPool.h"

using namespace std;

RecordReaderMp4::RecordReaderMp4(const string& path)
    :RecordReader(path)
{

}

RecordReaderMp4::~RecordReaderMp4()
{
}

bool RecordReaderMp4::start()
{
    RecordReader::start();

    weak_ptr<RecordReaderMp4> wSelf = dynamic_pointer_cast<RecordReaderMp4>(shared_from_this());
    static string rootPath = Config::instance()->getAndListen([](const json &config){
        rootPath = Config::instance()->get("Record", "rootPath");
    }, "Record", "rootPath");

    string ext= _filePath.substr(_filePath.rfind('.') + 1);
    auto abpath = File::absolutePath(_filePath, rootPath);

    auto demuxer = make_shared<Mp4FileReader>(abpath);
    demuxer->setOnFrame([wSelf](const FrameBuffer::Ptr &frame){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        self->_frameList.push_back(frame);
        // self->_onFrame(frame);
        // self->_lastFrameTime = frame->dts();
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

        while (true) {
            logInfo << "self->_frameList size: " << self->_frameList.size();
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
            task->func_ = [wSelf, demuxer](){
                auto self = wSelf.lock();
                if (!self) {
                    return ;
                }
            //     logInfo << "read mp4";
                if (!demuxer->mov_reader_read2()) {
                    logInfo << "mov_reader_read2 failed, stop";
                    self->stop();
                    return ;
                }
            };
            self->_workLoop->addOrderTask(task);

            return 10;
        }

        auto now = self->_clock.startToNow();

        if (self->_lastFrameTime <= now) {
            return 1;
        } else {
            return int(self->_lastFrameTime - now);
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

