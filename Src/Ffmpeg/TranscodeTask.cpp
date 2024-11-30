#ifdef ENABLE_FFMPEG

#include "TranscodeTask.h"
#include "Log/Logger.h"
#include "Common/Define.h"
#include "Common/UrlParser.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"
#include "Codec/AacTrack.h"
#include "Codec/G711Track.h"
#include "Codec/H264Frame.h"
#include "Codec/H265Frame.h"

using namespace std;

mutex TranscodeTask::_mtx;
unordered_map<string, TranscodeTask::Ptr> TranscodeTask::_mapTask;

TranscodeTask::TranscodeTask()
{}

TranscodeTask::~TranscodeTask()
{
    if (_source) {
        _source->release();
        _source->delConnection(this);
    }

    auto originSrc = _originSource.lock();
    if (originSrc) {
        originSrc->release();
        originSrc->delConnection(this);
    }
}

shared_ptr<TranscodeTask> TranscodeTask::instance()
{
    static shared_ptr<TranscodeTask> task = make_shared<TranscodeTask>();

    return task;
}

std::string TranscodeTask::addTask(const std::string& uri, const std::string& videoCodec, const std::string& audioCodec)
{
    string key = uri + "_" + videoCodec + "_" + audioCodec;
    {
        lock_guard<mutex> lck(_mtx);
        if (_mapTask.find(key) != _mapTask.end()) {
            throw runtime_error("a same task is exists");
        }
    }

    auto task = std::make_shared<TranscodeTask>();
    string taskId = task->init(uri, videoCodec, audioCodec);

    {
        lock_guard<mutex> lck(_mtx);
        _mapTask.emplace(key, task);
    }

    return taskId;
}

