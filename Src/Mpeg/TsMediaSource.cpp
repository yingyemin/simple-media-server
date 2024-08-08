#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "TsMediaSource.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

TsMediaSource::TsMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop, bool muxer)
    :MediaSource(urlParser, loop)
    ,_muxer(muxer)
{
    _cache = std::make_shared<list<StreamBuffer::Ptr>>();
}

TsMediaSource::~TsMediaSource()
{
    logInfo << "~TsMediaSource";
}

void TsMediaSource::addTrack(const TsDemuxer::Ptr& track)
{
    std::weak_ptr<TsMediaSource> weakSelf = std::static_pointer_cast<TsMediaSource>(shared_from_this());
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
        _mapTsDecodeTrack.emplace(0, track);
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
            //         inputTs(buf);
            //     }
            // });
        }
    }

    // setStatus(SourceStatus::AVAILABLE);
}

void TsMediaSource::onReady()
{
    MediaSource::onReady();
    if (_muxer) {
        std::weak_ptr<TsMediaSource> weakSelf = std::static_pointer_cast<TsMediaSource>(shared_from_this());
        _tsEncodeTrack->setOnTsPacket([weakSelf](const StreamBuffer::Ptr& rtp, int pts, int dts, bool keyframe){
            // logInfo << "mux a ps packet";
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return;
            }
            if (true) {
                // logInfo << "mux a ps packet mark";
                strongSelf->_cache->emplace_back(std::move(rtp));
                strongSelf->_ring->write(strongSelf->_cache);
                strongSelf->_cache = std::make_shared<list<StreamBuffer::Ptr>>();
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

void TsMediaSource::addTrack(const shared_ptr<TrackInfo>& track)
{
    logTrace << "TsMediaSource::addTrack";
    MediaSource::addTrack(track);
    
    std::weak_ptr<TsMediaSource> weakSelf = std::static_pointer_cast<TsMediaSource>(shared_from_this());
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
    if (!_tsEncodeTrack) {
        _tsEncodeTrack = make_shared<TsMuxer>();

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

    _tsEncodeTrack->addTrackInfo(track);
}

void TsMediaSource::addDecodeTrack(const shared_ptr<TrackInfo>& track)
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

void TsMediaSource::addSink(const MediaSource::Ptr &src)
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
        //         auto track = _mapTsDecodeTrack[0];
        //         inputTs(rtp);
        //     }
        // });
    }
}

void TsMediaSource::delSink(const MediaSource::Ptr &src)
{
    logInfo << "TsMediaSource::delSink";
    if (!_loop->isCurrent()) {
        weak_ptr<TsMediaSource> wSlef = static_pointer_cast<TsMediaSource>(shared_from_this());
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

void TsMediaSource::onFrame(const FrameBuffer::Ptr& frame)
{
    logInfo << "on get a frame: index : " << frame->getTrackIndex();
    // auto it = _mapGB28181EncodeTrack.find(frame->getTrackIndex());
    if (!_tsEncodeTrack) {
        return ;
    }
    // logInfo << "on muxer a frame";
    _tsEncodeTrack->onFrame(frame);
}

void TsMediaSource::stopDecode()
{
    logInfo << "TsMediaSource::stopDecode()";
    _decodeFlag = false;
}

void TsMediaSource::startDecode()
{
    _decodeFlag = true;
    _mapTsDecodeTrack[0]->clear();

    logInfo << "TsMediaSource::startDecode(): " << this;
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

    // _mapTsDecodeTrack[0] = track;
}

void TsMediaSource::stopEncode()
{
    _encodeFlag = false;
    _tsEncodeTrack->stopEncode();
}

void TsMediaSource::startEncode()
{
    _encodeFlag = true;
    _tsEncodeTrack->startEncode();
}

void TsMediaSource::inputTs(const StreamBuffer::Ptr& buffer)
{
    logInfo << "TsMediaSource::inputTs: " << this << ", " << _decodeFlag;
    _cache->emplace_back(std::move(buffer));
    // logInfo << "write cache size: " << strongSelf->_cache->size();
    _ring->write(_cache);
    _cache = std::make_shared<list<StreamBuffer::Ptr>>();

    if (_decodeFlag) {
        logInfo << "demuxer->onTsPacket";
        lock_guard<mutex> lck(_mtxTrack);
        auto demuxer = _mapTsDecodeTrack[0];
        demuxer->onTsPacket(buffer->data(), buffer->size(), 0);
    }
}