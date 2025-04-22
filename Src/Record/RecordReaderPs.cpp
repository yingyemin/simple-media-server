#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Common/Config.h"
#include "RecordReaderPs.h"
#include "Logger.h"
#include "Util/String.h"
#include "Mpeg/PsDemuxer.h"
#include "WorkPoller/WorkLoopPool.h"

using namespace std;

RecordReaderPs::RecordReaderPs(const string& path)
    :RecordReader(path)
{

}

RecordReaderPs::~RecordReaderPs()
{
}

bool RecordReaderPs::start()
{
    RecordReader::start();

    weak_ptr<RecordReaderPs> wSelf = dynamic_pointer_cast<RecordReaderPs>(shared_from_this());
    static string rootPath = Config::instance()->getAndListen([](const json &config){
        rootPath = Config::instance()->get("Record", "rootPath");
    }, "Record", "rootPath");

    string ext= _filePath.substr(_filePath.rfind('.') + 1);
    auto abpath = File::absolutePath(_filePath, rootPath);

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
                    self->_lastFrameTime = frame->dts();
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

            return 40;
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


void RecordReaderPs::stop()
{
    RecordReader::stop();
}

void RecordReaderPs::close()
{
    RecordReader::close();
}

