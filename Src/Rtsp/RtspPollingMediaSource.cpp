#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtspPollingMediaSource.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "Common/Define.h"

using namespace std;

RtspPollingMediaSource::RtspPollingMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop, bool muxer)
    :RtspMediaSource(urlParser, loop)
    ,_muxer(muxer)
{
    _cache = std::make_shared<deque<RtpPacket::Ptr>>();
    _videoStampAdjust = make_shared<VideoStampAdjust>(0);
    _audioStampAdjust = make_shared<AudioStampAdjust>(0);
}

RtspPollingMediaSource::~RtspPollingMediaSource()
{
    logInfo << "~RtspPollingMediaSource";
}

void RtspPollingMediaSource::addTrack(const RtspTrack::Ptr& track)
{
    
}

void RtspPollingMediaSource::onReady()
{
    std::weak_ptr<RtspPollingMediaSource> weakSelf = std::static_pointer_cast<RtspPollingMediaSource>(shared_from_this());
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
        logDebug << "create _ring, path: " << _urlParser.path_;
        _ring = std::make_shared<QueType>(_ringSize, std::move(lam));
    }
    MediaSource::onReady();
    if (_mapSink.size() == 0 && _ring) {
        // _ring->delOnWrite(this);
        _probeFinish = true;
    }
}

