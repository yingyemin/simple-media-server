#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "JT1078MediaSource.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

JT1078MediaSource::JT1078MediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop, bool muxer)
    :MediaSource(urlParser, loop)
    ,_muxer(muxer)
{
    _cache = std::make_shared<list<JT1078RtpPacket::Ptr>>();
}

JT1078MediaSource::~JT1078MediaSource()
{
    logTrace << "~JT1078MediaSource, " << _urlParser.path_;
}

void JT1078MediaSource::addTrack(const JT1078DecodeTrack::Ptr& track)
{
    std::weak_ptr<JT1078MediaSource> weakSelf = std::static_pointer_cast<JT1078MediaSource>(shared_from_this());
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
        logTrace << "create _ring: " << _urlParser.path_;
        _ring = std::make_shared<RingType>(_ringSize, std::move(lam));
    }
    {
        lock_guard<mutex> lck(_mtxTrack);
        _mapJT1078DecodeTrack.emplace(track->getTrackIndex(), track);
    }
    
    track->setOnRtpPacket([weakSelf](const JT1078RtpPacket::Ptr& rtp, bool start){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        if (start) {
            strongSelf->_start = start;
        }
        logTrace << "on rtp seq: " << rtp->getSeq() << ", time stamp: " << rtp->getTimestamp()
                 << ", mark: " << (int)rtp->getMark();
        strongSelf->_ring->addBytes(rtp->size());
        if (rtp->getMark() || rtp->getTimestamp() != strongSelf->_lastStamp || strongSelf->_cache->size() > 300) {
            strongSelf->_cache->emplace_back(std::move(rtp));
            // logInfo << "write cache size: " << strongSelf->_cache->size();
            strongSelf->_ring->write(strongSelf->_cache, strongSelf->_start);
            strongSelf->_cache = std::make_shared<list<JT1078RtpPacket::Ptr>>();
            strongSelf->_start = false;
            if (strongSelf->_probeFinish) {
                if (strongSelf->_mapSink.empty()) {
                    strongSelf->_ring->delOnWrite(strongSelf.get());
                }
                strongSelf->_probeFinish = false;
            }
        } else {
            strongSelf->_cache->emplace_back(std::move(rtp));
        }

        strongSelf->_lastStamp = rtp->getTimestamp();
    });

    track->setOnTrackInfo([weakSelf](const std::shared_ptr<TrackInfo> &trackInfo){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        strongSelf->addDecodeTrack(trackInfo);
    });

    track->setOnReady([weakSelf](){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        strongSelf->onReady();
    });

    if (!_muxer) {
        track->setOnFrame([weakSelf](const FrameBuffer::Ptr& frame){
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return;
            }
            // logInfo << "on frame";
            strongSelf->MediaSource::onFrame(frame);
            for (auto& wSink : strongSelf->_mapSink) {
                // logInfo << "on frame to sink";
                // auto sink = wSink.second.lock();
                auto sink = wSink.second;
                if (!sink) {
                    continue;
                }
                sink->onFrame(frame);
            }
        });
        if (_status < SourceStatus::AVAILABLE || _mapSink.size() > 0) {
            track->startDecode();
            _ring->addOnWrite(this, [weakSelf](RingDataType in, bool is_key){
                auto strongSelf = weakSelf.lock();
                if (!strongSelf) {
                    return;
                }
                auto rtpList = *(in.get());
                for (auto& rtp : rtpList) {
                    int index = rtp->getTrackType() == "video" ? VideoTrackType : AudioTrackType;
                    auto track = strongSelf->_mapJT1078DecodeTrack[index];
                    track->decodeRtp(rtp);
                }
            });
        }
    }

    // setStatus(SourceStatus::AVAILABLE);
}

void JT1078MediaSource::onReady()
{
    std::weak_ptr<JT1078MediaSource> weakSelf = std::static_pointer_cast<JT1078MediaSource>(shared_from_this());
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
        _ring = std::make_shared<RingType>(_ringSize, std::move(lam));
    }
    MediaSource::onReady();
    if (_mapSink.size() == 0 && _ring) {
        // _ring->delOnWrite(this);
        _probeFinish = true;
        
        for (auto& track : _mapJT1078DecodeTrack) {
            track.second->stopDecode();
        }
    }
}

