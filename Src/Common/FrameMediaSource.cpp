#include "FrameMediaSource.h"
#include "Log/Logger.h"

using namespace std;

FrameMediaSource::FrameMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop)
    : MediaSource(urlParser, loop)
{
    // _cache = std::make_shared<toolkit::List<Frame::Ptr>>();
    // TODO get it from config
    // _ring = std::make_shared<FrameRingType>(250, nullptr);
    // logDebug << "create frame ring: " << _ring;
    _frameClock.start();
}

FrameMediaSource::~FrameMediaSource()
{
    logDebug << "~FrameMediaSource";
}

// void FrameMediaSource::onFrame(const FrameBuffer::Ptr& frame)
// {
    // if (frame->getTrackType() == VideoTrackType) {
    //     frame->split([&](const FrameBuffer::Ptr& subFrame) {
    //         if (subFrame->size() - subFrame->startSize() > 0) {
    //             inputFrame(subFrame);
    //         }
    //     });
    // }
// }

void FrameMediaSource::onFrame(const FrameBuffer::Ptr& frame)
{
    if (!_ring) {
        logDebug << "_ring is empty, path: " << _urlParser.path_ << ", this: " << this;
        return ;
    }

    if (_origin && !isReady()) {
        MediaSource::onFrame(frame);
        return ;
    }

    if (!frame || _mapTrackInfo.find(frame->getTrackIndex()) == _mapTrackInfo.end()) {
        return ;
    }

    if (!_mapTrackInfo[frame->getTrackIndex()]->isReady()) {
        return ;
    }

    uint64_t now = TimeClock::now();
    if (_frameClock.startToNow() > 5000) {
        _fps = _frameCount * 1000.0 / _frameClock.startToNow();
        _frameCount = 0;
        _frameClock.update();
    }

    // if (_urlParser.type_ == "transcode") {
    //     logInfo << "transcode frame: " << _urlParser.type_;
    // }

    if (!_origin)
        logTrace << "before adjust frame pts: " << frame->_pts << ", frame dts: " << frame->_dts << ", type: " << frame->_trackType
                << ", size: " << frame->size() << ", path: " << _urlParser.path_ << ", this: " << this;
    // for (auto& sink : _mapSink) {
        // logInfo << "on frame to sink";
        bool keyframe = false;
        if (frame->getTrackType() == VideoTrackType) {
            _lastFrameTime = now;
            ++_frameCount;
            // logInfo << "nal type: " << (int)frame->getNalType();
            if (frame->isNonPicNalu()) {
                return ;
            }

            // static bool del = false;
            // if (!del) {
            //     FILE* fp = fopen("testdele.h264", "ab+");
            //     if (frame->keyFrame()) {
            //         FrameBuffer::Ptr vps;
            //         FrameBuffer::Ptr sps;
            //         FrameBuffer::Ptr pps;
            //         _mapTrackInfo[frame->_index]->getVpsSpsPps(vps, sps, pps);
            //         fwrite(sps->data(), 1, sps->size(), fp);
            //         fwrite(pps->data(), 1, pps->size(), fp);
            //     }
            //     fwrite(frame->data(), 1, frame->size(), fp);
            //     fclose(fp);
            // }
            // if (frame->keyFrame()) {
            //     del = true;
            // } else {
            //     del = false;
            // }

            if (frame->startFrame()) {
                keyframe = true;
                // _sendConfig = true;

                if (_videoStampAdjust)
                    _videoStampAdjust->inputStamp(frame->_pts, frame->_dts, 1);
                if (_frame) {
                    _ring->write(_frame, false);
                    _frame = nullptr;
                }
                _ring->write(frame, keyframe);
            } else if (frame->isNewNalu() && _frame) {
                if (_frame->keyFrame()) {
                    _keyframe = _frame;
                    _gopTime = now - _lastKeyframeTime;
                    _lastKeyframeTime = now;
                    if (_frame->codec() == "h264" || _frame->codec() == "h265") {
                        if (!_sendConfig) {
                            FrameBuffer::Ptr vps;
                            FrameBuffer::Ptr sps;
                            FrameBuffer::Ptr pps;
                            _mapTrackInfo[_frame->_index]->getVpsSpsPps(vps, sps, pps);

                            if (_mapTrackInfo[_frame->_index]->codec_ == "h264") {
                                keyframe = true;
                            } else if (_mapTrackInfo[_frame->_index]->codec_ == "h265") {
                                keyframe = false;
                                vps->_dts = _frame->_dts;
                                vps->_pts = _frame->_pts;
                                vps->_index = _frame->_index;
                                _ring->write(vps, true);
                            }
                            
                            sps->_dts = _frame->_dts;
                            sps->_pts = _frame->_pts;
                            sps->_index = _frame->_index;
                            _ring->write(sps, keyframe);

                            pps->_dts = _frame->_dts;
                            pps->_pts = _frame->_pts;
                            pps->_index = _frame->_index;
                            _ring->write(pps, false);
                        }
                    }
                    _sendConfig = false;
                }

                // logInfo << "ring write: frame pts: " << _frame->_pts << ", frame dts: " << _frame->_dts 
                //     << ", type: " << _frame->_trackType << ", this: " << this;
                _ring->write(_frame, false);
                if (_videoStampAdjust)
                    _videoStampAdjust->inputStamp(frame->_pts, frame->_dts, 1);
                _frame = frame;
            } else {
                if (!_frame) {
                    if (_videoStampAdjust)
                        _videoStampAdjust->inputStamp(frame->_pts, frame->_dts, 1);
                    _frame = frame;
                } else {
                    _frame->_buffer.append(frame->_buffer);
                }
            }
        } else {
            int samples = 0;
            if (frame->_codec == "aac") {
                samples = 1024;
            } else if (frame->_codec == "g711a" || frame->_codec == "g711u") {
                samples = frame->size() - frame->startSize();
            }
            if (_audioStampAdjust)
                _audioStampAdjust->inputStamp(frame->_pts, frame->_dts, samples);
            _ring->write(frame, false);
        }
        if (!_origin)
            logTrace << "frame pts: " << frame->_pts << ", frame dts: " << frame->_dts 
                    << ", type: " << frame->_trackType << ", path: " << _urlParser.path_ << ", this: " << this;
        // logInfo << "keyframe: " << keyframe << ", size: " << frame->size() << ", type: " << (int)frame->getNalType();
        // logInfo << "frame source type: " << _urlParser.type_;
        // _ring->write(frame, keyframe);
    // }
}