std::string TranscodeTask::init(const std::string& uri, const std::string& videoCodec, const std::string& audioCodec)
{
    if (!_workLoop) {
        _workLoop = WorkLoopPool::instance()->getLoopByCircle();
    }

    _taskId = uri + "_" + videoCodec + "_" + audioCodec;
    auto originSource = MediaSource::get(uri, DEFAULT_VHOST);
    if (!originSource) {
        throw runtime_error("origin source is not exists: " + uri);
    }

    auto source = originSource->getOrCreate(PROTOCOL_FRAME, "transcode",
    [uri, originSource](){
        UrlParser urlParser;
        urlParser.protocol_ = PROTOCOL_FRAME;
        urlParser.path_ = uri;
        urlParser.type_ = "transcode";
        return make_shared<FrameMediaSource>(urlParser, originSource->getLoop());
    });

    if (!source) {
        throw runtime_error("create frame source error");
    }

    auto frameSrc = dynamic_pointer_cast<FrameMediaSource>(source);
    if (!frameSrc) {
        throw runtime_error("dynamic_pointer_cast frame source error");
    }

    _source = frameSrc;
    weak_ptr<TranscodeTask> wSelf = shared_from_this();

    auto tracks = originSource->getTrackInfo();
    for (auto& iter: tracks) {
        auto track = iter.second;
        logInfo << "track->trackType_: " << track->trackType_;

        if (track->trackType_ == "video" && !videoCodec.empty()) {
            VideoEncodeOption option;
            TrackInfo::Ptr newTrack;
            if (videoCodec == "h264") {
                option.codec_ = "libx264";
                newTrack = H264Track::createTrack(track->index_, track->payloadType_, 90000);
            } else if (videoCodec == "h265") {
                option.codec_ = "libx265";
                newTrack = make_shared<H265Track>();
                newTrack->codec_ = "h265";
            } else {
                throw runtime_error("video codec not support: " + videoCodec);
            }

            newTrack->payloadType_ = track->payloadType_;
            newTrack->index_ = track->index_;
            newTrack->trackType_ = track->trackType_;
            newTrack->samplerate_ = track->samplerate_;
            frameSrc->addTrack(newTrack);
            // 如果要264转265，这里改成AV_CODEC_ID_H264
            _transcodeVideo.reset(new TranscodeVideo(option, track->codec_ == "h264"? AV_CODEC_ID_H264 : AV_CODEC_ID_H265));
            _transcodeVideo->setOnPacket([wSelf, frameSrc, newTrack](const StreamBuffer::Ptr &packet){
                auto self = wSelf.lock();
                if (!self || !self->_eventLoop) {
                    return ;
                }

                FrameBuffer::Ptr frame;
                if (newTrack->codec_ == "h264") {
                    frame = make_shared<H264Frame>();
                } else {
                    frame = make_shared<H265Frame>();
                }
                // frame->_buffer.assign("\x0\x0\x0\x1", 4);
                frame->_buffer.assign(packet->data(), packet->size());
                frame->_startSize = 4;
                frame->_index = newTrack->index_;
                frame->_trackType = VideoTrackType;
                frame->_codec = newTrack->codec_;

                // FILE* fp = fopen("testvod.h265", "ab+");
                // fwrite(frame->data(), 1, frame->size(), fp);
                // fclose(fp);

                self->_eventLoop->async([newTrack, frameSrc, frame]() {
                    // logInfo << "on frame after transcode";
                    frame->split([newTrack, frameSrc](const FrameBuffer::Ptr& subFrame) {
                        if (!newTrack->isReady()) {
                            if (subFrame->getNalType() == H265_VPS) {
                                newTrack->setVps(subFrame);
                            } else if (subFrame->getNalType() == H265_SPS
                                        || subFrame->getNalType() == H264_SPS) {
                                newTrack->setSps(subFrame);
                            } else if (subFrame->getNalType() == H265_PPS
                                        || subFrame->getNalType() == H264_PPS) {
                                newTrack->setPps(subFrame);
                            }
                        }
                        // logInfo << "get a frame type: " << (int)subFrame->getNalType();

                        if (newTrack->isReady()) {
                            frameSrc->onFrame(subFrame);
                        }
                    });
                    // frameSrc->onFrame(frame);
                }, true);
            });
            _transcodeVideo->initDecode();
        } else if (track->trackType_ == "audio" && !audioCodec.empty()) {
            TrackInfo::Ptr newTrack;
            if (audioCodec == "aac") {
                newTrack = make_shared<AacTrack>();
                newTrack->codec_ = "aac";
            } else if (audioCodec == "g711a") {
                newTrack = make_shared<G711aTrack>();
                newTrack->codec_ = audioCodec;
            } else if (audioCodec == "g711u") {
                newTrack = make_shared<G711uTrack>();
                newTrack->codec_ = audioCodec;
            } else {
                throw runtime_error("audio codec not support: " + audioCodec);
            }

            newTrack->payloadType_ = track->payloadType_;
            newTrack->index_ = track->index_;
            newTrack->trackType_ = track->trackType_;
            newTrack->samplerate_ = track->samplerate_;
            newTrack->channel_ = track->channel_;
            newTrack->bitPerSample_ = track->bitPerSample_;
            frameSrc->addTrack(newTrack);

            _transcodeAudio = make_shared<AudioDecoder>(track);
            auto encoder = make_shared<AudioEncoder>(newTrack);

            _transcodeAudio->setOnDecode([encoder](const FFmpegFrame::Ptr & frame) {
                encoder->inputFrame(frame, true);
            });
            //_audio_enc->setOnEncode([this](const Frame::Ptr& frame) {
            encoder->setOnPacket([wSelf, frameSrc, newTrack](const FrameBuffer::Ptr& frame) {
                auto self = wSelf.lock();
                if (!self || !self->_eventLoop) {
                    return ;
                }

                // frame->_startSize = 7;
                frame->_index = newTrack->index_;
                frame->_trackType = AudioTrackType;
                // frame->_codec = newTrack->codec_;
                // frameSrc->onFrame(frame);

                self->_eventLoop->async([newTrack, frameSrc, frame]() {
                    frameSrc->onFrame(frame);
                }, true);
            });
        } else {
            frameSrc->addTrack(track);
        }
    }

    // frameSrc->onReady();

    UrlParser urlParser;
    urlParser.protocol_ = PROTOCOL_FRAME;
    urlParser.path_ = uri;
    urlParser.vhost_ = DEFAULT_VHOST;
    urlParser.type_ = "trans-source";
    MediaSource::getOrCreateAsync(urlParser.path_, urlParser.vhost_, urlParser.protocol_, urlParser.type_, 
    [wSelf](const MediaSource::Ptr &src){
        logInfo << "get a src";
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        src->getLoop()->async([wSelf, src](){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            self->onOriginFrameSource(src);
        }, true);
    }, 
    [wSelf, urlParser]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }
        return make_shared<FrameMediaSource>(urlParser, nullptr);
    }, this);

    return _taskId;
}

