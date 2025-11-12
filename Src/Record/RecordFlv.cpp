#ifdef ENABLE_FLV
#include "RecordFlv.h"
#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>

#include "WorkPoller/WorkLoopPool.h"
#include "Common/Config.h"
#include "Util/String.hpp"

RecordFlv::RecordFlv(const UrlParser& urlParser, const RecordTemplate::Ptr& recordTemplate)
    :_template(recordTemplate)
{
    _urlParser = urlParser;
}

RecordFlv::~RecordFlv()
{
    auto frameSrc = _source.lock();
    if (frameSrc) {
        frameSrc->delConnection(this);
    }
}

bool RecordFlv::start()
{
    _workLoop = WorkLoopPool::instance()->getLoopByCircle();
    _loop = EventLoop::getCurrentLoop();

    static string rootPath = Config::instance()->getAndListen([](const json& config) {
        rootPath = Config::instance()->get("Record", "rootPath");
        }, "Record", "rootPath");

    auto now = time(nullptr);
    auto nowTm = TimeClock::localtime(now);
    auto abpath = File::absolutePath(_urlParser.path_, rootPath) + "/"
        + to_string(nowTm.tm_year) + "/" + to_string(nowTm.tm_mon)
        + "/" + to_string(nowTm.tm_mday) + "/" + to_string(time(nullptr)) + ".flv";
    logInfo << "get record path: " << abpath;
    // if (!_file.open(abpath, "wb+")) {
    //     return false;
    // }

    _flvWriter = make_shared<FlvFileWriter>(abpath);
    if (!_flvWriter->open()) {
        return false;
    }
    
    _recordInfo.uri = _urlParser.path_;
    _recordInfo.status = "on";
    _recordInfo.startTime = time(nullptr);
    _recordInfo.filePath = abpath;
    _recordInfo.fileName = _urlParser.path_ + "/"
        + to_string(nowTm.tm_year) + "/" + to_string(nowTm.tm_mon)
        + "/" + to_string(nowTm.tm_mday) + "/" + to_string(time(nullptr)) + ".flv";

    auto hook = HookManager::instance()->getHook("MediaHook");
    // if (hook) {
    //     hook->onRecord(_recordInfo);
    // }

    weak_ptr<RecordFlv> wSelf = dynamic_pointer_cast<RecordFlv>(shared_from_this());
    MediaSource::getOrCreateAsync(_urlParser.path_, _urlParser.vhost_, _urlParser.protocol_, _urlParser.type_,
        [wSelf](const MediaSource::Ptr& src) {
            logInfo << "get a src";
            auto self = wSelf.lock();
            if (!self) {
                return;
            }

            auto frameSrc = dynamic_pointer_cast<RtmpMediaSource>(src);
            if (!frameSrc) {
                self->onError("hls source is not exist");
                return;
            }

            // self->_source = rtmpSrc;

            self->_loop->async([wSelf, frameSrc]() {
                auto self = wSelf.lock();
                if (!self) {
                    return;
                }

                self->onPlayFrame(frameSrc);
                }, true);
        },
        [wSelf]() -> MediaSource::Ptr {
            auto self = wSelf.lock();
            if (!self) {
                return nullptr;
            }
            auto source = make_shared<RtmpMediaSource>(self->_urlParser, nullptr, true);
            if (self->_urlParser.type_ == "enhanced") {
                source->setEnhanced(true);
            } else if (self->_urlParser.type_ == "fastPts") {
                source->setFastPts(true);
            } else {
                return make_shared<RtmpMediaSource>(self->_urlParser, nullptr,true);
            }

            return source;
        }, this);

    return true;
}
void RecordFlv::onPlayFrame(const RtmpMediaSource::Ptr& frameSrc)
{
    _clock.start();
    weak_ptr<RecordFlv> wSelf = dynamic_pointer_cast<RecordFlv>(shared_from_this());
    // дflv�ļ�ͷ
    _flvWriter->onWriteFlvHeader(frameSrc);
    auto tracks = frameSrc->getTrackInfo();
    if (frameSrc->_avcHeader) {
        logTrace << "send a avc header, path: " << _urlParser.path_;
        _flvWriter->onWriteFlvTag(RTMP_VIDEO, frameSrc->_avcHeader, 0);
        //sendFlvTag(RTMP_VIDEO, 0, rtmpSrc->_avcHeader, rtmpSrc->_avcHeaderSize);
    }

    if (frameSrc->_aacHeader) {
        logTrace << "send a aac header, path: " << _urlParser.path_;
        _flvWriter->onWriteFlvTag(RTMP_AUDIO, frameSrc->_aacHeader, 0);
    }
    if (!_playFrameReader) {
        logInfo << "set _playFrameReader";
        _playFrameReader = frameSrc->getRing()->attach(EventLoop::getCurrentLoop(), true);
        _playFrameReader->setGetInfoCB([wSelf]() {
            auto self = wSelf.lock();
            ClientInfo ret;
            if (!self) {
                return ret;
            }
            ret.ip_ = "record-flv";
            ret.port_ = 0;
            ret.protocol_ = self->_urlParser.protocol_;
            return ret;
            });
        _playFrameReader->setDetachCB([wSelf]() {
            auto strong_self = wSelf.lock();
            if (!strong_self) {
                return;
            }
            // strong_self->shutdown(SockException(Err_shutdown, "rtsp ring buffer detached"));
            strong_self->onError("ring buffer detach");
            });
        logInfo << "setReadCB =================";
        _playFrameReader->setReadCB([wSelf](const RtmpMediaSource::RingDataType& pack) {
            auto self = wSelf.lock();
            if (!self/* || pack->empty()*/) {
                return;
            }
            // logInfo << "send rtmp msg";
            auto pktList = *(pack.get());
            for (auto& pkt : pktList) {
                //auto buffer = StreamBuffer::create();
                //buffer->assign(pkt->payload(), pkt->size());

                auto task = make_shared<WorkTask>();
                task->priority_ = 100;
                task->func_ = [wSelf, pkt]() {
                    auto self = wSelf.lock();
                    if (!self) {
                        return;
                    }
                    self->_flvWriter->onWriteFlvTag(pkt, pkt->abs_timestamp);
                    };
                self->_workLoop->addOrderTask(task);
            }
            });
        _source = frameSrc;
        }
}
void RecordFlv::stop()
{
    auto self = dynamic_pointer_cast<RecordFlv>(shared_from_this());
    // _loop->async([self](){
    //     self->_mp4Writer->stop();
    // }, true);

    auto task = make_shared<WorkTask>();
    task->priority_ = 100;
    task->func_ = [self]() {
        // auto self = wSelf.lock();
        // if (!self) {
        //     return ;
        // }
        //self->_mp4Writer->stop();
        self->_recordInfo.endTime = time(nullptr);
        self->_recordInfo.duration = self->_recordInfo.endTime - self->_recordInfo.startTime;
        struct stat statbuf;
        if (stat(self->_recordInfo.filePath.data(), &statbuf) == 0) {
            self->_recordInfo.fileSize = statbuf.st_size;
        }


        auto hook = HookManager::instance()->getHook("MediaHook");
        if (hook) {
            hook->onRecord(self->_recordInfo);
        }
        };
    _workLoop->addOrderTask(task);

    if (_onClose) {
        _onClose();
    }
    _stop = true;
}

void RecordFlv::onError(const string& err)
{
    logInfo << "get a err: " << err;
    stop();
}

void RecordFlv::setOnClose(const function<void()>& cb)
{
    _onClose = cb;
}


#endif