#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "RtpPacket.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

#define AV_RB16(x) ((((const uint8_t *)(x))[0] << 8) | ((const uint8_t *)(x))[1])

size_t RtpHeader::getCsrcSize() const {
    // 每个csrc占用4字节
    return csrc << 2;
}

uint8_t *RtpHeader::getCsrcData() {
    if (!csrc) {
        return nullptr;
    }
    return &payload;
}

size_t RtpHeader::getExtSize() const {
    // rtp有ext
    if (!ext) {
        return 0;
    }
    auto ext_ptr = &payload + getCsrcSize();
    // uint16_t reserved = AV_RB16(ext_ptr);
    // 每个ext占用4字节
    return AV_RB16(ext_ptr + 2) << 2;
}

uint16_t RtpHeader::getExtReserved() const {
    // rtp有ext
    if (!ext) {
        return 0;
    }
    auto ext_ptr = &payload + getCsrcSize();
    return AV_RB16(ext_ptr);
}

uint8_t *RtpHeader::getExtData() {
    if (!ext) {
        return nullptr;
    }
    auto ext_ptr = &payload + getCsrcSize();
    // 多出的4个字节分别为reserved、ext_len
    return ext_ptr + 4;
}

size_t RtpHeader::getPayloadOffset() const {
    // 有ext时，还需要忽略reserved、ext_len 4个字节
    return getCsrcSize() + (ext ? (4 + getExtSize()) : 0);
}

uint8_t *RtpHeader::getPayloadData() {
    return &payload + getPayloadOffset();
}

size_t RtpHeader::getPaddingSize(size_t rtp_size) const {
    if (!padding) {
        return 0;
    }
    auto end = (uint8_t *)this + rtp_size - 1;
    return *end;
}

ssize_t RtpHeader::getPayloadSize(size_t rtp_size) const {
    auto invalid_size = getPayloadOffset() + getPaddingSize(rtp_size);
    return (ssize_t)rtp_size - invalid_size - RtpPacket::kRtpHeaderSize;
}

// string RtpHeader::dumpString(size_t rtp_size) const {
//     _StrPrinter printer;
//     printer << "version:" << (int)version << "\r\n";
//     printer << "padding:" << getPaddingSize(rtp_size) << "\r\n";
//     printer << "ext:" << getExtSize() << "\r\n";
//     printer << "csrc:" << getCsrcSize() << "\r\n";
//     printer << "mark:" << (int)mark << "\r\n";
//     printer << "pt:" << (int)pt << "\r\n";
//     printer << "seq:" << ntohs(seq) << "\r\n";
//     printer << "stamp:" << ntohl(stamp) << "\r\n";
//     printer << "ssrc:" << ntohl(ssrc) << "\r\n";
//     printer << "rtp size:" << rtp_size << "\r\n";
//     printer << "payload offset:" << getPayloadOffset() << "\r\n";
//     printer << "payload size:" << getPayloadSize(rtp_size) << "\r\n";
//     return std::move(printer);
// }

/////////////////////////////////RtpPacket//////////////////////////////////////

RtpPacket::RtpPacket(const StreamBuffer::Ptr& buffer, int rtpOverTcpHeaderSize)
    :_rtpOverTcpHeaderSize(rtpOverTcpHeaderSize)
{
    _data = StreamBuffer::create();
    _size = buffer->size() + _rtpOverTcpHeaderSize;
    if (_rtpOverTcpHeaderSize == 4) {
        char* rtpBuffer = new char[_size];
        memcpy(rtpBuffer + 4, buffer->data(), _size - 4);
        _data->move(rtpBuffer, _size);
    } else {
        // logInfo << "buffer size: " << buffer->size();
        _data->assign(buffer->data(), _size);
    }
    _header = (RtpHeader *)(data() + _rtpOverTcpHeaderSize);
}

RtpPacket::RtpPacket(const int length, int rtpOverTcpHeaderSize)
    :_rtpOverTcpHeaderSize(rtpOverTcpHeaderSize)
{
    _data = StreamBuffer::create();
    _size = length + rtpOverTcpHeaderSize;
    _data->setCapacity(_size + 1);
    _header = (RtpHeader *)(data() + _rtpOverTcpHeaderSize);
}

RtpPacket::Ptr RtpPacket::create(const shared_ptr<TrackInfo>& trackInfo, int len, uint64_t pts, uint16_t seq, bool mark)
{
    // StreamBuffer::Ptr buffer = StreamBuffer::create();
    // buffer->setCapacity(len + 1);
    RtpPacket::Ptr rtp = make_shared<RtpPacket>(len, 4);
    RtpHeader* header = rtp->getHeader();
    rtp->type_ = trackInfo->trackType_;
    rtp->trackIndex_ = trackInfo->index_;
    // auto data = buffer->data();

    // data[0] = '$';
    // data[1] = trackInfo->index_ * 2;
    // data[2] = (len - 4) >> 8;
    // data[3] = (len - 4) & 0xFF;

    header->version = RtpPacket::kRtpVersion;
    header->padding = 0;
    header->ext = 0;
    header->csrc = 0;
    header->mark = mark;
    header->pt = trackInfo->payloadType_;
    header->seq = htons(seq);
    header->stamp = htonl(uint64_t(pts) * trackInfo->samplerate_ / 1000);
    header->ssrc = htonl(trackInfo->ssrc_ + 1000);
    rtp->ntpStamp_ = pts;
    rtp->samplerate_ = trackInfo->samplerate_ ? trackInfo->samplerate_ : 90000;

    return rtp;
}

RtpHeader *RtpPacket::getHeader() {
    // 需除去rtcp over tcp 4个字节长度
    return _header;
}

// const RtpHeader *RtpPacket::getHeader() const {
//     return (RtpHeader *)(data() + RtpPacket::kRtpTcpHeaderSize);
// }

char* RtpPacket::data()
{
    return _data->data();
}

StreamBuffer::Ptr RtpPacket::buffer()
{
    return _data;
}

size_t RtpPacket::size()
{
    if (_size != 0) {
        return _size;
    }
    return _data->size();
}

// string RtpPacket::dumpString() const {
//     return ((RtpPacket *)this)->getHeader()->dumpString(size() - RtpPacket::kRtpTcpHeaderSize);
// }

uint16_t RtpPacket::getSeq() {
    return ntohs(_header->seq);
}

uint32_t RtpPacket::getStamp() {
    return ntohl(_header->stamp);
}

uint64_t RtpPacket::getStampMS(bool ntp) {
    // logInfo << "getStamp(): " << getStamp();
    // logInfo << "samplerate_: " << samplerate_;
    return ntp ? ntpStamp_ : getStamp() * uint64_t(1000) / samplerate_;
}

uint32_t RtpPacket::getSSRC() {
    return ntohl(_header->ssrc);
}

uint8_t *RtpPacket::getPayload() {
    return _header->getPayloadData();
}

size_t RtpPacket::getPayloadSize() {
    // 需除去rtcp over tcp 4个字节长度
    return _header->getPayloadSize(size() - _rtpOverTcpHeaderSize);
}

// RtpPacket::Ptr RtpPacket::create() {
// #if 0
//     static ResourcePool<RtpPacket> packet_pool;
//     static onceToken token([]() {
//         packet_pool.setSize(1024);
//     });
//     auto ret = packet_pool.obtain2();
//     ret->setSize(0);
//     return ret;
// #else
//     return Ptr(new RtpPacket);
// #endif
// }