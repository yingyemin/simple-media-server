#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "Fmp4MediaSource.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

Fmp4MediaSource::Fmp4MediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop, bool muxer)
    :MediaSource(urlParser, loop)
    ,_muxer(muxer)
{
    // _cache = std::make_shared<Buffer>();
}

Fmp4MediaSource::~Fmp4MediaSource()
{
    logInfo << "~Fmp4MediaSource";
}

void Fmp4MediaSource::addTrack(const Fmp4Demuxer::Ptr& track)
{
    std::weak_ptr<Fmp4MediaSource> weakSelf = std::static_pointer_cast<Fmp4MediaSource>(shared_from_this());
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
        _mapFmp4DecodeTrack.emplace(0, track);
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

void Fmp4MediaSource::onReady()
{
    MediaSource::onReady();
    if (_muxer) {
        _fmp4EncodeTrack->fmp4_writer_init_segment();
        _fmp4EncodeTrack->fmp4_writer_save_segment();
        _fmp4Header = _fmp4EncodeTrack->getFmp4Header();

        std::weak_ptr<Fmp4MediaSource> weakSelf = std::static_pointer_cast<Fmp4MediaSource>(shared_from_this());
        _fmp4EncodeTrack->setOnFmp4Segment([weakSelf](const Buffer::Ptr& rtp, bool keyframe){
            logInfo << "mux a ps packet";
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return;
            }
            if (true) {
                logInfo << "mux a ps packet mark";
                // strongSelf->_cache->emplace_back(std::move(rtp));
                strongSelf->_ring->write(rtp, keyframe);
                // strongSelf->_cache = std::make_shared<Buffer>();
            } else {
                logInfo << "mux a ps packet no mark";
                // strongSelf->_cache->emplace_back(std::move(rtp));
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

void Fmp4MediaSource::addTrack(const shared_ptr<TrackInfo>& track)
{
    logTrace << "Fmp4MediaSource::addTrack";
    MediaSource::addTrack(track);
    
    std::weak_ptr<Fmp4MediaSource> weakSelf = std::static_pointer_cast<Fmp4MediaSource>(shared_from_this());
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
    if (!_fmp4EncodeTrack) {
        _fmp4EncodeTrack = make_shared<Fmp4Muxer>(MOV_FLAG_SEGMENT);
        _fmp4EncodeTrack->init();

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

    if (track->trackType_ == "video") {
        _fmp4EncodeTrack->addVideoTrack(track);
    } else if (track->trackType_ == "audio") {
        _fmp4EncodeTrack->addAudioTrack(track);
    }
}

void Fmp4MediaSource::addDecodeTrack(const shared_ptr<TrackInfo>& track)
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

void Fmp4MediaSource::addSink(const MediaSource::Ptr &src)
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
        //         auto track = _mapFmp4DecodeTrack[0];
        //         inputPs(rtp);
        //     }
        // });
    }
}

void Fmp4MediaSource::delSink(const MediaSource::Ptr &src)
{
    logInfo << "Fmp4MediaSource::delSink";
    if (!_loop->isCurrent()) {
        weak_ptr<Fmp4MediaSource> wSlef = static_pointer_cast<Fmp4MediaSource>(shared_from_this());
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

void Fmp4MediaSource::onFrame(const FrameBuffer::Ptr& frame)
{
    logInfo << "on get a frame: index : " << frame->getTrackIndex();
    // auto it = _mapGB28181EncodeTrack.find(frame->getTrackIndex());
    if (!_fmp4EncodeTrack) {
        return ;
    }
    // logInfo << "on muxer a frame";
    _fmp4EncodeTrack->inputFrame_l(frame->getTrackIndex(), frame->data() + frame->startSize(), frame->size() - frame->startSize(), frame->pts(), frame->dts(), frame->keyFrame(), 1);
}

void Fmp4MediaSource::stopDecode()
{
    _decodeFlag = false;
}

void Fmp4MediaSource::startDecode()
{
    _decodeFlag = true;
    // _mapFmp4DecodeTrack[0]->clear();
    // auto track = make_shared<Fmp4Demuxer>();
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

    // _mapFmp4DecodeTrack[0] = track;
}

void Fmp4MediaSource::stopEncode()
{
    _encodeFlag = false;
    _fmp4EncodeTrack->stopEncode();
}

void Fmp4MediaSource::startEncode()
{
    _encodeFlag = true;
    _fmp4EncodeTrack->startEncode();
}

void Fmp4MediaSource::inputFmp4(const Buffer::Ptr& buffer)
{
    // _cache->emplace_back(std::move(buffer));
    // logInfo << "write cache size: " << strongSelf->_cache->size();
    _ring->addBytes(buffer->size());
    _ring->write(buffer);
    // _cache = std::make_shared<Buffer::Ptr>();

    // if (_decodeFlag) {
    //     lock_guard<mutex> lck(_mtxTrack);
    //     auto demuxer = _mapFmp4DecodeTrack[0];
    //     demuxer->onPsStream(buffer->data(), buffer->size(), 0, 0);
    // }
}

int Fmp4MediaSource::playerCount()
{
    int count = _ring->readerCount();
    lock_guard<mutex> lck(_mtxTrack);
    count -= _mapSink.size();

    return count;
}

void Fmp4MediaSource::getClientList(const function<void(const list<ClientInfo>& info)>& func)
{
    list<ClientInfo> clientInfo;
    _ring->getInfoList([func](list<ClientInfo> &infoList){
        func(infoList);
    });
}