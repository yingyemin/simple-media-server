﻿#include <cstdlib>
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
    logInfo << "~JT1078MediaSource";
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
        logInfo << "create _ring";
        _ring = std::make_shared<RingType>(_ringSize, std::move(lam));
    }
    {
        lock_guard<mutex> lck(_mtxTrack);
        _mapJT1078DecodeTrack.emplace(track->getTrackIndex(), track);
    }
    
    track->setOnRtpPacket([weakSelf](const JT1078RtpPacket::Ptr& rtp){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        // logInfo << "on rtp seq: " << rtp->getSeq();
        strongSelf->_ring->addBytes(rtp->size());
        if (rtp->getHeader()->mark) {
            strongSelf->_cache->emplace_back(std::move(rtp));
            // logInfo << "write cache size: " << strongSelf->_cache->size();
            strongSelf->_ring->write(strongSelf->_cache);
            strongSelf->_cache = std::make_shared<list<JT1078RtpPacket::Ptr>>();
            if (strongSelf->_probeFinish) {
                if (strongSelf->_mapSink.empty()) {
                    strongSelf->_ring->delOnWrite(strongSelf.get());
                }
                strongSelf->_probeFinish = false;
            }
        } else {
            strongSelf->_cache->emplace_back(std::move(rtp));
        }
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
    MediaSource::onReady();
    if (_mapSink.size() == 0) {
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
    
    // std::weak_ptr<JT1078MediaSource> weakSelf = std::static_pointer_cast<JT1078MediaSource>(shared_from_this());
    // if (!_ring) {
    //     auto lam = [weakSelf](int size) {
    //         auto strongSelf = weakSelf.lock();
    //         if (!strongSelf) {
    //             return;
    //         }
    //         strongSelf->getLoop()->async([weakSelf, size](){
    //             auto strongSelf = weakSelf.lock();
    //             if (!strongSelf) {
    //                 return;
    //             }
    //             strongSelf->onReaderChanged(size);
    //         }, true, true); 
    //     };
    //     logInfo << "create _ring";
    //     _ring = std::make_shared<RingType>(_ringSize, std::move(lam));
    // }
    // auto rtspTrack = make_shared<GB28181EncodeTrack>(track->index_, track);
    // {
    //     lock_guard<mutex> lck(_mtxTrack);
    //     logInfo << "add track, index: " << track->index_;
    //     _mapGB28181EncodeTrack.emplace(track->index_, rtspTrack);
    // }
    // if (_muxer) {
    //     rtspTrack->setOnRtpPacket([weakSelf](const RtpPacket::Ptr& rtp){
    //         // logInfo << "mux a rtp packet";
    //         auto strongSelf = weakSelf.lock();
    //         if (!strongSelf) {
    //             return;
    //         }
    //         if (rtp->getHeader()->mark) {
    //             strongSelf->_cache->emplace_back(std::move(rtp));
    //             strongSelf->_ring->write(strongSelf->_cache);
    //             strongSelf->_cache = std::make_shared<list<RtpPacket::Ptr>>();
    //         } else {
    //             strongSelf->_cache->emplace_back(std::move(rtp));
    //         }
    //     });
    //     // if (_mapSink.size() > 0)
    //         rtspTrack->startEncode();
    // }
}

void JT1078MediaSource::addDecodeTrack(const shared_ptr<TrackInfo>& track)
{
    logInfo << "addDecodeTrack ========= ";
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
    if (_ring->getOnWriteSize() > 0) {
        return ;
    }
    std::weak_ptr<JT1078MediaSource> weakSelf = std::static_pointer_cast<JT1078MediaSource>(shared_from_this());
    _ring->addOnWrite(src.get(), [weakSelf](RingDataType in, bool is_key){
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
    _ring->delOnWrite(src.get());
    lock_guard<mutex> lck(_mtxTrack);
    if (_mapSink.size() == 0) {
        for (auto& track : _mapJT1078DecodeTrack) {
            track.second->stopDecode();
        }
    }
    _mapSource.clear();
}

// void JT1078MediaSource::onFrame(const FrameBuffer::Ptr& frame)
// {
//     // logInfo << "on get a frame: index : " << frame->getTrackIndex();
//     auto it = _mapGB28181EncodeTrack.find(frame->getTrackIndex());
//     if (it == _mapGB28181EncodeTrack.end()) {
//         return ;
//     }
//     // logInfo << "on muxer a frame";
//     it->second->onFrame(frame);
// }