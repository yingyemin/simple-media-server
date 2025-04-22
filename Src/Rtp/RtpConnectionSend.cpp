#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "RtpConnectionSend.h"
#include "RtpManager.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"
#include "Common/HookManager.h"

using namespace std;

RtpConnectionSend::RtpConnectionSend(const EventLoop::Ptr& loop, const Socket::Ptr& socket)
    :TcpConnection(loop, socket)
    ,_loop(loop)
    ,_socket(socket)
{
    logInfo << "RtpConnectionSend";
    _transType = 1;
}

RtpConnectionSend::RtpConnectionSend(const EventLoop::Ptr& loop, const Socket::Ptr& socket, int transType)
    :TcpConnection(loop, socket)
    ,_loop(loop)
    ,_socket(socket)
{
    logInfo << "RtpConnectionSend";
    _transType = transType;
}

RtpConnectionSend::~RtpConnectionSend()
{
    logInfo << "~RtpConnectionSend";
    if (_task) {
        _task->quit = true;
    }
    auto gbSrc = _source.lock();
	if (gbSrc) {
		gbSrc->delConnection(this);
	}

    if (_playReader) {
        PlayerInfo info;
        info.ip = _socket->getPeerIp();
        info.port = _socket->getPeerPort();
        info.protocol = PROTOCOL_RTP;
        info.status = "off";
        info.type = _urlParser.type_;
        info.uri = _urlParser.path_;
        info.vhost = _urlParser.vhost_;

        auto hook = HookManager::instance()->getHook(MEDIA_HOOK);
        if (hook) {
            hook->onPlayer(info);
        }
    }
}

void RtpConnectionSend::init()
{
    weak_ptr<RtpConnectionSend> wSelf = dynamic_pointer_cast<RtpConnectionSend>(shared_from_this());
    // get source
    string uri = "/" + _app + "/" + _stream;
    MediaSource::getOrCreateAsync(uri, DEFAULT_VHOST, PROTOCOL_RTP, to_string(_ssrc), 
        [wSelf](const MediaSource::Ptr &src){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        if (!src) {
            self->close();
            return ;
        }
        auto source = dynamic_pointer_cast<RtpMediaSource>(src);
        if (source) {
            self->_source = source;
            self->_socket->getLoop()->async([wSelf](){
                auto self = wSelf.lock();
                if (!self) {
                    return ;
                }
                self->initReader();
            }, true);
        }        
    }, [uri, wSelf]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }
        // UrlParser parser;
        self->_urlParser.path_ = uri;
        self->_urlParser.vhost_ = DEFAULT_VHOST;
        self->_urlParser.protocol_ = PROTOCOL_RTP;
        self->_urlParser.type_ = DEFAULT_TYPE;
        auto source = make_shared<RtpMediaSource>(self->_urlParser, nullptr, true);
        source->setSsrc(self->_ssrc);
        source->setPayloadType(self->_payloadType);

        return source;
    }, this);

    if (_transType != 1) {
        _socket->getLoop()->addTimerTask(2000, [wSelf](){
            auto self = wSelf.lock();
            if (!self) {
                return 0;
            }
            self->onManager();

            return 2000;
        }, [wSelf](bool success, const shared_ptr<TimerTask>& task){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            self->_task = task;
        });
    }
}

void RtpConnectionSend::initReader()
{
    if (!_playReader) {
        logInfo << "set _playReader";
        auto gbSrc = _source.lock();
        if (!gbSrc) {
            close();
            return ;
        }
        weak_ptr<RtpConnectionSend> wSelf = dynamic_pointer_cast<RtpConnectionSend>(shared_from_this());
        _playReader = gbSrc->getRing()->attach(_socket->getLoop(), true);
        _playReader->setGetInfoCB([wSelf]() {
            auto self = wSelf.lock();
            ClientInfo ret;
            if (!self) {
                return ret;
            }
            ret.ip_ = self->_socket->getLocalIp();
            ret.port_ = self->_socket->getLocalPort();
            ret.protocol_ = PROTOCOL_RTP;
            return ret;
        });
        _playReader->setDetachCB([wSelf]() {
            auto strong_self = wSelf.lock();
            if (!strong_self) {
                return;
            }
            // // strong_self->shutdown(SockException(Err_shutdown, "rtsp ring buffer detached"));
            strong_self->close();
        });
        logInfo << "setReadCB =================";
        _playReader->setReadCB([wSelf](const RtpMediaSource::RingDataType &pack) {
            auto self = wSelf.lock();
            if (!self/* || pack->empty()*/) {
                return;
            }
            // logInfo << "send rtp packet";
            // auto pktList = *(pack.get());
            // for (auto& rtp : pktList) {
            //     logInfo << "send rtmp msg,time: " << pkt->abs_timestamp << ", type: " << (int)(pkt->type_id)
            //                 << ", length: " << pkt->length;
                self->sendRtpPacket(pack);
            // }
        });

        PlayerInfo info;
        info.ip = _socket->getPeerIp();
        info.port = _socket->getPeerPort();
        info.protocol = PROTOCOL_RTP;
        info.status = "on";
        info.type = _urlParser.type_;
        info.uri = _urlParser.path_;
        info.vhost = _urlParser.vhost_;

        auto hook = HookManager::instance()->getHook(MEDIA_HOOK);
        if (hook) {
            hook->onPlayer(info);
        }
    }
}

