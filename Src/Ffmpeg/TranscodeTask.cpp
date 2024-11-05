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

TranscodeTask::TranscodeTask()
{}

TranscodeTask::~TranscodeTask()
{}

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
    _taskId = key;
    auto originSource = MediaSource::get(uri, DEFAULT_VHOST);
    if (!originSource) {
        throw runtime_error("origin source is not exists");
    }

    auto source = MediaSource::getOrCreate(uri, PROTOCOL_FRAME, DEFAULT_VHOST, "transcode",
    [uri](){
        UrlParser urlParser;
        urlParser.protocol_ = PROTOCOL_FRAME;
        urlParser.path_ = uri;
        return make_shared<FrameMediaSource>(urlParser, EventLoop::getCurrentLoop());
    });

    if (!source) {
        throw runtime_error("create frame source error");
    }

    if (source->getLoop()) {
        throw runtime_error("frame source is exists");
    }

    auto frameSrc = dynamic_pointer_cast<FrameMediaSource>(source);
    if (!frameSrc) {
        throw runtime_error("dynamic_pointer_cast frame source error");
    }

    _source = frameSrc;

    auto tracks = source->getTrackInfo();
    for (auto& iter: tracks) {
        auto track = iter.second;
        if (track->trackType_ == "video" && !videoCodec.empty()) {
            VideoEncodeOption option;
            // 如果要264转265，这里改成libx265
            option.codec_ = videoCodec;
            TrackInfo::Ptr newTrack;
            if (videoCodec == "libx264") {
                newTrack = H264Track::createTrack(track->index_, track->payloadType_, 90000);
            } else if (videoCodec == "libx265") {
                newTrack = make_shared<H265Track>();
                newTrack->codec_ = "h265";
            }
            newTrack->payloadType_ = track->payloadType_;
            newTrack->index_ = track->index_;
            newTrack->trackType_ = track->trackType_;
            frameSrc->addTrack(newTrack);
            // 如果要264转265，这里改成AV_CODEC_ID_H264
            _transcodeVideo.reset(new TranscodeVideo(option, track->codec_ == "h264"? AV_CODEC_ID_H264 : AV_CODEC_ID_H265));
            _transcodeVideo->setOnPacket([frameSrc, newTrack](const StreamBuffer::Ptr &packet){
                FrameBuffer::Ptr frame;
                if (newTrack->codec_ == "h264") {
                    frame = make_shared<H264Frame>();
                } else {
                    frame = make_shared<H265Frame>();
                }
                frame->_buffer.assign("\x0\x0\x0\x1", 4);
                frame->_buffer.append(packet->data(), packet->size());
                frame->_startSize = 4;
                frame->_index = newTrack->index_;
                frame->_trackType = VideoTrackType;
                frame->_codec = newTrack->codec_;
                frameSrc->onFrame(frame);
            });
            _transcodeVideo->initDecode();
        } else if (track->trackType_ == "audio" && !audioCodec.empty()) {
            TrackInfo::Ptr newTrack;
            if (audioCodec == "libfdk_aac") {
                newTrack = make_shared<AacTrack>();
                newTrack->codec_ = "aac";
            } else if (audioCodec == "pcm_alaw" || audioCodec == "pcm_mulaw") {
                newTrack = make_shared<H265Track>();
                newTrack->codec_ = audioCodec == "pcm_alaw" ? "g711a" : "g711u";
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
            encoder->setOnPacket([frameSrc, newTrack](const FrameBuffer::Ptr& frame) {
                frame->_startSize = 7;
                frame->_index = newTrack->index_;
                frame->_trackType = AudioTrackType;
                frame->_codec = newTrack->codec_;
                frameSrc->onFrame(frame);
            });
        }
    }

    weak_ptr<TranscodeTask> wSelf = shared_from_this();
    UrlParser urlParser;
    urlParser.protocol_ = PROTOCOL_FRAME;
    urlParser.path_ = uri;
    urlParser.vhost_ = DEFAULT_VHOST;
    urlParser.type_ = DEFAULT_TYPE;
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

    {
        lock_guard<mutex> lck(_mtx);
        _mapTask.emplace(key, shared_from_this());
    }

    return _taskId;
}

void TranscodeTask::onOriginFrameSource(const MediaSource::Ptr &src)
{
    auto frameSrc = dynamic_pointer_cast<FrameMediaSource>(src);
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
    logInfo << "setReadCB =================";
    _playReader->setReadCB([wSelf](const MediaSource::FrameRingDataType &pack) {
        auto self = wSelf.lock();
        if (!self/* || pack->empty()*/) {
            return;
        }
        
        auto frameSrc = self->_source.lock();
        if (!frameSrc) {
            return ;
        }

        if (pack->_trackType == VideoTrackType) {
            self->_transcodeVideo->inputFrame(pack);
        } else {
            self->_transcodeAudio->inputFrame(pack, true, true, false);
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

void TranscodeTask::close()
{
    lock_guard<mutex> lck(_mtx);
    _mapTask.erase(_taskId);
}

#endif