#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtspMediaSource.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

RtspMediaSource::RtspMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop, bool muxer)
    :MediaSource(urlParser, loop)
    ,_muxer(muxer)
{
    _cache = std::make_shared<deque<RtpPacket::Ptr>>();
}

RtspMediaSource::~RtspMediaSource()
{
    logInfo << "~RtspMediaSource";
}

void RtspMediaSource::addTrack(const RtspTrack::Ptr& track)
{
    if (track->getTrackInfo()->trackType_ == "video") {
        _hasVideo = true;
    }
    std::weak_ptr<RtspMediaSource> weakSelf = std::static_pointer_cast<RtspMediaSource>(shared_from_this());
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
    {
        lock_guard<mutex> lck(_mtxTrack);
        _mapRtspTrack.emplace(track->getTrackIndex(), track);
    }

    // if (track->getTrackInfo()->trackType_ == "video") {
    //     _mapStampAdjust[track->getTrackIndex()] = make_shared<VideoStampAdjust>();
    // } else if (track->getTrackInfo()->trackType_ == "audio") {
    //     _mapStampAdjust[track->getTrackIndex()] = make_shared<AudioStampAdjust>();
    // }
    MediaSource::addTrack(track->getTrackInfo());
    
    track->setOnRtpPacket([weakSelf](const RtpPacket::Ptr& rtp, bool start){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        if (start) {
            strongSelf->_start = start;
        }
        strongSelf->_ring->addBytes(rtp->size());
        // logInfo << "on rtp seq: " << rtp->getSeq() << ", size: " << rtp->size() 
        //         << ", type: " << rtp->type_ << ", start: " << start
        //         << ", ssrc: " << rtp->getSSRC();
        if (rtp->getHeader()->mark || (!strongSelf->_hasVideo && strongSelf->_lastRtpStmp != rtp->getHeader()->stamp)) {
            strongSelf->_cache->emplace_back(std::move(rtp));
            // logInfo << "write cache size: " << strongSelf->_cache->size();
            strongSelf->_ring->write(strongSelf->_cache, strongSelf->_start);
            strongSelf->_cache = std::make_shared<deque<RtpPacket::Ptr>>();
            strongSelf->_start = false;
            
            if (strongSelf->_probeFinish) {
                if (strongSelf->_mapSink.empty()) {
                    strongSelf->_ring->delOnWrite(strongSelf.get());
        
                    for (auto& track : strongSelf->_mapRtspTrack) {
                        track.second->stopDecode();
                    }
                }
                strongSelf->_probeFinish = false;
            }
        } else {
            strongSelf->_cache->emplace_back(std::move(rtp));
        }
        strongSelf->_lastRtpStmp = rtp->getHeader()->stamp;
    });

    if (!_muxer) {
        track->setOnFrame([weakSelf](const FrameBuffer::Ptr& frame){
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return;
            }
            // int samples = 1;
            // if (frame->_trackType == AudioTrackType) {
            //     samples = frame->_buffer.size() - frame->startSize();
            // }
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
            track->startDecode();
            _ring->addOnWrite(this, [weakSelf](DataType in, bool is_key){
                auto strongSelf = weakSelf.lock();
                if (!strongSelf) {
                    return;
                }
                auto rtpList = *(in.get());
                for (auto& rtp : rtpList) {
                    int index = rtp->trackIndex_;
                    auto track = strongSelf->_mapRtspTrack.find(index);
                    if (track != strongSelf->_mapRtspTrack.end()) {
                        track->second->decodeRtp(rtp);
                    }
                }
            });
        }
        for (auto& sink : _mapSink) {
            // if (sink.second.lock()) {
            //     sink.second.lock()->addTrack(track->getTrackInfo());
            // }
            sink.second->addTrack(track->getTrackInfo());
        }
    }
}

void RtspMediaSource::onReady()
{
    MediaSource::onReady();
    if (_mapSink.size() == 0 && _ring) {
        // _ring->delOnWrite(this);
        _probeFinish = true;
    }
}