void RtpConnectionSend::close()
{
    logInfo << "RtpConnectionSend::close()";
    
    if (_transType == 1 || _transType == 3) {
        TcpConnection::close();
    } else {
        _socket->close();
    }

    logInfo << "RtpConnectionSend::close() _ondetach";
    if (_ondetach) {
        logInfo << "start _ondetach";
        _ondetach();
    }
}

void RtpConnectionSend::onManager()
{
    logInfo << "manager";
}

void RtpConnectionSend::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    // logInfo << "get a buf: " << buffer->size();
    if (!_addr) {
        _addr = make_shared<sockaddr>();
        memcpy(_addr.get(), addr, len);
    }
}

void RtpConnectionSend::onError(const string& msg)
{
    logWarn << "get a error: " << msg;
    close();
}

ssize_t RtpConnectionSend::send(Buffer::Ptr pkt)
{
    logInfo << "pkt size: " << pkt->size();
    if (_transType == 1 || _transType == 3) {
        return TcpConnection::send(pkt);
    } else {
        return _socket->send(pkt);
    }
}

void RtpConnectionSend::setMediaInfo(const string& app, const string& stream, int ssrc)
{
    _app = app;
    _stream = stream;
    _ssrc = ssrc;
}

void RtpConnectionSend::sendRtpPacket(const RtpMediaSource::RingDataType &pkt)
{
    logInfo << "RtpConnectionSend::sendRtpPacket";
    switch (_transType) {
        case 1: 
        case 3: {
            // setSendFlushFlag(false);
            // int i = 0;
            // int len = pkt->size();
            // auto pktlist = *(pkt.get());
            // if (lastStamp >= pktlist.back()->getStamp()) {
            //     logInfo << "time is non increasing";
            // }
            // lastStamp = pktlist.back()->getStamp();
            // logInfo << "send a frame rtp";

            // uint8_t payload_ptr[4];
            for (auto it = pkt->begin(); it != pkt->end(); ++it) {
                // lastStamp = packet->getStamp();
                // if (_target_play_track == TrackInvalid || _target_play_track == rtp->type) {
                //     updateRtcpContext(rtp);
                    // send(rtp);
                    auto packet = it->get();
                    auto data = packet->data();

                    if ((packet->type_ == "video" && _onlyTrack == "audio") || (packet->type_ == "audio" && _onlyTrack == "video")) {
                        continue;
                    }

                    // data[0] = '$';
                    // data[1] = _track->getInterleavedRtp();
                    // data[0] = (packet->size() - 4) >> 8;
                    // data[1] = (packet->size() - 4) & 0x00FF;

                    // StreamBuffer::Ptr tcpHeader = StreamBuffer::create();
                    // tcpHeader->assign((char*)payload_ptr, 4);
                    // _socket->send(tcpHeader, 0);

                    // logInfo << "send rtp seq: " << packet->getSeq() 
                    //         << ", rtp size: " << packet->size() 
                    //         << ", rtp time: " << packet->getStamp() 
                    //         << ", index: " << packet->trackIndex_;
                    // if (++i == len) {
                    //     _socket->send(packet->buffer(), 1);
                    // } else {
                        _socket->send(packet->buffer());
                    // }
                    // if (++i == len) {
                    //     break;
                    // }
                // }
            };
            _socket->send((Buffer::Ptr)nullptr, 1);
            // uint8_t payload_ptr[4];
            // payload_ptr[0] = '$';
            // payload_ptr[1] = 0;//pktlist.back()->trackIndex_*2;
            // payload_ptr[2] = pktlist.back()->size() >> 8;
            // payload_ptr[3] = (pktlist.back()->size() & 0x00FF);

            // StreamBuffer::Ptr tcpHeader = StreamBuffer::create();
            // tcpHeader->assign((char*)payload_ptr, 4);
            // _socket->send(tcpHeader, 1);

            // logInfo << "send rtp seq: " << pktlist.back()->getSeq() << ", rtp size: " << pktlist.back()->size() << ", rtp time: " << pktlist.back()->getStamp();
            // _socket->send(pktlist.back()->buffer(), 1);
            // lastStamp = pktlist.back()->getStamp();
            // flushAll();
            // setSendFlushFlag(true);
        }
            break;
        case 2:
        case 4: {
            // setSendFlushFlag(false);
            int i = 0;
            int len = pkt->size() - 1;
            auto pktlist = *(pkt.get());
            for (auto& packet: pktlist) {
                if ((packet->type_ == "video" && _onlyTrack == "audio") || (packet->type_ == "audio" && _onlyTrack == "video")) {
                    continue;
                }
                // if (_target_play_track == TrackInvalid || _target_play_track == rtp->type) {
                //     updateRtcpContext(rtp);
                    // send(rtp);
                    // logInfo << "send rtp seq: " << packet->getSeq() << ", rtp size: " << packet->size() << ", rtp time: " << packet->getStamp();
                    // logInfo << "send rtp time: " << packet->getStamp() << ", mark:" << packet->getHeader()->mark;
                    
                    _socket->send(packet->buffer(), 1, 2);
                    // if (++i == len) {
                    //     break;
                    // }
                // }
            };
            // // TODO udp发送需要指定对端地址
            // logInfo << "send rtp seq: " << pktlist.back()->getSeq() << ", rtp size: " << pktlist.back()->size() << ", rtp time: " << pktlist.back()->getStamp();
            // _socket->send(pktlist.back()->buffer(), 1);
            // flushAll();
            // setSendFlushFlag(true);
        }
            break;
        default:
            break;
    }
}