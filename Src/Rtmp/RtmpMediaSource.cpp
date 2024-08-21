#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtmpMediaSource.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

RtmpMediaSource::RtmpMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop, bool muxer)
    :MediaSource(urlParser, loop)
    ,_muxer(muxer)
{
    _cache = std::make_shared<list<RtmpMessage::Ptr>>();
}

RtmpMediaSource::~RtmpMediaSource()
{
    logInfo << "~RtmpMediaSource";
}

void RtmpMediaSource::addTrack(const RtmpDecodeTrack::Ptr& track)
{
    std::weak_ptr<RtmpMediaSource> weakSelf = std::static_pointer_cast<RtmpMediaSource>(shared_from_this());
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
        _mapRtmpDecodeTrack.emplace(track->getTrackIndex(), track);
    }
    MediaSource::addTrack(track->getTrackInfo());
    
    track->setOnRtmpPacket([weakSelf](const RtmpMessage::Ptr& pkt){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        if (pkt->isKeyFrame()) {
            strongSelf->_start = true;
        }
        // logInfo << "write rtmp packet: ";
        strongSelf->_ring->addBytes(pkt->length);
        if (pkt->abs_timestamp != strongSelf->_lastPts) {
            strongSelf->_cache->emplace_back(std::move(pkt));
            // logInfo << "write cache size: " << strongSelf->_cache->size();
            strongSelf->_ring->write(strongSelf->_cache, strongSelf->_start);
            strongSelf->_cache = std::make_shared<list<RtmpMessage::Ptr>>();
            strongSelf->_start= false;
        } else {
            strongSelf->_cache->emplace_back(std::move(pkt));
        }
        strongSelf->_lastPts = pkt->abs_timestamp;
        uint8_t frame_type = (pkt->payload.get()[0] >> 4) & 0x0f;
        uint8_t codec_id = pkt->payload.get()[0] & 0x0f;

        // logInfo << "frame_type : " << (int)frame_type << ", codec_id: " << (int)codec_id;
    });

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

    track->setOnTrackInfo([weakSelf](const std::shared_ptr<TrackInfo> &trackInfo){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        for (auto& sink : strongSelf->_mapSink) {
            sink.second->addTrack(trackInfo);
        }
    });

    if (_mapSink.size() > 0) {
        track->startDecode();
    }
}

void RtmpMediaSource::addTrack(const shared_ptr<TrackInfo>& track)
{
    MediaSource::addTrack(track);
    
    logInfo << "index: " << track->index_ << ", codec: " << track->codec_ ;
    std::weak_ptr<RtmpMediaSource> weakSelf = std::static_pointer_cast<RtmpMediaSource>(shared_from_this());
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
    auto rtmpTrack = make_shared<RtmpEncodeTrack>(track);
    {
        lock_guard<mutex> lck(_mtxTrack);
        logInfo << "add track, index: " << track->index_;
        int trackType = track->trackType_ == "video" ? VideoTrackType : AudioTrackType;
        logInfo << "add track, type: " << trackType;
        _mapRtmpEncodeTrack.emplace(trackType, rtmpTrack);
    }
    if (_muxer) {
        rtmpTrack->setOnRtmpPacket([weakSelf](const RtmpMessage::Ptr& pkt, bool start){
            // logInfo << "mux a rtmp packet";
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return;
            }
            if (start) {
                strongSelf->_start = start;
            }
            // logInfo << "mapsink size: " << strongSelf->_mapSink.size();
            logInfo << "pkt->abs_timestamp: " << pkt->abs_timestamp;
            if (pkt->abs_timestamp != strongSelf->_lastPts) {
                strongSelf->_cache->emplace_back(std::move(pkt));
                strongSelf->_ring->write(strongSelf->_cache, strongSelf->_start);
                strongSelf->_cache = std::make_shared<list<RtmpMessage::Ptr>>();
                strongSelf->_start = false;
            } else {
                strongSelf->_cache->emplace_back(std::move(pkt));
            }
            strongSelf->_lastPts = pkt->abs_timestamp;
        });
        rtmpTrack->startEncode();

        if (track->codec_ == "aac") {
            auto config = rtmpTrack->getConfig();
            _aacHeaderSize = config.size();
            _aacHeader.reset(new char[_aacHeaderSize], [](char* p){delete[] p;});
            memcpy(_aacHeader.get(), config.data(), _aacHeaderSize);
        } else if (track->trackType_ == "video") {
            auto config = rtmpTrack->getConfig();
            if (track->codec_ == "h264" || track->codec_ == "h265") {
                _avcHeaderSize = config.size();
                _avcHeader.reset(new char[_avcHeaderSize], [](char* p){delete[] p;});
                memcpy(_avcHeader.get(), config.data(), _avcHeaderSize);
            }
        }
    }
}