void JT1078MediaSource::addTrack(const shared_ptr<TrackInfo>& track)
{
    MediaSource::addTrack(track);
    
    std::weak_ptr<JT1078MediaSource> weakSelf = std::static_pointer_cast<JT1078MediaSource>(shared_from_this());
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
        _ring = std::make_shared<RingType>(_ringSize, std::move(lam));
    }
    auto jt1078Track = make_shared<JT1078EncodeTrack>(track, _simCode, _channel);
    {
        lock_guard<mutex> lck(_mtxTrack);
        logInfo << "add track, index: " << track->index_;
        _mapJT1078EncodeTrack.emplace(track->index_, jt1078Track);
    }
    if (_muxer) {
        jt1078Track->setOnRtpPacket([weakSelf](const JT1078RtpPacket::Ptr& rtp){
            // logInfo << "mux a rtp packet";
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return;
            }
            if (rtp->getMark()) {
                strongSelf->_cache->emplace_back(std::move(rtp));
                strongSelf->_ring->write(strongSelf->_cache);
                strongSelf->_cache = std::make_shared<list<JT1078RtpPacket::Ptr>>();
            } else {
                strongSelf->_cache->emplace_back(std::move(rtp));
            }
        });
        // if (_mapSink.size() > 0)
            jt1078Track->startEncode();
    }
}

void JT1078MediaSource::addDecodeTrack(const shared_ptr<TrackInfo>& track)
{
    logTrace << "add track: " << track->trackType_ << "path: " << _urlParser.path_;
    MediaSource::addTrack(track);

    for (auto& wSink : _mapSink) {
        // auto sink = wSink.second.lock();
        auto sink = wSink.second;
        if (!sink) {
            continue;
        }
        sink->addTrack(track);
    }
}

void JT1078MediaSource::addSink(const MediaSource::Ptr &src)
{
    MediaSource::addSink(src);
    for (auto& trackInfo : _mapTrackInfo) {
        src->addTrack(trackInfo.second);
    }
    if (_status == SourceStatus::AVAILABLE) {
        src->onReady();
    }
    if (_mapSink.size() == 1 && _status == SourceStatus::AVAILABLE) {
        lock_guard<mutex> lck(_mtxTrack);
        for (auto& track : _mapJT1078DecodeTrack) {
            // src->addTrack(track.second->getTrackInfo());
            track.second->startDecode();
        }
    }
    // if (_ring->getOnWriteSize() > 0) {
    //     return ;
    // }
    std::weak_ptr<JT1078MediaSource> weakSelf = std::static_pointer_cast<JT1078MediaSource>(shared_from_this());
    _ring->addOnWrite(this, [weakSelf](RingDataType in, bool is_key){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        auto rtpList = *(in.get());
        for (auto& rtp : rtpList) {
            int index = rtp->getTrackType() == "video" ? VideoTrackType : AudioTrackType;
            auto track = strongSelf->_mapJT1078DecodeTrack[index];
            track->decodeRtp(rtp);
        }
    });
}

void JT1078MediaSource::delSink(const MediaSource::Ptr &src)
{
    logInfo << "JT1078MediaSource::delSink";
    if (!_loop->isCurrent()) {
        weak_ptr<JT1078MediaSource> wSlef = static_pointer_cast<JT1078MediaSource>(shared_from_this());
        _loop->async([wSlef, src](){
            auto self = wSlef.lock();
            if (self) {
                self->delSink(src);
            }
        }, true, false);
    }
    MediaSource::delSink(src);
    // _ring->delOnWrite(src.get());
    lock_guard<mutex> lck(_mtxTrack);
    if (_mapSink.size() == 0) {
        _ring->delOnWrite(this);
        for (auto& track : _mapJT1078DecodeTrack) {
            track.second->stopDecode();
        }
    }
    _mapSource.clear();
}

void JT1078MediaSource::onFrame(const FrameBuffer::Ptr& frame)
{
    // logInfo << "on get a frame: index : " << frame->getTrackIndex();
    auto it = _mapJT1078EncodeTrack.find(frame->getTrackIndex());
    if (it == _mapJT1078EncodeTrack.end()) {
        return ;
    }
    // logInfo << "on muxer a frame";
    it->second->onFrame(frame);
}