void FrameMediaSource::addTrack(const shared_ptr<TrackInfo>& track)
{
    logTrace << "on add track to sink, uri: " << _urlParser.path_;
    weak_ptr<FrameMediaSource> weak_self = static_pointer_cast<FrameMediaSource>(shared_from_this());
    if (!_ring) {
        auto lam = [weak_self](int size) {
            auto strongSelf = weak_self.lock();
            if (!strongSelf) {
                return;
            }
            strongSelf->onReaderChanged(size);
        };
        _ring = std::make_shared<FrameRingType>(_ring_size, std::move(lam));
    }

    MediaSource::addTrack(track);
    if (track->trackType_ == "video" && !_origin) {
        _videoStampAdjust = make_shared<VideoStampAdjust>(0);
        auto originSrc = _originSrc.lock();
        if (originSrc) {
            _videoStampAdjust->setStampMode((StampMode)originSrc->getVideoStampMode());
        }
    } else if (track->trackType_ == "audio" && !_origin) {
        _audioStampAdjust = make_shared<AudioStampAdjust>(0);
        _audioStampAdjust->setCodec(track->codec_);
        auto originSrc = _originSrc.lock();
        if (originSrc) {
            _audioStampAdjust->setStampMode((StampMode)originSrc->getAudioStampMode());
        }
    }

    if (track->isReady()) {
        for (auto& sink : _mapSink) {
            logTrace << "on add track to sink, uri: " << _urlParser.path_;
        //     if (sink.second.lock()) {
        //         sink.second.lock()->addTrack(track);
        //     }
            sink.second->addTrack(track);
        }
    }
}

void FrameMediaSource::addSink(const MediaSource::Ptr &sink)
{
    MediaSource::addSink(sink);

    for (auto& track: _mapTrackInfo) {
        if (!track.second || !track.second->isReady()) {
            continue;
        }
        logTrace << "on add track to sink, uri: " << _urlParser.path_;
        sink->addTrack(track.second);
    }
    sink->onReady();
    weak_ptr<FrameMediaSource> weak_self = static_pointer_cast<FrameMediaSource>(shared_from_this());
    if (!_ring) {
        auto lam = [weak_self](int size) {
            auto strongSelf = weak_self.lock();
            if (!strongSelf) {
                return;
            }
            strongSelf->onReaderChanged(size);
        };
        _ring = std::make_shared<FrameRingType>(_ring_size, std::move(lam));
    }
    _ring->addOnWrite(sink.get(), [weak_self, sink](FrameRingDataType in, bool is_key){
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return ;
        }
        // logInfo << "frame pts: " << in->_pts << ", frame dts: " << in->_dts << ", type: " << in->_trackType;
        // logInfo << "keyframe: " << is_key << ", size: " << in->size() << ", type: " << (int)in->getNalType();
        // logInfo << "frame source type: " << strong_self->_urlParser.type_;
        sink->onFrame(in);
        // for (auto& sinkW: strong_self->_mapSink) {
        //     // auto sink = sinkW.second.lock();
        //     auto sink = sinkW.second;
        //     if (sink) {
        //         // logInfo << "sink->onFrame(in)";
        //         sink->onFrame(in);
        //     }
        // }
    });
}

void FrameMediaSource::delSink(const MediaSource::Ptr& sink)
{
    logTrace << "FrameMediaSource::delSink, uri: " << _urlParser.path_;
    MediaSource::delSink(sink);
    _ring->delOnWrite(sink.get());
    if (_mapSink.size() == 0) {
        for (auto& source : _mapSource) {
            source.second->delSink(static_pointer_cast<FrameMediaSource>(shared_from_this()));
        }
        if (_urlParser.type_ != "transcode") {
            release();
        }
    }
}


