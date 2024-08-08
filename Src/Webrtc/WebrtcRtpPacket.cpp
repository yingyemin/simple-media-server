#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <netinet/in.h>

#include "WebrtcRtpPacket.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

static unordered_map<int, string> mapTypeToUrl = {
    {kRtpExtensionAudioLevel,          AudioLevelUrl},
    {kRtpExtensionAbsoluteSendTime,    AbsoluteSendTimeUrl},
    {kRtpExtensionTWCC,                TWCCUrl},
    {kRtpExtensionMid,                 MidUrl},
    {kRtpExtensionRtpStreamId,         RtpStreamIdUrl},
    {kRtpExtensionRepairedRtpStreamId, RepairedRtpStreamIdUrl},
    {kRtpExtensionVideoTiming,         VideoTimingUrl},
    {kRtpExtensionColorSpace,          ColorSpaceUrl},
    {kRtpExtensionCsrcAudioLevel,      CsrcAudioLevelUrl},
    {kRtpExtensionFrameMarking,        FrameMarkingUrl},
    {kRtpExtensionVideoContentType,    VideoContentTypeUrl},
    {kRtpExtensionPlayoutDelay,        PlayoutDelayUrl},
    {kRtpExtensionVideoRotation,       VideoRotationUrl},
    {kRtpExtensionTOffset,             TOffsetUrl},
    {kRtpExtensionAv1,                 Av1Url},
    {kRtpExtensionEncrypt,             EncryptUrl}
};

static unordered_map<string, int> mapUrlToType = {
    {AudioLevelUrl,         kRtpExtensionAudioLevel},
    {AbsoluteSendTimeUrl,      kRtpExtensionAbsoluteSendTime},
    {TWCCUrl,       kRtpExtensionTWCC},
    {MidUrl,         kRtpExtensionMid},
    {RtpStreamIdUrl,       kRtpExtensionRtpStreamId},
    {RepairedRtpStreamIdUrl,      kRtpExtensionRepairedRtpStreamId},
    {VideoTimingUrl,       kRtpExtensionVideoTiming},
    {ColorSpaceUrl,        kRtpExtensionColorSpace},
    {CsrcAudioLevelUrl,         kRtpExtensionCsrcAudioLevel},
    {FrameMarkingUrl,        kRtpExtensionFrameMarking},
    {VideoContentTypeUrl,         kRtpExtensionVideoContentType},
    {PlayoutDelayUrl,      kRtpExtensionPlayoutDelay},
    {VideoRotationUrl,      kRtpExtensionVideoRotation},
    {TOffsetUrl,      kRtpExtensionTOffset},
    {Av1Url,         kRtpExtensionAv1},
    {EncryptUrl,       kRtpExtensionEncrypt}
};

#define AV_RB16(x) ((((const uint8_t *)(x))[0] << 8) | ((const uint8_t *)(x))[1])

void RtpExtTypeMap::addId(int id, const string& uri)
{
    if (id < 1 || id > 255) {
        return ;
    }

    if (mapUrlToType.find(uri) == mapUrlToType.end()) {
        return ;
    }

    _mapIdToType[id] = mapUrlToType[uri];
    _mapTypeToId[mapUrlToType[uri]] = id;
}

int RtpExtTypeMap::getTypeById(int id)
{
    if (_mapIdToType.find(id) != _mapIdToType.end()) {
        return _mapIdToType[id];
    }

    return kRtpExtensionNone;
}

int RtpExtTypeMap::getIdByType(int type)
{
    if (_mapTypeToId.find(type) != _mapTypeToId.end()) {
        return _mapTypeToId[type];
    }

    return 0;
}

////////////////////////WebrtcRtpExt/////////////////////////

void WebrtcRtpExt::checkValid(int inType, int minSize) const
{
    if (inType != type_ || size() < minSize) {
        throw "rtpext invalid";
    }
}