void RtspPollingMediaSource::addTrack(const shared_ptr<TrackInfo>& track)
{
    if (_mapTrackInfo.find(track->index_) != _mapTrackInfo.end()) {
        return ;
    }
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
        logDebug << "sdp : " << _sdp;
    }
    int trackIndex = track->trackType_ == "video" ? VideoTrackType : AudioTrackType;
    string control = "trackID=" + to_string(trackIndex);
    addControl2Index(control, trackIndex);
    logDebug << "index: " << trackIndex << ", codec: " << track->codec_ << ", control: " << control << ", path: " << _urlParser.path_;
    std::weak_ptr<RtspPollingMediaSource> weakSelf = std::static_pointer_cast<RtspPollingMediaSource>(shared_from_this());
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
        logDebug << "create _ring : " << _ring << ", path: " << _urlParser.path_;;
    }
    auto rtspTrack = make_shared<RtspEncodeTrack>(trackIndex, track);
    {
        lock_guard<mutex> lck(_mtxTrack);
        logDebug << "add track, index: " << trackIndex << ", path: " << _urlParser.path_;;
        _mapRtspTrack.emplace(trackIndex, rtspTrack);
    }
    rtspTrack->setSsrc(trackIndex + 1000);
    rtspTrack->setEnableHuge(_enableHugeRtp);
    rtspTrack->setFastPts(_enableFastPts);
    if (_muxer) {
        rtspTrack->setOnRtpPacket([weakSelf](const RtpPacket::Ptr& rtp, bool start){
            // logInfo << "mux a rtp packet";
            auto strongSelf = weakSelf.lock();
            if (!strongSelf) {
                return;
            }

            if (!strongSelf->_hasVideo) {
                start = true;
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

RtspTrack::Ptr RtspPollingMediaSource::getTrack(int index)
{
    lock_guard<mutex> lck(_mtxTrack);
    auto it = _mapRtspTrack.find(index);
    if (it != _mapRtspTrack.end()) {
        return it->second;
    }

    return nullptr;
}


void RtspPollingMediaSource::addSink(const MediaSource::Ptr &src)
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
    weak_ptr<RtspPollingMediaSource> weakSelf = std::static_pointer_cast<RtspPollingMediaSource>(shared_from_this());
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

void RtspPollingMediaSource::delSink(const MediaSource::Ptr &src)
{
    logDebug << "RtspPollingMediaSource::delSink" << ", path: " << _urlParser.path_;
    if (!_loop->isCurrent()) {
        weak_ptr<RtspPollingMediaSource> wSlef = static_pointer_cast<RtspPollingMediaSource>(shared_from_this());
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

void RtspPollingMediaSource::onFrame(const FrameBuffer::Ptr& frame)
{
    // logInfo << "on get a frame: index : " << frame->getTrackType()
    //           << ", codec: " << frame->codec() << ", size: " << frame->size()
    //           << ", nal type: " << (int)frame->getNalType() << ", start size: " << frame->startSize();
    auto frameClone = FrameBuffer::createFrame(frame->codec(), frame->startSize(), frame->_index, false);
    frameClone->_pts = frame->pts();
    frameClone->_dts = frame->dts();
    frameClone->_buffer = frame->_buffer;
    frameClone->_index = frame->_index;
    frameClone->_startSize = frame->startSize();
    frameClone->_trackType = frame->getTrackType();
    if (frame->getTrackType() == VideoTrackType) {
        _videoStampAdjust->inputStamp(frameClone->_pts, frameClone->_dts, 1);
    } else if (frame->getTrackType() == AudioTrackType) {
        int samples = 0;
        if (frame->_codec == "aac") {
            samples = 1024;
        } else if (frame->_codec == "g711a" || frame->_codec == "g711u") {
            samples = frame->size() - frame->startSize();
        }
        _audioStampAdjust->inputStamp(frameClone->_pts, frameClone->_dts, samples);
    }

    auto it = _mapRtspTrack.find(frame->getTrackType());
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
    it->second->onFrame(frameClone);
}

void RtspPollingMediaSource::setSdp(const string& sdp)
{
    logDebug << "sdp: " << sdp << ", path: " << _urlParser.path_;;
    lock_guard<mutex> lck(_mtxSdp);
    _sdp = sdp;
}

string RtspPollingMediaSource::getSdp()
{
    lock_guard<mutex> lck(_mtxSdp);
    return _sdp;
}

int RtspPollingMediaSource::playerCount()
{
    if (!_ring) {
        return 0;
    }
    int count = _ring->readerCount();
    count -= _sinkSize;

    return count;
}

void RtspPollingMediaSource::getClientList(const function<void(const list<ClientInfo>& info)>& func)
{
    list<ClientInfo> clientInfo;
    if (!_ring) {
        func(clientInfo);
        return ;
    }
    _ring->getInfoList([func](list<ClientInfo> &infoList){
        func(infoList);
    });
}

void RtspPollingMediaSource::start(const function<void()>& onReady)
{
    if (_startPolling && onReady) {
        onReady();
        return ;
    }
    _startPolling = true;
    logInfo << "RtspPollingMediaSource::start, path: " << _urlParser.path_ << "is onready: " << (onReady ? "true" : "false");
    auto cb = shared_ptr<int>(new int(0), [onReady](int* p){delete p; if (onReady) onReady();});
    weak_ptr<RtspPollingMediaSource> wSelf = std::static_pointer_cast<RtspPollingMediaSource>(shared_from_this());
    // if (!_videoMediaSource) {
        int videoCount = _streamList.size();
        ++_streamIndex %= videoCount;
        _videoUrlParser = _streamList[_streamIndex];
        MediaSource::getOrCreateAsync(_videoUrlParser.path_, _videoUrlParser.vhost_, _videoUrlParser.protocol_, 
                                    _videoUrlParser.type_, [wSelf, cb, &videoCount](MediaSource::Ptr source){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }
            if (/*self->_videoMediaSource || */!source) {
                return ;
            }

            auto frameSrc = std::dynamic_pointer_cast<FrameMediaSource>(source);
            if (!frameSrc) {
                return ;
            }
            self->_videoMediaSource = frameSrc;
            self->_loop->async([wSelf, frameSrc, cb](){
                auto self = wSelf.lock();
                if (self) {
                    self->onSource(frameSrc);
                }
            }, true);
        }, [wSelf]() -> MediaSource::Ptr {
            auto self = wSelf.lock();
            if (!self) {
                return nullptr;
            }
            return make_shared<FrameMediaSource>(self->_videoUrlParser);
        }, this);
    // }

    _loop->addTimerTask(5000, [wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return 0;
        }

        self->start(nullptr);
        return 0;
    }, nullptr);
}

void RtspPollingMediaSource::onSource(const FrameMediaSource::Ptr& source)
{
    logDebug << "onSource, path: " << _urlParser.path_;
    auto trackInfo = source->getTrackInfo();
    if (trackInfo.find(VideoTrackType) == trackInfo.end()) {
        return ;
    }
    addTrack(trackInfo[VideoTrackType]);
    weak_ptr<RtspPollingMediaSource> wSelf = std::static_pointer_cast<RtspPollingMediaSource>(shared_from_this());
    _videoPlayReader = source->getRing()->attach(_loop, true);
    _videoPlayReader->setGetInfoCB([wSelf]() {
        auto self = wSelf.lock();
        ClientInfo ret;
        if (!self) {
            return ret;
        }
        // ret.ip_ = self->_socket->getPeerIp();
        // ret.port_ = self->_socket->getPeerPort();
        ret.protocol_ = PROTOCOL_FRAME;
        ret.createTime_ = self->getCreateTime();
        ret.close_ = [wSelf](){
            auto self = wSelf.lock();
            if (self) {
                self->release();
            }
        };
        // ret.bitrate_ = self->_lastBitrate;
        return ret;
    });
    _videoPlayReader->setDetachCB([wSelf]() {
        auto self = wSelf.lock();
        if (!self) {
            return;
        }
        // strong_self->shutdown(SockException(Err_shutdown, "rtsp ring buffer detached"));
        // self->release();
    });
    _videoPlayReader->setReadCB([wSelf](const MediaSource::FrameRingDataType &pack) {
        auto self = wSelf.lock();
        if (!self/* || pack->empty()*/) {
            return;
        }
        if (pack->_trackType != VideoTrackType) {
            return;
        }
        self->onFrame(pack);
    });
}
