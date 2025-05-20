#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtspRtpTransport.h"
#include "Rtp/RtpPacket.h"
#include "Logger.h"
#include "Util/String.h"
#include "Rtp/Decoder/RtpDecodeH264.h"

using namespace std;

RtspRtpTransport::RtspRtpTransport(int transType, int dataType, const RtspTrack::Ptr& track, const Socket::Ptr& socket)
    :_transType(transType)
    ,_dataType(dataType)
    ,_socket(socket)
    ,_track(track)
{
    _index = track->getTrackIndex();
    logDebug << "_index : =========== " << _index;
}

void RtspRtpTransport::start() {
    RtspRtpTransport::Wptr wSelf = shared_from_this();
    // TODO get limits from config
    _sort = make_shared<RtpSort>(25);
    _sort->setOnRtpPacket([wSelf](const RtpPacket::Ptr& rtp){
        // logInfo << "decode rtp seq: " << rtp->getSeq() << ", rtp size: " << rtp->size() << ", rtp time: " << rtp->getStamp();
        auto self = wSelf.lock();
        if (self) {
            self->_track->onRtpPacket(rtp, false);
        }
    });
    _socket->setReadCb([wSelf](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
        // TODO 保存对端地址，udp发送时需要
        auto self = wSelf.lock();
        if (self) {
            self->onRtpPacket(buffer);
            self->bindPeerAddr(addr);
        }
        return 0;
    });
}

void RtspRtpTransport::bindPeerAddr(struct sockaddr* addr)
{
    if (_transType == Transport_UDP) {
        _socket->bindPeerAddr(addr);
    }
}

void RtspRtpTransport::onRtpPacket(const StreamBuffer::Ptr& buffer)
{
    if (buffer->size() <= 12) {
        // 可能是收到了打洞的包
        return;
    }
    if (_transType == Transport_TCP) {
        auto rtp = make_shared<RtpPacket>(buffer, 4);
        rtp->trackIndex_ = _index;
        auto data = rtp->data();
        data[0] = '$';
        data[1] = _track->getTrackIndex() * 2;
        data[2] = (rtp->size() - 4) >> 8;
        data[3] = (rtp->size() - 4) & 0x00FF;
        // logInfo << "recv rtp seq: " << rtp->getSeq() << ", rtp size: " << rtp->size() 
        //             << ", rtp time: " << rtp->getStamp() << ", index: " << _index;
        _track->onRtpPacket(rtp, false);
    } else if (_transType == Transport_UDP) {
        auto rtp = make_shared<RtpPacket>(buffer, 4);
        rtp->trackIndex_ = _index;
        auto data = rtp->data();
        data[0] = '$';
        data[1] = _track->getTrackIndex() * 2;
        data[2] = (rtp->size() - 4) >> 8;
        data[3] = (rtp->size() - 4) & 0x00FF;
        // logInfo << "recv rtp seq: " << rtp->getSeq() << ", rtp size: " << rtp->size() 
        //             << ", rtp time: " << rtp->getStamp() << ", index: " << _index;
        _sort->inputRtp(rtp);
    }
}

int RtspRtpTransport::sendRtpPacket(const RtpPacket::Ptr &packet, bool flag)
{
    // static int lastStamp = 0;
    int bytes = 0;
    switch (_transType) {
        case Transport_TCP: {
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
            // for (auto it = pkt->begin(); it != pkt->end(); ++it) {
                // lastStamp = packet->getStamp();
                // if (_target_play_track == TrackInvalid || _target_play_track == rtp->type) {
                //     updateRtcpContext(rtp);
                    // send(rtp);
                    // auto packet = it->get();
                    // auto data = packet->data();

                    // data[0] = '$';
                    // data[1] = _track->getInterleavedRtp();
                    // data[2] = (packet->size() - 4) >> 8;
                    // data[3] = (packet->size() - 4) & 0x00FF;

                    // StreamBuffer::Ptr tcpHeader = StreamBuffer::create();
                    // tcpHeader->assign((char*)payload_ptr, 4);
                    // _socket->send(tcpHeader, 0);

                    // logInfo << "send rtp seq: " << packet->getSeq() 
                    //         << ", rtp size: " << packet->size() 
                    //         << ", rtp time: " << packet->getStamp() 
                    //         << ", index: " << packet->trackIndex_
                    //         << ", start: " << RtpDecodeH264::isStartGop(*it);
                    // if (++i == len) {
                    //     _socket->send(packet->buffer(), 1);
                    // } else {
                        if (_tcpSend) {
                            _tcpSend(packet->buffer(), flag);
                        } else {
                            _socket->send(packet->buffer(), flag);
                        }
                        bytes += packet->size();
                        if (_rtcp) {
                            _rtcp->onSendRtp(packet);
                        }
                    // }
                    // if (++i == len) {
                    //     break;
                    // }
                // }
            // };
            // _socket->send((Buffer::Ptr)nullptr, 1);
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
        case Transport_UDP: {
            // setSendFlushFlag(false);
            // int i = 0;
            // int len = pkt->size() - 1;
            // auto pktlist = *(pkt.get());
            // for (auto& packet: pktlist) {
                // if (_target_play_track == TrackInvalid || _target_play_track == rtp->type) {
                //     updateRtcpContext(rtp);
                    // send(rtp);
                    // logInfo << "send rtp seq: " << packet->getSeq() << ", rtp size: " << packet->size() << ", rtp time: " << packet->getStamp();
                    // logInfo << "send rtp time: " << packet->getStamp() << ", mark:" << packet->getHeader()->mark;
                    
                    _socket->send(packet->buffer(), 1, 4);
                    bytes += packet->size();
                    if (_rtcp) {
                        _rtcp->onSendRtp(packet);
                    }
                    // if (++i == len) {
                    //     break;
                    // }
                // }
            // };
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

    return bytes;
}