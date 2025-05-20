#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <sys/time.h>
#include <unistd.h> 

#include "RtspRtcpTransport.h"
#include "RtspRtpTransport.h"
#include "Rtp/RtpPacket.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

RtspRtcpTransport::RtspRtcpTransport(int transType, int dataType, const RtspTrack::Ptr& track, const Socket::Ptr& socket)
    :_transType(transType)
    ,_dataType(dataType)
    ,_socket(socket)
    ,_track(track)
{
    if (track)
        _index = track->getTrackIndex();
}

void RtspRtcpTransport::start() {
    RtspRtcpTransport::Wptr wSelf = shared_from_this();
    _socket->setReadCb([wSelf](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
        auto self = wSelf.lock();
        if (self) {
            self->bindPeerAddr(addr);
            self->onRtcpPacket(buffer);
        }
        return 0;
    });
}

void RtspRtcpTransport::bindPeerAddr(struct sockaddr* addr)
{
    if (_transType == Transport_UDP) {
        _socket->bindPeerAddr(addr);
    }
}

void RtspRtcpTransport::onSendRtp(const RtpPacket::Ptr& rtp)
{
    _sendSize += rtp->size();
    _lastStamp = rtp->getStamp();
    _sendCount += 1;
}

void RtspRtcpTransport::onRtcpPacket(const StreamBuffer::Ptr& buffer) {
    sendRtcpPacket();
}

void RtspRtcpTransport::sendRtcpPacket()
{
    if (!_track) {
        return ;
    }

    static const char s_cname[] = "sms11";
    uint8_t rtcp_buf[4 + 28 + 10 + sizeof(s_cname) + 1] = {0};
    uint8_t *rtcp_sr = rtcp_buf + 4, *rtcp_sdes = rtcp_sr + 28;
    int rtcpLen = sizeof(rtcp_buf);
    // auto &track = _sdp_track[_index];
    // auto &counter = _rtcp_counter[_index];

    rtcp_buf[0] = '$';
    rtcp_buf[1] = _index * 2 + 1;
    rtcp_buf[2] = (sizeof(rtcp_buf) - 4) >> 8;
    rtcp_buf[3] = (sizeof(rtcp_buf) - 4) & 0xFF;

    // rtcpLen += 4;

    rtcp_sr[0] = 0x80;
    rtcp_sr[1] = 0xC8;
    rtcp_sr[2] = 0x00;
    rtcp_sr[3] = 0x06;

    uint32_t ssrc = htonl(_track->getSsrc());
    memcpy(&rtcp_sr[4], &ssrc, 4);

    uint64_t msw;
    uint64_t lsw;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    msw = tv.tv_sec + 0x83AA7E80; /* 0x83AA7E80 is the number of seconds from 1900 to 1970 */
    lsw = (uint32_t) ((double) tv.tv_usec * (double) (((uint64_t) 1) << 32) * 1.0e-6);

    msw = htonl(msw);
    memcpy(&rtcp_sr[8], &msw, 4);

    lsw = htonl(lsw);
    memcpy(&rtcp_sr[12], &lsw, 4);
    //直接使用网络字节序
    auto stamp = htonl(_lastStamp);
    memcpy(&rtcp_sr[16], &stamp, 4);

    uint32_t sendCount = htonl(_sendCount);
    memcpy(&rtcp_sr[20], &sendCount, 4);

    uint32_t octCount = htonl(_sendSize);
    memcpy(&rtcp_sr[24], &octCount, 4);

    // rtcpLen += 28;

    rtcp_sdes[0] = 0x81;
    rtcp_sdes[1] = 0xCA;
    rtcp_sdes[2] = 0x00;
    rtcp_sdes[3] = 0x03;

    memcpy(&rtcp_sdes[4], &ssrc, 4);

    rtcp_sdes[8] = 0x01;
    rtcp_sdes[9] = 0x05;
    memcpy(&rtcp_sdes[10], s_cname, sizeof(s_cname));
    rtcp_sdes[10 + sizeof(s_cname)] = 0x00;

    // rtcpLen += 10 + sizeof(s_cname);

    auto buffer = StreamBuffer::create();
    if (_transType == Transport_TCP) {
        buffer->assign((char*)rtcp_buf, rtcpLen);
        if (_tcpSend) {
            _tcpSend(buffer, true);
        } else {
            _socket->send(buffer, true);
        }
    } else {
        buffer->assign((char*)(rtcp_buf + 4), rtcpLen - 4);
        _socket->send(buffer, true);
    }
    
}