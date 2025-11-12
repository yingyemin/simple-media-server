#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#ifndef _WIN32
#include <arpa/inet.h>
#endif

#include "GB28181ConnectionSend.h"
#include "GB28181Manager.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "Common/Define.h"
#include "Common/HookManager.h"

using namespace std;

GB28181ConnectionSend::GB28181ConnectionSend(const EventLoop::Ptr& loop, const Socket::Ptr& socket)
    :TcpConnection(loop, socket)
    ,_loop(loop)
    ,_socket(socket)
{
    logInfo << "GB28181ConnectionSend";
    _transType = 1;
}

GB28181ConnectionSend::GB28181ConnectionSend(const EventLoop::Ptr& loop, const Socket::Ptr& socket, int transType)
    :TcpConnection(loop, socket)
    ,_loop(loop)
    ,_socket(socket)
{
    logInfo << "GB28181ConnectionSend";
    _transType = transType;
}

GB28181ConnectionSend::~GB28181ConnectionSend()
{
    logInfo << "~GB28181ConnectionSend";
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
        info.protocol = PROTOCOL_GB28181;
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

void GB28181ConnectionSend::init()
{
    weak_ptr<GB28181ConnectionSend> wSelf = dynamic_pointer_cast<GB28181ConnectionSend>(shared_from_this());
    // get source
    string uri = "/" + _app + "/" + _stream;
    MediaSource::getOrCreateAsync(uri, DEFAULT_VHOST, PROTOCOL_GB28181, to_string(_ssrc), 
        [wSelf](const MediaSource::Ptr &src){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        if (!src) {
            self->close();
            return ;
        }
        auto source = dynamic_pointer_cast<GB28181MediaSource>(src);
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
        self->_urlParser.protocol_ = PROTOCOL_GB28181;
        self->_urlParser.type_ = DEFAULT_TYPE;
        auto source = make_shared<GB28181MediaSource>(self->_urlParser, nullptr, true);
        source->setSsrc(self->_ssrc);

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

void GB28181ConnectionSend::initReader()
{
    if (!_playReader) {
        logInfo << "set _playReader";
        auto gbSrc = _source.lock();
        if (!gbSrc) {
            close();
            return ;
        }
        weak_ptr<GB28181ConnectionSend> wSelf = dynamic_pointer_cast<GB28181ConnectionSend>(shared_from_this());
        _playReader = gbSrc->getRing()->attach(_socket->getLoop(), true);
        _playReader->setGetInfoCB([wSelf]() {
            auto self = wSelf.lock();
            ClientInfo ret;
            if (!self) {
                return ret;
            }
            ret.ip_ = self->_socket->getLocalIp();
            ret.port_ = self->_socket->getLocalPort();
            ret.protocol_ = PROTOCOL_GB28181;
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
        _playReader->setReadCB([wSelf](const GB28181MediaSource::RingDataType &pack) {
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
        info.protocol = PROTOCOL_GB28181;
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

void GB28181ConnectionSend::close()
{
    logInfo << "GB28181ConnectionSend::close()";
    
    if (_transType == 1 || _transType == 3) {
        TcpConnection::close();
    } else {
        _socket->close();
    }

    logInfo << "GB28181ConnectionSend::close() _ondetach";
    if (_ondetach) {
        logInfo << "start _ondetach";
        _ondetach();
    }
}

void GB28181ConnectionSend::onManager()
{
    logInfo << "manager";
}

void GB28181ConnectionSend::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    // logInfo << "get a buf: " << buffer->size();
    if (!_addr) {
        _addr = make_shared<sockaddr>();
        memcpy(_addr.get(), addr, len);
    }
}

void GB28181ConnectionSend::onError(const string& msg)
{
    logWarn << "get a error: " << msg;
    close();
}

ssize_t GB28181ConnectionSend::send(Buffer::Ptr pkt)
{
    logInfo << "pkt size: " << pkt->size();
    if (_transType == 1 || _transType == 3) {
        return TcpConnection::send(pkt);
    } else {
        return _socket->send(pkt);
    }
}

void GB28181ConnectionSend::setMediaInfo(const string& app, const string& stream, int ssrc)
{
    _app = app;
    _stream = stream;
    _ssrc = ssrc;
}

void GB28181ConnectionSend::sendRtpPacket(const GB28181MediaSource::RingDataType &pkt)
{
    logInfo << "GB28181ConnectionSend::sendRtpPacket";
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
