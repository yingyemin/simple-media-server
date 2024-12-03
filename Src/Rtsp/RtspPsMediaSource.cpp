#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtspPsMediaSource.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

RtspPsMediaSource::RtspPsMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop, bool muxer)
    :RtspMediaSource(urlParser, loop)
    ,_muxer(muxer)
{
    _cache = std::make_shared<deque<RtpPacket::Ptr>>();
}

RtspPsMediaSource::~RtspPsMediaSource()
{
    logInfo << "~RtspPsMediaSource";
}

void RtspPsMediaSource::addTrack(const RtspPsDecodeTrack::Ptr& track)
{
    std::weak_ptr<RtspPsMediaSource> weakSelf = std::static_pointer_cast<RtspPsMediaSource>(shared_from_this());
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
        _ring = std::make_shared<QueType>(_ringSize, std::move(lam));
        logInfo << "create _ring: " << _ring;
    }

    _psDecode = track;

    // if (track->getTrackInfo()->trackType_ == "video") {
    //     _mapStampAdjust[track->getTrackIndex()] = make_shared<VideoStampAdjust>();
    // } else if (track->getTrackInfo()->trackType_ == "audio") {
    //     _mapStampAdjust[track->getTrackIndex()] = make_shared<AudioStampAdjust>();
    // }
    // MediaSource::addTrack(track->getTrackInfo());
    
    _psDecode->setOnRtpPacket([weakSelf](const RtpPacket::Ptr& rtp, bool start){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        if (start) {
            strongSelf->_start = start;
        }
        strongSelf->_ring->addBytes(rtp->size());
        // logInfo << "on rtp seq: " << rtp->getSeq() << ", size: " << rtp->size() << ", type: " << rtp->type_;
        if (rtp->getHeader()->mark) {
            strongSelf->_cache->emplace_back(std::move(rtp));
            // logInfo << "write cache size: " << strongSelf->_cache->size();
            strongSelf->_ring->write(strongSelf->_cache, strongSelf->_start);
            strongSelf->_cache = std::make_shared<deque<RtpPacket::Ptr>>();
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
    });

    _psDecode->setOnTrackInfo([weakSelf](const std::shared_ptr<TrackInfo> &trackInfo){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        strongSelf->addDecodeTrack(trackInfo);
    });

    _psDecode->setOnReady([weakSelf](){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        strongSelf->onReady();
    });

    if (!_muxer) {
        _psDecode->setOnFrame([weakSelf](const FrameBuffer::Ptr& frame){
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return;
            }
            int samples = 1;
            if (frame->_trackType == AudioTrackType) {
                samples = frame->_buffer.size() - frame->startSize();
            }
            // strongSelf->_mapStampAdjust[frame->_index]->inputStamp(frame->_pts, frame->_dts, samples);
            // logInfo << "on frame";
            strongSelf->MediaSource::onFrame(frame);
            for (auto& sink : strongSelf->_mapSink) {
                // logInfo << "on frame to sink";
                // if (sink.second.lock()) {
                //     sink.second.lock()->onFrame(frame);
                // }
                sink.second->onFrame(frame);
            }
        });
        if (_status < SourceStatus::AVAILABLE || _mapSink.size() > 0) {
            _psDecode->startDecode();
            _ring->addOnWrite(this, [weakSelf](DataType in, bool is_key){
                auto strongSelf = weakSelf.lock();
                if (!strongSelf) {
                    return;
                }
                auto rtpList = *(in.get());
                for (auto& rtp : rtpList) {
                    int index = rtp->trackIndex_;
                    
                    auto track = strongSelf->_psDecode;
                    track->decodeRtp(rtp);
                }
            });
        }
    }
}

void RtspPsMediaSource::onReady()
{
    MediaSource::onReady();
    if (_muxer) {
        std::weak_ptr<RtspPsMediaSource> weakSelf = std::static_pointer_cast<RtspPsMediaSource>(shared_from_this());
        _psEncode->setOnRtpPacket([weakSelf](const RtpPacket::Ptr& rtp, bool start){
            // logInfo << "mux a rtp packet";
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return;
            }
            if (rtp->getHeader()->mark) {
                // logInfo << "mux a rtp packet mark";
                strongSelf->_cache->emplace_back(std::move(rtp));
                strongSelf->_ring->write(strongSelf->_cache);
                strongSelf->_cache = std::make_shared<deque<RtpPacket::Ptr>>();
                strongSelf->_start = false;
            } else {
                // logInfo << "mux a rtp packet no mark";
                strongSelf->_cache->emplace_back(std::move(rtp));
            }
            if (start) {
                strongSelf->_start = true;
            }
        });
        // if (_mapSink.size() > 0)
            _psEncode->startEncode();
    } else {
        if (_mapSink.size() == 0 && _ring) {
            _probeFinish = true;
            
            // for (auto& track : _mapGB28181DecodeTrack) {
            //     track.second->stopDecode();
            // }
            _psDecode->stopDecode();
        }
    }
}

