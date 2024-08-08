#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "WebrtcMediaSource.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

WebrtcMediaSource::WebrtcMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop, bool muxer)
    :MediaSource(urlParser, loop)
    ,_muxer(muxer)
{
    _cache = std::make_shared<deque<RtpPacket::Ptr>>();
}

WebrtcMediaSource::~WebrtcMediaSource()
{
    logInfo << "~WebrtcMediaSource";
}

void WebrtcMediaSource::addTrack(const WebrtcDecodeTrack::Ptr& track)
{
    std::weak_ptr<WebrtcMediaSource> weakSelf = std::static_pointer_cast<WebrtcMediaSource>(shared_from_this());
    if (!_ring) {
        auto lam = [weakSelf](int size) {
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return;
            }
            strongSelf->getLoop()->async([weakSelf, size](){
                auto strongSelf = weakSelf.lock();
                if (!strongSelf) {
                    return;
                }
                strongSelf->onReaderChanged(size);
            }, true, true); 
        };
        logInfo << "create _ring";
        _ring = std::make_shared<QueType>(_ringSize, std::move(lam));
    }
    {
        lock_guard<mutex> lck(_mtxTrack);
        _mapWebrtcDecodeTrack.emplace(track->getTrackIndex(), track);
    }
    addTrack(track->getTrackInfo());
    
    track->setOnRtpPacket([weakSelf](const RtpPacket::Ptr& rtp){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        // logInfo << "on rtp seq: " << rtp->getSeq() << ", size: " << rtp->size() << ", type: " << rtp->type_;
        if (rtp->getHeader()->mark) {
            strongSelf->_cache->emplace_back(std::move(rtp));
            // logInfo << "write cache size: " << strongSelf->_cache->size();
            strongSelf->_ring->write(strongSelf->_cache);
            strongSelf->_cache = std::make_shared<deque<RtpPacket::Ptr>>();
        } else {
            strongSelf->_cache->emplace_back(std::move(rtp));
        }
    });

    if (!_muxer) {
        track->setOnFrame([weakSelf](const FrameBuffer::Ptr& frame){
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return;
            }
            // logInfo << "on frame";
            for (auto& sink : strongSelf->_mapSink) {
                // logInfo << "on frame to sink";
                // if (sink.second.lock()) {
                //     sink.second.lock()->onFrame(frame);
                // }
                sink.second->onFrame(frame);
            }
        });
        if (_mapSink.size() > 0) {
            track->startDecode();
        }
        for (auto& sink : _mapSink) {
            // if (sink.second.lock()) {
            //     sink.second.lock()->addTrack(track->getTrackInfo());
            // }
            sink.second->addTrack(track->getTrackInfo());
        }
    }
}

void WebrtcMediaSource::addTrack(const shared_ptr<TrackInfo>& track)
{
    MediaSource::addTrack(track);
    
    logInfo << "index: " << track->index_ << ", codec: " << track->codec_;
    std::weak_ptr<WebrtcMediaSource> weakSelf = std::static_pointer_cast<WebrtcMediaSource>(shared_from_this());
    if (!_ring) {
        auto lam = [weakSelf](int size) {
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return;
            }
            strongSelf->getLoop()->async([weakSelf, size](){
                auto strongSelf = weakSelf.lock();
                if (!strongSelf) {
                    return;
                }
                strongSelf->onReaderChanged(size);
            }, true, true); 
        };
        logInfo << "create _ring";
        _ring = std::make_shared<QueType>(_ringSize, std::move(lam));
    }
    auto rtcTrack = make_shared<WebrtcEncodeTrack>(track->index_, track);
    {
        lock_guard<mutex> lck(_mtxTrack);
        logInfo << "add track, index: " << track->index_;
        _mapWebrtcEncodeTrack.emplace(track->index_, rtcTrack);
    }
    if (_muxer) {
        rtcTrack->setOnRtpPacket([weakSelf](const RtpPacket::Ptr& rtp){
            // logInfo << "mux a rtp packet";
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return;
            }
            // logInfo << "on rtp seq: " << rtp->getSeq() << ", size: " << rtp->size() << ", type: " << rtp->type_;
            if (rtp->getHeader()->mark) {
                strongSelf->_cache->emplace_back(std::move(rtp));
                strongSelf->_ring->write(strongSelf->_cache);
                strongSelf->_cache = std::make_shared<deque<RtpPacket::Ptr>>();
            } else {
                strongSelf->_cache->emplace_back(std::move(rtp));
            }
        });
        // if (_mapSink.size() > 0)
            rtcTrack->startEncode();
    }
}

void WebrtcMediaSource::addSink(const MediaSource::Ptr &src)
{
    MediaSource::addSink(src);
    lock_guard<mutex> lck(_mtxTrack);
    for (auto& track : _mapWebrtcDecodeTrack) {
        src->addTrack(track.second->getTrackInfo());
        track.second->startDecode();
    }
    if (_status == SourceStatus::AVAILABLE) {
        src->onReady();
    }
    if (_mapSink.size() == 1) {
        _ring->addOnWrite(this, [this](DataType in, bool is_key){
            auto rtpList = *(in.get());
            for (auto& rtp : rtpList) {
                int index = rtp->trackIndex_;
                auto track = _mapWebrtcDecodeTrack[index];
                track->decodeRtp(rtp);
            }
        });
    }
}

void WebrtcMediaSource::delSink(const MediaSource::Ptr &src)
{
    logInfo << "WebrtcMediaSource::delSink";
    if (!_loop->isCurrent()) {
        weak_ptr<WebrtcMediaSource> wSlef = static_pointer_cast<WebrtcMediaSource>(shared_from_this());
        _loop->async([wSlef, src](){
            auto self = wSlef.lock();
            if (self) {
                self->delSink(src);
            }
        }, true, false);
    }
    MediaSource::delSink(src);
    lock_guard<mutex> lck(_mtxTrack);
    if (_mapSink.size() == 0) {
        _ring->delOnWrite(this);
        
        for (auto& track : _mapWebrtcDecodeTrack) {
            track.second->stopDecode();
        }
    }
    _mapSource.clear();
}

void WebrtcMediaSource::onFrame(const FrameBuffer::Ptr& frame)
{
    // logInfo << "on get a frame: index : " << frame->getTrackIndex()
    //           << ", codec: " << frame->codec();
    auto it = _mapWebrtcEncodeTrack.find(frame->getTrackIndex());
    if (it == _mapWebrtcEncodeTrack.end()) {
        return ;
    }
    // logInfo << "on muxer a frame, pts: " << frame->pts() << ", nal type: " << (int)(frame->getNalType());
    // if (frame->codec() == "h264") {
    //     FILE* fp = fopen("test2.264", "ab+");
    //     fwrite(frame->_buffer.data(), 1, frame->_buffer.size(), fp);
    //     fclose(fp);
    // }
    it->second->onFrame(frame);
}