#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "GB28181MediaSource.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

GB28181MediaSource::GB28181MediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop, bool muxer)
    :MediaSource(urlParser, loop)
    ,_muxer(muxer)
{
    _cache = std::make_shared<list<RtpPacket::Ptr>>();
}

GB28181MediaSource::~GB28181MediaSource()
{
    logInfo << "~GB28181MediaSource";
}

void GB28181MediaSource::addTrack(const GB28181DecodeTrack::Ptr& track)
{
    std::weak_ptr<GB28181MediaSource> weakSelf = std::static_pointer_cast<GB28181MediaSource>(shared_from_this());
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
        _mapGB28181DecodeTrack.emplace(track->getTrackIndex(), track);
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
                    int index = rtp->trackIndex_;
                    // logInfo << "_mapGB28181DecodeTrack size: " << strongSelf->_mapGB28181DecodeTrack.size();
                    auto track = strongSelf->_mapGB28181DecodeTrack[index];
                    track->decodeRtp(rtp);
                }
            });
        }
    }

    // setStatus(SourceStatus::AVAILABLE);
}

void GB28181MediaSource::onReady()
{
    std::weak_ptr<GB28181MediaSource> weakSelf = std::static_pointer_cast<GB28181MediaSource>(shared_from_this());
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
    if (_muxer) {
        std::weak_ptr<GB28181MediaSource> weakSelf = std::static_pointer_cast<GB28181MediaSource>(shared_from_this());
        _gB28181EncodeTrack->setOnRtpPacket([weakSelf](const RtpPacket::Ptr& rtp){
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
            _gB28181EncodeTrack->startEncode();
    } else {
        if (_mapSink.size() == 0 && _ring) {
            _probeFinish = true;
            
            for (auto& track : _mapGB28181DecodeTrack) {
                track.second->stopDecode();
            }
        }
    }
}

void GB28181MediaSource::addTrack(const shared_ptr<TrackInfo>& track)
{
    logTrace << "GB28181MediaSource::addTrack";
    MediaSource::addTrack(track);
    
    std::weak_ptr<GB28181MediaSource> weakSelf = std::static_pointer_cast<GB28181MediaSource>(shared_from_this());
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
    if (!_gB28181EncodeTrack) {
        _gB28181EncodeTrack = make_shared<GB28181EncodeTrack>(0);
        _gB28181EncodeTrack->setSsrc(_ssrc);

        // if (_muxer) {
        //     gbTrack->setOnRtpPacket([weakSelf](const RtpPacket::Ptr& rtp){
        //         logInfo << "mux a rtp packet";
        //         auto strongSelf = weakSelf.lock();
        //         if (!strongSelf) {
        //             return;
        //         }
        //         if (rtp->getHeader()->mark) {
        //             logInfo << "mux a rtp packet mark";
        //             strongSelf->_cache->emplace_back(std::move(rtp));
        //             strongSelf->_ring->write(strongSelf->_cache);
        //             strongSelf->_cache = std::make_shared<list<RtpPacket::Ptr>>();
        //         } else {
        //             logInfo << "mux a rtp packet no mark";
        //             strongSelf->_cache->emplace_back(std::move(rtp));
        //         }
        //     });
        //     // if (_mapSink.size() > 0)
        //         gbTrack->startEncode();
        // }
    }

    _gB28181EncodeTrack->addTrackInfo(track);
}

void GB28181MediaSource::addDecodeTrack(const shared_ptr<TrackInfo>& track)
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

void GB28181MediaSource::addSink(const MediaSource::Ptr &src)
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
        for (auto& track : _mapGB28181DecodeTrack) {
            // src->addTrack(track.second->getTrackInfo());
            track.second->startDecode();
        }
    }
    // if (_ring->getOnWriteSize() > 0) {
    //     return ;
    // }
    weak_ptr<GB28181MediaSource> weakSelf = std::static_pointer_cast<GB28181MediaSource>(shared_from_this());
    _ring->addOnWrite(this, [weakSelf](RingDataType in, bool is_key){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        auto rtpList = *(in.get());
        for (auto& rtp : rtpList) {
            int index = rtp->trackIndex_;
            auto track = strongSelf->_mapGB28181DecodeTrack[index];
            track->decodeRtp(rtp);
        }
    });
}

void GB28181MediaSource::delSink(const MediaSource::Ptr &src)
{
    logInfo << "GB28181MediaSource::delSink";
    if (!_loop->isCurrent()) {
        weak_ptr<GB28181MediaSource> wSlef = static_pointer_cast<GB28181MediaSource>(shared_from_this());
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
        for (auto& track : _mapGB28181DecodeTrack) {
            track.second->stopDecode();
        }
    }
    _mapSource.clear();
}

void GB28181MediaSource::onFrame(const FrameBuffer::Ptr& frame)
{
    logInfo << "on get a frame: index : " << frame->getTrackIndex();
    // auto it = _mapGB28181EncodeTrack.find(frame->getTrackIndex());
    if (!_gB28181EncodeTrack) {
        return ;
    }
    // logInfo << "on muxer a frame";
    _gB28181EncodeTrack->onFrame(frame);
}

int GB28181MediaSource::playerCount()
{
    int count = _ring->readerCount();
    // lock_guard<mutex> lck(_mtxTrack);
    count -= _sinkSize;

    return count;
}

void GB28181MediaSource::getClientList(const function<void(const list<ClientInfo>& info)>& func)
{
    list<ClientInfo> clientInfo;
    _ring->getInfoList([func](list<ClientInfo> &infoList){
        func(infoList);
    });
}