void RtspPsMediaSource::addTrack(const shared_ptr<TrackInfo>& track)
{
    MediaSource::addTrack(track);
    {
        lock_guard<mutex> lck(_mtxSdp);
        if (_sdp.empty()) {
            std::stringstream ss;
            ss << "v=0\r\n"
            << "o=- 0 0 IN IP4 0.0.0.0\r\n"
            << "s=Streamed by " << "SimpleMediaServer" << "\r\n"
            << "c=IN IP4 0.0.0.0\r\n"
            << "t=0 0\r\n"
            //    暂时不考虑点播
            << "a=range:npt=now-\r\n"
            << "a=control:*\r\n"
            << "m=video 0 RTP/AVP 96\r\n"
            << "a=rtpmap:96 MP2P/90000\r\n"
            << "a=control:trackID=" << track->index_ << "\r\n";

            _sdp = ss.str();
            logInfo << "sdp : " << _sdp;
        }
    }
    string control = "trackID=" + to_string(track->index_);
    addControl2Index(control, track->index_);
    logInfo << "index: " << track->index_ << ", codec: " << track->codec_ << ", control: " << control;
    std::weak_ptr<RtspPsMediaSource> weakSelf = std::static_pointer_cast<RtspPsMediaSource>(shared_from_this());
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
        _ring = std::make_shared<QueType>(_ringSize, std::move(lam));
        logInfo << "create _ring : " << _ring;
    }
    if (!_psEncode) {
        _psEncode = make_shared<RtspPsEncodeTrack>(0);
    }

    _psEncode->addTrackInfo(track);
}

void RtspPsMediaSource::addDecodeTrack(const shared_ptr<TrackInfo>& track)
{
    logInfo << "addDecodeTrack ========= ";
    if (track->trackType_ == "video") {
        _mapStampAdjust[track->index_] = make_shared<VideoStampAdjust>();
    } else if (track->trackType_ == "audio") {
        _mapStampAdjust[track->index_] = make_shared<AudioStampAdjust>();
    }
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

RtspTrack::Ptr RtspPsMediaSource::getTrack(int index)
{
    if (_muxer) {
        return _psEncode;
    } else {
        return _psDecode;
    }
}


void RtspPsMediaSource::addSink(const MediaSource::Ptr &src)
{
    MediaSource::addSink(src);
    
    for (auto& track : _mapTrackInfo) {
        src->addTrack(track.second);
    }
    if (_status == SourceStatus::AVAILABLE) {
        src->onReady();
    }
    if (_mapSink.size() == 1) {
        lock_guard<mutex> lck(_mtxTrack);
        // for (auto& track : _mapRtspTrack) {
            _psDecode->startDecode();
        // }
    }
    // if (_ring->getOnWriteSize() > 0) {
    //     return ;
    // }
    weak_ptr<RtspPsMediaSource> weakSelf = std::static_pointer_cast<RtspPsMediaSource>(shared_from_this());
    _ring->addOnWrite(this, [weakSelf](DataType in, bool is_key){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        auto rtpList = *(in.get());
        for (auto& rtp : rtpList) {
            // int index = rtp->trackIndex_;
            // auto track = strongSelf->_mapRtspTrack.find(index);
            // if (track != strongSelf->_mapRtspTrack.end()) {
                strongSelf->_psDecode->decodeRtp(rtp);
            // }
        }
    });
}

void RtspPsMediaSource::delSink(const MediaSource::Ptr &src)
{
    logInfo << "RtspPsMediaSource::delSink";
    if (!_loop->isCurrent()) {
        weak_ptr<RtspPsMediaSource> wSlef = static_pointer_cast<RtspPsMediaSource>(shared_from_this());
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
        // for (auto& track : _mapRtspTrack) {
            _psDecode->stopDecode();
        // }
    }
    _mapSource.clear();
}

void RtspPsMediaSource::onFrame(const FrameBuffer::Ptr& frame)
{
    // logInfo << "on get a frame: index : " << frame->getTrackIndex()
    //           << ", codec: " << frame->codec() << ", size: " << frame->size()
    //           << ", nal type: " << (int)frame->getNalType() << ", start size: " << frame->startSize();
    if (!_psEncode) {
        return ;
    }
    // logInfo << "on muxer a frame";
    _psEncode->onFrame(frame);
}

void RtspPsMediaSource::setSdp(const string& sdp)
{
    logInfo << "sdp: " << sdp;
    lock_guard<mutex> lck(_mtxSdp);
    _sdp = sdp;
}

string RtspPsMediaSource::getSdp()
{
    lock_guard<mutex> lck(_mtxSdp);
    return _sdp;
}

int RtspPsMediaSource::playerCount()
{
    int count = _ring->readerCount();
    lock_guard<mutex> lck(_mtxTrack);
    count -= _mapSink.size();

    return count;
}

void RtspPsMediaSource::getClientList(const function<void(const list<ClientInfo>& info)>& func)
{
    list<ClientInfo> clientInfo;
    _ring->getInfoList([func](list<ClientInfo> &infoList){
        func(infoList);
    });
}