//https://tools.ietf.org/html/rfc6464
// 0                   1
//                    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5
//                   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//                   |  ID   | len=0 |V| level       |
//                   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//              Figure 1: Sample Audio Level Encoding Using the
//                          One-Byte Header Format
//
//
//      0                   1                   2                   3
//      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//     |      ID       |     len=1     |V|    level    |    0 (pad)    |
//     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//              Figure 2: Sample Audio Level Encoding Using the
//                          Two-Byte Header Format
void WebrtcRtpExt::getAudioLevel(int& vad, int level) const{
    checkValid(kRtpExtensionAudioLevel, 1);

    auto &byte = value_[0];
    vad = byte & 0x80;
    level = byte & 0x7F;
}

//http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time
//Wire format: 1-byte extension, 3 bytes of data. total 4 bytes extra per packet (plus shared 4 bytes for all extensions present: 2 byte magic word 0xBEDE, 2 byte # of extensions). Will in practice replace the “toffset” extension so we should see no long term increase in traffic as a result.
//
//Encoding: Timestamp is in seconds, 24 bit 6.18 fixed point, yielding 64s wraparound and 3.8us resolution (one increment for each 477 bytes going out on a 1Gbps interface).
//
//Relation to NTP timestamps: abs_send_time_24 = (ntp_timestamp_64 >> 14) & 0x00ffffff ; NTP timestamp is 32 bits for whole seconds, 32 bits fraction of second.
//
//Notes: Packets are time stamped when going out, preferably close to metal. Intermediate RTP relays (entities possibly altering the stream) should remove the extension or set its own timestamp.
uint32_t WebrtcRtpExt::getAbsSendTime() const {
    checkValid(kRtpExtensionAbsoluteSendTime, 3);
    uint32_t ret = 0;
    ret |= value_[0] << 16;
    ret |= value_[1] << 8;
    ret |= value_[2];
    return ret;
}

//https://tools.ietf.org/html/draft-holmer-rmcat-transport-wide-cc-extensions-01
//     0                   1                   2                   3
//      0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//     |       0xBE    |    0xDE       |           length=1            |
//     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//     |  ID   | L=1   |transport-wide sequence number | zero padding  |
//     +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
uint16_t WebrtcRtpExt::getTransportCCSeq() const {
    checkValid(kRtpExtensionTWCC, 2);
    uint16_t ret;
    ret = value_[0] << 8;
    ret |= value_[1];
    return ret;
}

//https://tools.ietf.org/html/draft-ietf-avtext-sdes-hdr-ext-07
//    0                   1                   2                   3
//    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//   |  ID   |  len  | SDES Item text value ...                      |
//   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
string WebrtcRtpExt::getSdesMid() const {
    checkValid(kRtpExtensionMid, 1);
    return string((char*)value_, length_);
}


//https://tools.ietf.org/html/draft-ietf-avtext-rid-06
//用于simulcast
//3.1.  RTCP 'RtpStreamId' SDES Extension
//
//        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |RtpStreamId=TBD|     length    | RtpStreamId                 ...
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//
//   The RtpStreamId payload is UTF-8 encoded and is not null-terminated.
//
//      RFC EDITOR NOTE: Please replace TBD with the assigned SDES
//      identifier value.

//3.2.  RTCP 'RepairedRtpStreamId' SDES Extension
//
//        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//       |Repaired...=TBD|     length    | RepairRtpStreamId           ...
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//
//
//   The RepairedRtpStreamId payload is UTF-8 encoded and is not null-
//   terminated.
//
//      RFC EDITOR NOTE: Please replace TBD with the assigned SDES
//      identifier value.

string WebrtcRtpExt::getRtpStreamId() const {
    checkValid(kRtpExtensionRtpStreamId, 1);
    return string((char*)value_, length_);
}

string WebrtcRtpExt::getRepairedRtpStreamId() const {
    checkValid(kRtpExtensionRepairedRtpStreamId, 1);
    return string((char*)value_, length_);
}


//http://www.webrtc.org/experiments/rtp-hdrext/video-timing
//Wire format: 1-byte extension, 13 bytes of data. Total 14 bytes extra per packet (plus 1-3 padding byte in some cases, plus shared 4 bytes for all extensions present: 2 byte magic word 0xBEDE, 2 byte # of extensions).
//
//First byte is a flags field. Defined flags:
//
//0x01 - extension is set due to timer.
//0x02 - extension is set because the frame is larger than usual.
//Both flags may be set at the same time. All remaining 6 bits are reserved and should be ignored.
//
//Next, 6 timestamps are stored as 16-bit values in big-endian order, representing delta from the capture time of a packet in ms. Timestamps are, in order:
//
//Encode start.
//Encode finish.
//Packetization complete.
//Last packet left the pacer.
//Reserved for network.
//Reserved for network (2).

