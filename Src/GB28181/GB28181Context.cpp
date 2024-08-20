#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <sys/socket.h>

#include "GB28181Context.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"
#include "Common/Config.h"

using namespace std;

GB28181Context::GB28181Context(const EventLoop::Ptr& loop, const string& uri, const string& vhost, const string& protocol, const string& type)
    :_uri(uri)
    ,_vhost(vhost)
    ,_protocol(protocol)
    ,_type(type)
    ,_loop(loop)
{

}

GB28181Context::~GB28181Context()
{
    logInfo << "~GB28181Context";
    auto gbSrc = _source.lock();
    if (gbSrc) {
        // gbSrc->delOnDetach(this);
        gbSrc->release();
    }
}

bool GB28181Context::init()
{
    UrlParser parser;
    parser.path_ = _uri;
    parser.vhost_ = _vhost;
    parser.protocol_ = _protocol;
    parser.type_ = _type;
    auto source = MediaSource::getOrCreate(_uri, _vhost
                    , _protocol, _type
                    , [this, parser](){
                        return make_shared<GB28181MediaSource>(parser, _loop);
                    });

    if (!source) {
        logWarn << "another stream is exist with the same uri";
        return false;
    }
    logInfo << "create a GB28181MediaSource";
    auto gbSrc = dynamic_pointer_cast<GB28181MediaSource>(source);
    if (!gbSrc) {
        logWarn << "source is not gb source";
        return false;
    }
    gbSrc->setOrigin();

    if (_payloadType == "ps") {
        _videoTrack = make_shared<GB28181DecodeTrack>(0);
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

    weak_ptr<GB28181Context> wSelf = shared_from_this();
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
            return -1;
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
            if (rtp->getHeader()->pt == 104 || rtp->getHeader()->pt == 8 || rtp->getHeader()->pt == 0) {
                rtp->trackIndex_ = AudioTrackType;
                self->_audioTrack->onRtpPacket(rtp);
            } else if (rtp->getHeader()->pt == 96) {
                rtp->trackIndex_ = VideoTrackType;
                self->_videoTrack->onRtpPacket(rtp);
            }
        }
    });

    _timeClock.start();

    return true;
}

void GB28181Context::onRtpPacket(const RtpPacket::Ptr& rtp, struct sockaddr* addr, int len, bool sort)
{
    if (addr) {
        if (!_addr) {
            _addr = make_shared<sockaddr>();
            memcpy(_addr.get(), addr, sizeof(sockaddr));
        } else if (memcmp(_addr.get(), addr, sizeof(struct sockaddr)) != 0) {
            // 记录一下这个流，提供切换流的api
            return ;
        }
    }

    rtp->trackIndex_ = 0;
    
    if (sort) {
        _sort->onRtpPacket(rtp);
    } else {
        if (rtp->getHeader()->pt == 104 || rtp->getHeader()->pt == 8 || rtp->getHeader()->pt == 0) {
            rtp->trackIndex_ = AudioTrackType;
            _audioTrack->onRtpPacket(rtp);
        } else if (rtp->getHeader()->pt == 96) {
            rtp->trackIndex_ = VideoTrackType;
            _videoTrack->onRtpPacket(rtp);
        }
    }

    _timeClock.update();
}

void GB28181Context::heartbeat()
{
    // static int timeout = Config::instance()->getConfig()["GB28181"]["Server"]["timeout"];
    static int timeout = Config::instance()->getAndListen([](const json& config){
        timeout = config["GB28181"]["Server"]["timeout"];
        logInfo << "timeout: " << timeout;
    }, "GB28181", "Server", "timeout");

    if (_timeClock.startToNow() > timeout) {
        logInfo << "alive is false";
        _alive = false;
    }
}

void GB28181Context::setPayloadType(const string& payloadType)
{
    _payloadType = payloadType;
}

void GB28181Context::createVideoTrack(const string& videoCodec)
{
    _videoTrack = make_shared<GB28181DecodeTrack>(0, false);
    _videoTrack->createVideoTrack(videoCodec);
}

void GB28181Context::createAudioTrack(const string& audioCodec, int channel, int sampleBit, int sampleRate)
{
    _audioTrack = make_shared<GB28181DecodeTrack>(1, false);
    _audioTrack->createAudioTrack(audioCodec, channel, sampleBit, sampleRate);
}