#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtpMediaSource.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

RtpMediaSource::RtpMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop, bool muxer)
    :MediaSource(urlParser, loop)
    ,_muxer(muxer)
{
    _cache = std::make_shared<list<RtpPacket::Ptr>>();
}

RtpMediaSource::~RtpMediaSource()
{
    logInfo << "~RtpMediaSource";
}

void RtpMediaSource::addTrack(const RtpDecodeTrack::Ptr& track)
{
    std::weak_ptr<RtpMediaSource> weakSelf = std::static_pointer_cast<RtpMediaSource>(shared_from_this());
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
        _mapRtpDecodeTrack.emplace(track->getTrackIndex(), track);
    }
    
    track->setOnRtpPacket([weakSelf](const RtpPacket::Ptr& rtp){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        // logInfo << "on rtp seq: " << rtp->getSeq();
        // logInfo << "on rtp mark: " << (int)rtp->getHeader()->mark;
        strongSelf->_ring->addBytes(rtp->size());
        if (rtp->getHeader()->mark || strongSelf->_cache->size() > 5) {
            strongSelf->_cache->emplace_back(std::move(rtp));
            // logInfo << "write cache size: " << strongSelf->_cache->size();
            strongSelf->_ring->write(strongSelf->_cache);
            strongSelf->_cache = std::make_shared<list<RtpPacket::Ptr>>();
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

    if (_payloadType == "ps" || _payloadType == "ts") {
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
    } else {
        addDecodeTrack(track->getTrackInfo());
    }

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
                    int index = rtp->trackIndex_;
                    // logInfo << "_mapRtpDecodeTrack size: " << strongSelf->_mapRtpDecodeTrack.size();
                    auto track = strongSelf->_mapRtpDecodeTrack[index];
                    track->decodeRtp(rtp);
                }
            });
        }
    }

    // setStatus(SourceStatus::AVAILABLE);
}

void RtpMediaSource::onReady()
{
    MediaSource::onReady();
    if (_muxer) {
        std::weak_ptr<RtpMediaSource> weakSelf = std::static_pointer_cast<RtpMediaSource>(shared_from_this());
        for (auto& track : _mapRtpEncodeTrack) {
            track.second->setOnRtpPacket([weakSelf](const RtpPacket::Ptr& rtp){
                logInfo << "mux a rtp packet";
                auto strongSelf = weakSelf.lock();
                if (!strongSelf) {
                    return;
                }
                if (rtp->getHeader()->mark) {
                    logInfo << "mux a rtp packet mark";
                    strongSelf->_cache->emplace_back(std::move(rtp));
                    strongSelf->_ring->write(strongSelf->_cache);
                    strongSelf->_cache = std::make_shared<list<RtpPacket::Ptr>>();
                } else {
                    logInfo << "mux a rtp packet no mark";
                    strongSelf->_cache->emplace_back(std::move(rtp));
                }
            });
            // if (_mapSink.size() > 0)
                track.second->startEncode();
        }
    } else {
        if (_mapSink.size() == 0 && _ring) {
            _probeFinish = true;
            
            for (auto& track : _mapRtpDecodeTrack) {
                track.second->stopDecode();
            }
        }
    }
}

void RtpMediaSource::addTrack(const shared_ptr<TrackInfo>& track)
{
    logTrace << "RtpMediaSource::addTrack";
    MediaSource::addTrack(track);
    
    std::weak_ptr<RtpMediaSource> weakSelf = std::static_pointer_cast<RtpMediaSource>(shared_from_this());
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

    if (_payloadType == "ps" || _payloadType == "ts") {
        if (_mapRtpEncodeTrack.find(0) == _mapRtpEncodeTrack.end()) {
            auto rtpTrack = make_shared<RtpEncodeTrack>(0, _payloadType);
            rtpTrack->setSsrc(_ssrc);

            _mapRtpEncodeTrack[0] = rtpTrack;
        }

        _mapRtpEncodeTrack[0]->addTrackInfo(track);
    } else {
        if (_mapRtpEncodeTrack.find(track->index_) == _mapRtpEncodeTrack.end()) {
            auto rtpTrack = make_shared<RtpEncodeTrack>(track->index_, "raw");
            rtpTrack->setSsrc(_ssrc);

            _mapRtpEncodeTrack[track->index_] = rtpTrack;
            rtpTrack->addTrackInfo(track);
        }
    }

    // _rtpEncodeTrack->addTrackInfo(track);
}

void RtpMediaSource::addDecodeTrack(const shared_ptr<TrackInfo>& track)
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

void RtpMediaSource::addSink(const MediaSource::Ptr &src)
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
        for (auto& track : _mapRtpDecodeTrack) {
            // src->addTrack(track.second->getTrackInfo());
            track.second->startDecode();
        }
    }
    // if (_ring->getOnWriteSize() > 0) {
    //     return ;
    // }
    weak_ptr<RtpMediaSource> weakSelf = std::static_pointer_cast<RtpMediaSource>(shared_from_this());
    _ring->addOnWrite(this, [weakSelf](RingDataType in, bool is_key){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        auto rtpList = *(in.get());
        for (auto& rtp : rtpList) {
            int index = rtp->trackIndex_;
            auto track = strongSelf->_mapRtpDecodeTrack[index];
            track->decodeRtp(rtp);
        }
    });
}

void RtpMediaSource::delSink(const MediaSource::Ptr &src)
{
    logInfo << "RtpMediaSource::delSink";
    if (!_loop->isCurrent()) {
        weak_ptr<RtpMediaSource> wSlef = static_pointer_cast<RtpMediaSource>(shared_from_this());
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
        for (auto& track : _mapRtpDecodeTrack) {
            track.second->stopDecode();
        }
    }
    _mapSource.clear();
}

void RtpMediaSource::onFrame(const FrameBuffer::Ptr& frame)
{
    logInfo << "on get a frame: index : " << frame->getTrackIndex();
    if (_payloadType == "ps" || _payloadType == "ts") {
        if (_mapRtpEncodeTrack.find(0) == _mapRtpEncodeTrack.end()) {
            return ;
        }

        _mapRtpEncodeTrack[0]->onFrame(frame);
    } else {
        auto it = _mapRtpEncodeTrack.find(frame->getTrackIndex());
        if (it == _mapRtpEncodeTrack.end()) {
            return ;
        }

        it->second->onFrame(frame);
    }
}

int RtpMediaSource::playerCount()
{
    if (!_ring) {
        return 0;
    }
    
    int count = _ring->readerCount();
    lock_guard<mutex> lck(_mtxTrack);
    count -= _mapSink.size();

    return count;
}

void RtpMediaSource::getClientList(const function<void(const list<ClientInfo>& info)>& func)
{
    list<ClientInfo> clientInfo;
    _ring->getInfoList([func](list<ClientInfo> &infoList){
        func(infoList);
    });
}