void RtspMediaSource::addTrack(const shared_ptr<TrackInfo>& track)
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
            << "a=control:*\r\n";

            _sdp = ss.str();
        }
        _sdp += track->getSdp();
        logInfo << "sdp : " << _sdp;
    }
    string control = "trackID=" + to_string(track->index_);
    addControl2Index(control, track->index_);
    logInfo << "index: " << track->index_ << ", codec: " << track->codec_ << ", control: " << control;
    std::weak_ptr<RtspMediaSource> weakSelf = std::static_pointer_cast<RtspMediaSource>(shared_from_this());
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
    auto rtspTrack = make_shared<RtspEncodeTrack>(track->index_, track);
    {
        lock_guard<mutex> lck(_mtxTrack);
        logInfo << "add track, index: " << track->index_;
        _mapRtspTrack.emplace(track->index_, rtspTrack);
    }
    rtspTrack->setSsrc(track->index_ + 1000);
    rtspTrack->setEnableHuge(_enableHugeRtp);
    if (_muxer) {
        rtspTrack->setOnRtpPacket([weakSelf](const RtpPacket::Ptr& rtp, bool start){
            // logInfo << "mux a rtp packet";
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return;
            }
            if (start) {
                strongSelf->_start = start;
            }
            // logInfo << "on rtp seq: " << rtp->getSeq() << ", size: " << rtp->size() << ", type: " << rtp->type_ << ", start: " << strongSelf->_start;
            if (rtp->getHeader()->mark) {
                strongSelf->_cache->emplace_back(std::move(rtp));
                strongSelf->_ring->write(strongSelf->_cache, strongSelf->_start);
                strongSelf->_cache = std::make_shared<deque<RtpPacket::Ptr>>();
                strongSelf->_start = false;
            } else {
                strongSelf->_cache->emplace_back(std::move(rtp));
            }
        });
        // if (_mapSink.size() > 0)
            rtspTrack->startEncode();
    }
}

RtspTrack::Ptr RtspMediaSource::getTrack(int index)
{
    lock_guard<mutex> lck(_mtxTrack);
    auto it = _mapRtspTrack.find(index);
    if (it != _mapRtspTrack.end()) {
        return it->second;
    }

    return nullptr;
}


void RtspMediaSource::addSink(const MediaSource::Ptr &src)
{
    MediaSource::addSink(src);
    
    for (auto& track : _mapRtspTrack) {
        src->addTrack(track.second->getTrackInfo());
    }
    if (_status == SourceStatus::AVAILABLE) {
        src->onReady();
    }
    if (_mapSink.size() == 1) {
        lock_guard<mutex> lck(_mtxTrack);
        for (auto& track : _mapRtspTrack) {
            track.second->startDecode();
        }
    }
    if (_ring->getOnWriteSize() > 0) {
        return ;
    }
    weak_ptr<RtspMediaSource> weakSelf = std::static_pointer_cast<RtspMediaSource>(shared_from_this());
    _ring->addOnWrite(src.get(), [weakSelf](DataType in, bool is_key){
        auto strongSelf = weakSelf.lock();
        if (!strongSelf) {
            return;
        }
        auto rtpList = *(in.get());
        for (auto& rtp : rtpList) {
            int index = rtp->trackIndex_;
            auto track = strongSelf->_mapRtspTrack.find(index);
            if (track != strongSelf->_mapRtspTrack.end()) {
                track->second->decodeRtp(rtp);
            }
        }
    });
}

void RtspMediaSource::delSink(const MediaSource::Ptr &src)
{
    logInfo << "RtspMediaSource::delSink";
    if (!_loop->isCurrent()) {
        weak_ptr<RtspMediaSource> wSlef = static_pointer_cast<RtspMediaSource>(shared_from_this());
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
        
        for (auto& track : _mapRtspTrack) {
            track.second->stopDecode();
        }
    }
    _mapSource.clear();
}

void RtspMediaSource::onFrame(const FrameBuffer::Ptr& frame)
{
    // logInfo << "on get a frame: index : " << frame->getTrackIndex()
    //           << ", codec: " << frame->codec() << ", size: " << frame->size()
    //           << ", nal type: " << (int)frame->getNalType() << ", start size: " << frame->startSize();
    auto it = _mapRtspTrack.find(frame->getTrackIndex());
    if (it == _mapRtspTrack.end()) {
        return ;
    }
    // logInfo << "on muxer a frame, pts: " << frame->pts();
    // if (frame->codec() == "h264") {
    //     static int i = 0;
    //     string name = "test" + to_string(++i) + ".264";
    //     FILE* fp = fopen(name.data(), "ab+");
    //     fwrite(frame->_buffer.data(), 1, frame->_buffer.size(), fp);
    //     fclose(fp);
    // }
    it->second->onFrame(frame);
}

void RtspMediaSource::setSdp(const string& sdp)
{
    logInfo << "sdp: " << sdp;
    lock_guard<mutex> lck(_mtxSdp);
    _sdp = sdp;
}

string RtspMediaSource::getSdp()
{
    lock_guard<mutex> lck(_mtxSdp);
    return _sdp;
}

int RtspMediaSource::playerCount()
{
    int count = _ring->readerCount();
    lock_guard<mutex> lck(_mtxTrack);
    count -= _mapSink.size();

    return count;
}

void RtspMediaSource::getClientList(const function<void(const list<ClientInfo>& info)>& func)
{
    list<ClientInfo> clientInfo;
    _ring->getInfoList([func](list<ClientInfo> &infoList){
        func(infoList);
    });
}