void WebrtcRtpExt::getVideoTiming(uint8_t &flags,
                            uint16_t &encode_start,
                            uint16_t &encode_finish,
                            uint16_t &packetization_complete,
                            uint16_t &last_pkt_left_pacer,
                            uint16_t &reserved_net0,
                            uint16_t &reserved_net1) const {
    checkValid(kRtpExtensionVideoTiming, 13);
    flags = value_[0];
    encode_start = value_[1] << 8 | value_[2];
    encode_finish = value_[3] << 8 | value_[4];
    packetization_complete = value_[5] << 8 | value_[6];
    last_pkt_left_pacer = value_[7] << 8 | value_[8];
    reserved_net0 = value_[9] << 8 | value_[10];
    reserved_net1 = value_[11] << 8 | value_[12];
}


//http://www.webrtc.org/experiments/rtp-hdrext/color-space
//  0                   1                   2                   3
//  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |  ID   | L = 3 |   primaries   |   transfer    |    matrix     |
// +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
// |range+chr.sit. |
// +-+-+-+-+-+-+-+-+


//http://www.webrtc.org/experiments/rtp-hdrext/video-content-type
//Values:
//0x00: Unspecified. Default value. Treated the same as an absence of an extension.
//0x01: Screenshare. Video stream is of a screenshare type.
//0x02: 摄像头？
//Notes: Extension shoud be present only in the last packet of key-frames.
// If attached to other packets it should be ignored.
// If extension is absent, Unspecified value is assumed.
uint8_t WebrtcRtpExt::getVideoContentType() const {
    checkValid(kRtpExtensionVideoContentType, 1);
    return value_[0];
}

//http://www.3gpp.org/ftp/Specs/html-info/26114.htm
void WebrtcRtpExt::getVideoOrientation(bool &camera_bit, bool &flip_bit, bool &first_rotation, bool &second_rotation) const {
    checkValid(kRtpExtensionVideoRotation, 1);
    uint8_t byte = value_[0];
    camera_bit = (byte & 0x08) >> 3;
    flip_bit = (byte & 0x04) >> 2;
    first_rotation = (byte & 0x02) >> 1;
    second_rotation = byte & 0x01;
}

//http://www.webrtc.org/experiments/rtp-hdrext/playout-delay
// 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//|  ID   | len=2 |       MIN delay       |       MAX delay       |
//+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
void WebrtcRtpExt::getPlayoutDelay(uint16_t &min_delay, uint16_t &max_delay) const {
    checkValid(kRtpExtensionPlayoutDelay, 3);
    uint32_t bytes = value_[0] << 16 | value_[1] << 8 | value_[2];
    min_delay = (bytes & 0x00FFF000) >> 12;
    max_delay = bytes & 0x00000FFF;
}

//urn:ietf:params:rtp-hdrext:toffset
//https://tools.ietf.org/html/rfc5450
//       0                   1                   2                   3
//       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//      |  ID   | len=2 |              transmission offset              |
//      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
uint32_t WebrtcRtpExt::getTransmissionOffset() const {
    checkValid(kRtpExtensionTOffset, 3);
    return value_[0] << 16 | value_[1] << 8 | value_[2];
}

//http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07
//      0                   1                   2                   3
//	    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//	   |  ID=? |  L=2  |S|E|I|D|B| TID |   LID         |    TL0PICIDX  |
//	   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
uint8_t WebrtcRtpExt::getFramemarkingTID() const {
    checkValid(kRtpExtensionFrameMarking, 3);
    return value_[0] & 0x07;
}

/////////////////////////////////WebrtcRtpHeader/////////////////////////////

size_t WebrtcRtpHeader::getCsrcSize() const {
    // 每个csrc占用4字节
    return csrc << 2;
}