// RtspTrack::Ptr RtmpMediaSource::getTrack(int index)
// {
//     lock_guard<mutex> lck(_mtxTrack);
//     auto it = _mapRtmpEncodeTrack.find(index);
//     if (it != _mapRtmpEncodeTrack.end()) {
//         return it->second;
//     }

//     return nullptr;
// }


void RtmpMediaSource::addSink(const MediaSource::Ptr &src)
{
    logInfo << "RtmpMediaSource::addSink";
    MediaSource::addSink(src);
    for (auto& trackIt : _mapRtmpDecodeTrack) {
        src->addTrack(trackIt.second->getTrackInfo());
    }
    if (_status == SourceStatus::AVAILABLE) {
        src->onReady();
    }
    if (_mapSink.size() == 1) {
        lock_guard<mutex> lck(_mtxTrack);
        for (auto& track : _mapRtmpDecodeTrack) {
            // src->addTrack(track.second->getTrackInfo());
            track.second->startDecode();
        }
    }
    weak_ptr<RtmpMediaSource> weakSelf = std::static_pointer_cast<RtmpMediaSource>(shared_from_this());
    _ring->addOnWrite(src.get(), [weakSelf](RingDataType in, bool is_key){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        logInfo << "is key: " << is_key;
        auto pktList = *(in.get());
        for (auto& pkt : pktList) {
            int index = pkt->trackIndex_;
            auto track = strongSelf->_mapRtmpDecodeTrack.find(index);
            if (track != strongSelf->_mapRtmpDecodeTrack.end()) {
                track->second->decodeRtmp(pkt);
            }
        }
    });
}

void RtmpMediaSource::delSink(const MediaSource::Ptr &src)
{
    logInfo << "RtmpMediaSource::delSink";
    if (!_loop->isCurrent()) {
        weak_ptr<RtmpMediaSource> wSlef = static_pointer_cast<RtmpMediaSource>(shared_from_this());
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
        for (auto& track : _mapRtmpDecodeTrack) {
            track.second->stopDecode();
        }
    }
    _mapSource.clear();
}

void RtmpMediaSource::onFrame(const FrameBuffer::Ptr& frame)
{
    logInfo << "on get a frame: index : " << frame->getTrackIndex()
              << ", codec: " << frame->codec() << ", nalu type: " << frame->getNalType();
    auto it = _mapRtmpEncodeTrack.find(frame->getTrackType());
    if (it == _mapRtmpEncodeTrack.end()) {
        return ;
    }
    // logInfo << "on muxer a frame, pts: " << frame->pts();
    // FILE* fp = fopen("test2.264", "ab+");
    // fwrite(frame->_buffer.data(), 1, frame->_buffer.size(), fp);
    // fclose(fp);
    it->second->onFrame(frame);
}

void RtmpMediaSource::setMetadata(const AmfObjects& meta)
{
    lock_guard<mutex> lck(_mtxMeta);
    _metaData = meta;
}

AmfObjects RtmpMediaSource::getMetadata()
{
    lock_guard<mutex> lck(_mtxMeta);
    return _metaData;
}

int RtmpMediaSource::playerCount()
{
    int count = _ring->readerCount();
    lock_guard<mutex> lck(_mtxTrack);
    count -= _mapSink.size();

    return count;
}

void RtmpMediaSource::getClientList(const function<void(const list<ClientInfo>& info)>& func)
{
    list<ClientInfo> clientInfo;
    _ring->getInfoList([func](list<ClientInfo> &infoList){
        func(infoList);
    });
}