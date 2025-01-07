#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <sys/socket.h>

#include "RtpContext.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"
#include "Common/Config.h"
#include "Hook/MediaHook.h"

using namespace std;

RtpContext::RtpContext(const EventLoop::Ptr& loop, const string& uri, const string& vhost, const string& protocol, const string& type)
    :_uri(uri)
    ,_vhost(vhost)
    ,_protocol(protocol)
    ,_type(type)
    ,_loop(loop)
{

}

RtpContext::~RtpContext()
{
    logInfo << "~RtpContext";
    auto gbSrc = _source.lock();
    if (gbSrc) {
        // gbSrc->delOnDetach(this);
        gbSrc->release();
    }
}

bool RtpContext::init()
{
    UrlParser parser;
    parser.path_ = _uri;
    parser.vhost_ = _vhost;
    parser.protocol_ = _protocol;
    parser.type_ = _type;
    auto source = MediaSource::getOrCreate(_uri, _vhost
                    , _protocol, _type
                    , [this, parser](){
                        // if (_payloadType == "ps") {
                        //     return make_shared<RtpPsMediaSource>(parser, _loop);
                        // } else if (_payloadType == "ts") {
                        //     return make_shared<RtpTsMediaSource>(parser, _loop);
                        // } else {
                            auto source = make_shared<RtpMediaSource>(parser, _loop);
                            source->setPayloadType(_payloadType);

                            return source;
                        // }
                    });

    if (!source) {
        logWarn << "another stream is exist with the same uri";
        return false;
    }
    logInfo << "create a RtpMediaSource";
    auto gbSrc = dynamic_pointer_cast<RtpMediaSource>(source);
    if (!gbSrc) {
        logWarn << "source is not gb source";
        return false;
    }
    gbSrc->setOrigin();

    if (_payloadType == "ps" || _payloadType == "ts") {
        _videoTrack = make_shared<RtpDecodeTrack>(0, _payloadType);
        gbSrc->addTrack(_videoTrack);
    } else {
        if (_videoTrack) {
            gbSrc->addTrack(_videoTrack);
            gbSrc->addDecodeTrack(_videoTrack->getTrackInfo());
        }
        if (_audioTrack) {
            gbSrc->addTrack(_audioTrack);
            gbSrc->addDecodeTrack(_audioTrack->getTrackInfo());
        }
    }

    weak_ptr<RtpContext> wSelf = shared_from_this();
    gbSrc->addOnDetach(this, [wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        self->_alive = false;
    });
    _source = gbSrc;

    _loop->addTimerTask(5000, [wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return 0;
        }

        self->heartbeat();
        return 5000;
    }, [wSelf](bool success, shared_ptr<TimerTask>){

    });

    _sort = make_shared<RtpSort>(256);
    _sort->setOnRtpPacket([wSelf](const RtpPacket::Ptr& rtp){
        // logInfo << "decode rtp seq: " << rtp->getSeq() << ", rtp size: " << rtp->size() << ", rtp time: " << rtp->getStamp();
        auto self = wSelf.lock();
        if (self) {
            if (self->_payloadType == "ps" || self->_payloadType == "ts") {
                rtp->trackIndex_ = VideoTrackType;
                self->_videoTrack->onRtpPacket(rtp);
            } else {
                if (rtp->getHeader()->pt == 104 || rtp->getHeader()->pt == 8 || rtp->getHeader()->pt == 0) {
                    rtp->trackIndex_ = AudioTrackType;
                    self->_audioTrack->onRtpPacket(rtp);
                } else if (rtp->getHeader()->pt == 96 || rtp->getHeader()->pt == 32) {
                    rtp->trackIndex_ = VideoTrackType;
                    self->_videoTrack->onRtpPacket(rtp);
                } else {
                    logInfo << "unknown rtp payload type:" << (int)rtp->getHeader()->pt;
                }
            }
        }
    });

    _timeClock.start();
        
    PublishInfo info;
    info.protocol = _protocol;
    info.type = _type;
    info.uri = _uri;
    info.vhost = _vhost;

    MediaHook::instance()->onPublish(info, [wSelf](const PublishResponse &rsp){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        if (!rsp.authResult) {
            self->_alive = false;
        }
    });

    return true;
}

void RtpContext::onRtpPacket(const RtpPacket::Ptr& rtp, struct sockaddr* addr, int len, bool sort)
{
    if (rtp->getHeader()->version != 2) {
        logInfo << "version is invalid: " << (int)rtp->getHeader()->version;
        return ;
    }

    if (_ssrc < 0) {
        _ssrc = rtp->getSSRC();
    } else if (_ssrc != rtp->getSSRC()) {
        logInfo << "收到其他流的数据, recv ssrc: " << rtp->getSSRC() << ", cur _ssrc: " << _ssrc;
        return ;
    }

    if (addr) {
        if (!_addr) {
            _addr = make_shared<sockaddr>();
            memcpy(_addr.get(), addr, sizeof(sockaddr));
        } else if (memcmp(_addr.get(), addr, sizeof(struct sockaddr)) != 0) {
            // 记录一下这个流，提供切换流的api
            logInfo << "收到其他地址的流";
            return ;
        }
    }

    // logInfo << "rtp payload type:" << (int)rtp->getHeader()->pt;

    rtp->trackIndex_ = 0;
    
    if (sort) {
        _sort->onRtpPacket(rtp);
    } else {
        if (_payloadType == "ps" || _payloadType == "ts") {
            rtp->trackIndex_ = VideoTrackType;
            _videoTrack->onRtpPacket(rtp);
        } else {
            if (rtp->getHeader()->pt == 104 || rtp->getHeader()->pt == 8 || rtp->getHeader()->pt == 0) {
                rtp->trackIndex_ = AudioTrackType;
                if (_audioTrack) {
                    _audioTrack->onRtpPacket(rtp);
                }
            } else if (rtp->getHeader()->pt == 96 || rtp->getHeader()->pt == 32) {
                rtp->trackIndex_ = VideoTrackType;
                if (_videoTrack) {
                    _videoTrack->onRtpPacket(rtp);
                }
            } else {
                logInfo << "unknown rtp payload type:" << (int)rtp->getHeader()->pt;
            }
        }
    }

    _timeClock.update();
}

void RtpContext::heartbeat()
{
    // static int timeout = Config::instance()->getConfig()["Rtp"]["Server"]["timeout"];
    static int timeout = Config::instance()->getAndListen([](const json& config){
        timeout = config["Rtp"]["Server"]["timeout"];
        logInfo << "timeout: " << timeout;
    }, "Rtp", "Server", "timeout");

    if (timeout == 0) {
        timeout = 5000;
    }

    if (_timeClock.startToNow() > timeout) {
        logInfo << "alive is false";
        _alive = false;
    }
}

void RtpContext::setPayloadType(const string& payloadType)
{
    _payloadType = payloadType;
}

void RtpContext::createVideoTrack(const string& videoCodec)
{
    _videoTrack = make_shared<RtpDecodeTrack>(VideoTrackType, "raw");
    _videoTrack->createVideoTrack(videoCodec);
}

void RtpContext::createAudioTrack(const string& audioCodec, int channel, int sampleBit, int sampleRate)
{
    _audioTrack = make_shared<RtpDecodeTrack>(AudioTrackType, "raw");
    _audioTrack->createAudioTrack(audioCodec, channel, sampleBit, sampleRate);
}