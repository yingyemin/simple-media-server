#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "PsMediaSource.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

PsMediaSource::PsMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop, bool muxer)
    :MediaSource(urlParser, loop)
    ,_muxer(muxer)
{
    _cache = std::make_shared<list<FrameBuffer::Ptr>>();
}

PsMediaSource::~PsMediaSource()
{
    logInfo << "~PsMediaSource";
}

void PsMediaSource::addTrack(const PsDemuxer::Ptr& track)
{
    std::weak_ptr<PsMediaSource> weakSelf = std::static_pointer_cast<PsMediaSource>(shared_from_this());
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
        _mapPsDecodeTrack.emplace(0, track);
    }

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
        track->setOnDecode([weakSelf](const FrameBuffer::Ptr& frame){
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
            startDecode();
            // _ring->addOnWrite(this, [this](RingDataType in, bool is_key){
            //     auto bufList = *(in.get());
            //     for (auto& buf : bufList) {
            //         inputPs(buf);
            //     }
            // });
        }
    }

    // setStatus(SourceStatus::AVAILABLE);
}

void PsMediaSource::onReady()
{
    MediaSource::onReady();
    if (_muxer) {
        std::weak_ptr<PsMediaSource> weakSelf = std::static_pointer_cast<PsMediaSource>(shared_from_this());
        _psEncodeTrack->setOnPsFrame([weakSelf](const FrameBuffer::Ptr& rtp){
            logInfo << "mux a ps packet";
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return;
            }
            if (true) {
                logInfo << "mux a ps packet mark";
                strongSelf->_cache->emplace_back(std::move(rtp));
                strongSelf->_ring->write(strongSelf->_cache);
                strongSelf->_cache = std::make_shared<list<FrameBuffer::Ptr>>();
            } else {
                logInfo << "mux a ps packet no mark";
                strongSelf->_cache->emplace_back(std::move(rtp));
            }
        });
        // if (_mapSink.size() > 0)
            startEncode();
    } else {
        if (_mapSink.size() == 0 && _ring) {
            // _ring->delOnWrite(this);
            
            stopDecode();
        }
    }
}

void PsMediaSource::addTrack(const shared_ptr<TrackInfo>& track)
{
    logTrace << "PsMediaSource::addTrack";
    MediaSource::addTrack(track);
    
    std::weak_ptr<PsMediaSource> weakSelf = std::static_pointer_cast<PsMediaSource>(shared_from_this());
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
    if (!_psEncodeTrack) {
        _psEncodeTrack = make_shared<PsMuxer>();

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

    _psEncodeTrack->addTrackInfo(track);
}

void PsMediaSource::addDecodeTrack(const shared_ptr<TrackInfo>& track)
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

void PsMediaSource::addSink(const MediaSource::Ptr &src)
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
        startDecode();
        // _ring->addOnWrite(this, [this](RingDataType in, bool is_key){
        //     auto rtpList = *(in.get());
        //     for (auto& rtp : rtpList) {
        //         auto track = _mapPsDecodeTrack[0];
        //         inputPs(rtp);
        //     }
        // });
    }
}

void PsMediaSource::delSink(const MediaSource::Ptr &src)
{
    logInfo << "PsMediaSource::delSink";
    if (!_loop->isCurrent()) {
        weak_ptr<PsMediaSource> wSlef = static_pointer_cast<PsMediaSource>(shared_from_this());
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
        // _ring->delOnWrite(this);
        
        stopDecode();
    }
    _mapSource.clear();
}

void PsMediaSource::onFrame(const FrameBuffer::Ptr& frame)
{
    logInfo << "on get a frame: index : " << frame->getTrackIndex();
    // auto it = _mapGB28181EncodeTrack.find(frame->getTrackIndex());
    if (!_psEncodeTrack) {
        return ;
    }
    // logInfo << "on muxer a frame";
    _psEncodeTrack->onFrame(frame);
}

void PsMediaSource::stopDecode()
{
    _decodeFlag = false;
}

void PsMediaSource::startDecode()
{
    _decodeFlag = true;
    _mapPsDecodeTrack[0]->clear();
    // auto track = make_shared<PsDemuxer>();
    // track->setOnDecode([weakSelf](const FrameBuffer::Ptr& frame){
    //     auto strongSelf = weakSelf.lock();
    //     if (!strongSelf) {
    //         return;
    //     }
    //     // logInfo << "on frame";
    //     for (auto& wSink : strongSelf->_mapSink) {
    //         // logInfo << "on frame to sink";
    //         // auto sink = wSink.second.lock();
    //         auto sink = wSink.second;
    //         if (!sink) {
    //             continue;
    //         }
    //         sink->onFrame(frame);
    //     }
    // });

    // _mapPsDecodeTrack[0] = track;
}

void PsMediaSource::stopEncode()
{
    _encodeFlag = false;
    _psEncodeTrack->stopEncode();
}

void PsMediaSource::startEncode()
{
    _encodeFlag = true;
    _psEncodeTrack->startEncode();
}

void PsMediaSource::inputPs(const FrameBuffer::Ptr& buffer)
{
    _ring->addBytes(buffer->size());
    _cache->emplace_back(std::move(buffer));
    // logInfo << "write cache size: " << strongSelf->_cache->size();
    _ring->write(_cache);
    _cache = std::make_shared<list<FrameBuffer::Ptr>>();

    if (_decodeFlag) {
        lock_guard<mutex> lck(_mtxTrack);
        auto demuxer = _mapPsDecodeTrack[0];
        demuxer->onPsStream(buffer->data(), buffer->size(), 0, 0);
    }
}

int PsMediaSource::playerCount()
{
    int count = _ring->readerCount();
    lock_guard<mutex> lck(_mtxTrack);
    count -= _mapSink.size();

    return count;
}

void PsMediaSource::getClientList(const function<void(const list<ClientInfo>& info)>& func)
{
    list<ClientInfo> clientInfo;
    _ring->getInfoList([func](list<ClientInfo> &infoList){
        func(infoList);
    });
}