uint8_t *WebrtcRtpHeader::getCsrcData() {
    if (!csrc) {
        return nullptr;
    }
    return &payload;
}

size_t WebrtcRtpHeader::getExtSize() const {
    // rtp有ext
    if (!ext) {
        return 0;
    }
    auto ext_ptr = &payload + getCsrcSize();
    // uint16_t reserved = AV_RB16(ext_ptr);
    // 每个ext占用4字节
    return AV_RB16(ext_ptr + 2) << 2;
}

uint16_t WebrtcRtpHeader::getExtReserved() const {
    // rtp有ext
    if (!ext) {
        return 0;
    }
    auto ext_ptr = &payload + getCsrcSize();
    return AV_RB16(ext_ptr);
}

uint8_t *WebrtcRtpHeader::getExtData() {
    if (!ext) {
        return nullptr;
    }
    auto ext_ptr = &payload + getCsrcSize();
    // 多出的4个字节分别为reserved、ext_len
    return ext_ptr + 4;
}

size_t WebrtcRtpHeader::getPayloadOffset() const {
    // 有ext时，还需要忽略reserved、ext_len 4个字节
    return getCsrcSize() + (ext ? (4 + getExtSize()) : 0);
}

uint8_t *WebrtcRtpHeader::getPayloadData() {
    return &payload + getPayloadOffset();
}

size_t WebrtcRtpHeader::getPaddingSize(size_t rtp_size) const {
    if (!padding) {
        return 0;
    }
    auto end = (uint8_t *)this + rtp_size - 1;
    return *end;
}

ssize_t WebrtcRtpHeader::getPayloadSize(size_t rtp_size) const {
    auto invalid_size = getPayloadOffset() + getPaddingSize(rtp_size);
    return (ssize_t)rtp_size - invalid_size - WebrtcRtpPacket::kRtpHeaderSize;
}

// string WebrtcRtpHeader::dumpString(size_t rtp_size) const {
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

/////////////////////////////////WebrtcRtpPacket//////////////////////////////////////

WebrtcRtpPacket::WebrtcRtpPacket(const StreamBuffer::Ptr& buffer, int rtpOverTcpHeaderSize)
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
    _header = (WebrtcRtpHeader *)(data() + _rtpOverTcpHeaderSize);
}

WebrtcRtpPacket::WebrtcRtpPacket(const int length, int rtpOverTcpHeaderSize)
    :_rtpOverTcpHeaderSize(rtpOverTcpHeaderSize)
{
    _data = StreamBuffer::create();
    _size = length + rtpOverTcpHeaderSize;
    _data->setCapacity(_size + 1);
    _header = (WebrtcRtpHeader *)(data() + _rtpOverTcpHeaderSize);
}

// WebrtcRtpPacket::Ptr WebrtcRtpPacket::create(const shared_ptr<TrackInfo>& trackInfo, int len, uint64_t pts, uint16_t seq, bool mark)
// {
//     // StreamBuffer::Ptr buffer = StreamBuffer::create();
//     // buffer->setCapacity(len + 1);
//     WebrtcRtpPacket::Ptr rtp = make_shared<WebrtcRtpPacket>(len, 4);
//     WebrtcRtpHeader* header = rtp->getHeader();
//     rtp->type_ = trackInfo->trackType_;
//     rtp->trackIndex_ = trackInfo->index_;
//     // auto data = buffer->data();

//     // data[0] = '$';
//     // data[1] = trackInfo->index_ * 2;
//     // data[2] = (len - 4) >> 8;
//     // data[3] = (len - 4) & 0xFF;

//     header->version = WebrtcRtpPacket::kRtpVersion;
//     header->padding = 0;
//     header->ext = 0;
//     header->csrc = 0;
//     header->mark = mark;
//     header->pt = trackInfo->payloadType_;
//     header->seq = htons(seq);
//     header->stamp = htonl(uint64_t(pts) * trackInfo->samplerate_ / 1000);
//     header->ssrc = htonl(trackInfo->ssrc_);
//     rtp->ntpStamp_ = pts;
//     rtp->samplerate_ = trackInfo->samplerate_ ? trackInfo->samplerate_ : 90000;