void TranscodeTask::setBitrate(int bitrate)
{
    if (_transcodeVideo) {
        _transcodeVideo->setBitrate(bitrate);
    }
}

void TranscodeTask::onOriginFrameSource(const MediaSource::Ptr &src)
{
    _eventLoop = src->getLoop();
    auto transSrc = _source;
    if (!transSrc) {
        return;
    }

    transSrc->setLoop(_eventLoop);

    auto frameSrc = dynamic_pointer_cast<FrameMediaSource>(src);
    _originSource = frameSrc;
    weak_ptr<TranscodeTask> wSelf = shared_from_this();
    _playReader = frameSrc->getRing()->attach(frameSrc->getLoop(), true);
    _playReader->setGetInfoCB([wSelf]() {
        auto self = wSelf.lock();
        ClientInfo ret;
        if (!self) {
            return ret;
        }
        // ret.ip_ = self->_socket->getLocalIp();
        // ret.port_ = self->_socket->getLocalPort();
        ret.protocol_ = PROTOCOL_FRAME;
        // ret.bitrate_ = self->_lastBitrate;
        ret.close_ = [wSelf](){
            auto self = wSelf.lock();
            if (self) {
                self->close();
            }
        };
        return ret;
    });
    _playReader->setDetachCB([wSelf]() {
        auto strong_self = wSelf.lock();
        if (!strong_self) {
            return;
        }
        // strong_self->shutdown(SockException(Err_shutdown, "rtsp ring buffer detached"));
        strong_self->close();
    });
    // logInfo << "setReadCB =================";
    _playReader->setReadCB([wSelf](const MediaSource::FrameRingDataType &pack) {
        // logInfo << "get a frame from origin source";
        auto self = wSelf.lock();
        if (!self/* || pack->empty()*/) {
            return;
        }
        
        auto frameSrc = self->_source;
        if (!frameSrc) {
            return ;
        }

        // logInfo << "start decode ===========";
        if (pack->_trackType == VideoTrackType) {
            if (self->_transcodeVideo) {
                WorkTask::Ptr task = make_shared<WorkTask>();
                task->priority_ = 100;
                task->func_ = [wSelf, pack](){
                    auto self = wSelf.lock();
                    if (!self/* || pack->empty()*/) {
                        return;
                    }
                    
                    // logInfo << "_transcodeVideo->inputFrame";

                    if (self->_transcodeVideo)
                        self->_transcodeVideo->inputFrame(pack);
                };
                // logInfo << "add work task";
                self->_workLoop->addOrderTask(task);
            } else {
                frameSrc->onFrame(pack);
            }
        } else if (pack->_trackType == AudioTrackType) {
            if (self->_transcodeAudio) {
                WorkTask::Ptr task = make_shared<WorkTask>();
                task->priority_ = 100;
                task->func_ = [wSelf, pack](){
                    auto self = wSelf.lock();
                    if (!self/* || pack->empty()*/) {
                        return;
                    }
                    
                    if (self->_transcodeAudio)
                        self->_transcodeAudio->inputFrame(pack, true, true, false);
                };
                self->_workLoop->addOrderTask(task);
            } else {
                frameSrc->onFrame(pack);
            }
        }
    });

    frameSrc->addOnDetach(this, [wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        self->close();
    });
}

void TranscodeTask::delTask(const std::string& taskId)
{
    lock_guard<mutex> lck(_mtx);
    _mapTask.erase(taskId);
}

TranscodeTask::Ptr TranscodeTask::getTask(const std::string& taskId)
{
    lock_guard<mutex> lck(_mtx);
    auto iter = _mapTask.find(taskId);
    if (iter != _mapTask.end()) {
        return iter->second;
    }

    return nullptr;
}

void TranscodeTask::close()
{
    delTask(_taskId);
}

#endif