//     return rtp;
// }

void WebrtcRtpPacket::parse()
{
    if (!_header->ext) {
        return ;
    }

    auto profieId = _header->getExtReserved();
    if (profieId == 0xBEDE) {
        parseOneByteRtpExt();
    } else if (profieId == 0x1000) {
        parseTwoByteRtpExt();
    }
}

void WebrtcRtpPacket::parseOneByteRtpExt()
{
    auto extData = _header->getExtData();
    auto extSize = _header->getExtSize();
    int index = 0;

    while (index < extSize) {
        auto head = extData[index];
        if (!head) {
            ++index;
            continue;
        }

        auto id = (extData[index]) >> 4;
        auto length = (extData[index]) & 0x0F + 1;

        index += 1;

        int type = _rtpExtTypeMap->getTypeById(id);
        auto rtpExt = make_shared<WebrtcRtpExt>();
        rtpExt->id_ = id;
        rtpExt->type_ = type;
        rtpExt->length_ = length;
        rtpExt->value_ = extData + index;

        index += length;

        _mapRtpExtOneByte[type] = rtpExt;
    }
}

void WebrtcRtpPacket::parseTwoByteRtpExt()
{
    auto extData = _header->getExtData();
    auto extSize = _header->getExtSize();
    int index = 0;

    while (index < extSize) {
        auto head = extData[index];
        if (!head) {
            ++index;
            continue;
        }

        auto id = extData[index];
        auto length = extData[index + 1];

        index += 2;

        int type = _rtpExtTypeMap->getTypeById(id);
        auto rtpExt = make_shared<WebrtcRtpExt>();
        rtpExt->id_ = id;
        rtpExt->type_ = type;
        rtpExt->length_ = length;
        rtpExt->value_ = extData + index;

        index += length;

        _mapRtpExtOneByte[type] = rtpExt;
    }
}

WebrtcRtpHeader *WebrtcRtpPacket::getHeader() {
    // 需除去rtcp over tcp 4个字节长度
    return _header;
}

// const WebrtcRtpHeader *WebrtcRtpPacket::getHeader() const {
//     return (WebrtcRtpHeader *)(data() + WebrtcRtpPacket::kRtpTcpHeaderSize);
// }

char* WebrtcRtpPacket::data()
{
    return _data->data();
}

StreamBuffer::Ptr WebrtcRtpPacket::buffer()
{
    return _data;
}

size_t WebrtcRtpPacket::size()
{
    if (_size != 0) {
        return _size;
    }
    return _data->size();
}

// string WebrtcRtpPacket::dumpString() const {
//     return ((WebrtcRtpPacket *)this)->getHeader()->dumpString(size() - WebrtcRtpPacket::kRtpTcpHeaderSize);
// }

uint16_t WebrtcRtpPacket::getSeq() {
    return ntohs(_header->seq);
}

uint32_t WebrtcRtpPacket::getStamp() {
    return ntohl(_header->stamp);
}

uint64_t WebrtcRtpPacket::getStampMS(bool ntp) {
    // logInfo << "getStamp(): " << getStamp();
    // logInfo << "samplerate_: " << samplerate_;
    return ntp ? ntpStamp_ : getStamp() * uint64_t(1000) / samplerate_;
}

uint32_t WebrtcRtpPacket::getSSRC() {
    return ntohl(_header->ssrc);
}

uint8_t *WebrtcRtpPacket::getPayload() {
    return _header->getPayloadData();
}

size_t WebrtcRtpPacket::getPayloadSize() {
    // 需除去rtcp over tcp 4个字节长度
    return _header->getPayloadSize(size() - _rtpOverTcpHeaderSize);
}

// WebrtcRtpPacket::Ptr WebrtcRtpPacket::create() {
// #if 0
//     static ResourcePool<WebrtcRtpPacket> packet_pool;
//     static onceToken token([]() {
//         packet_pool.setSize(1024);
//     });
//     auto ret = packet_pool.obtain2();
//     ret->setSize(0);
//     return ret;
// #else
//     return Ptr(new WebrtcRtpPacket);
// #endif
// }