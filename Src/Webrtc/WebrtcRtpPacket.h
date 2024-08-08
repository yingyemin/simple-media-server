#ifndef WebrtcRtpPacket_H
#define WebrtcRtpPacket_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"

using namespace std;

#define AudioLevelUrl "urn:ietf:params:rtp-hdrext:ssrc-audio-level"
#define AbsoluteSendTimeUrl "http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time"
#define TWCCUrl "http://www.ietf.org/id/draft-holmer-rmcat-transport-wide-cc-extensions-01"
#define MidUrl "urn:ietf:params:rtp-hdrext:sdes:mid"
#define RtpStreamIdUrl "urn:ietf:params:rtp-hdrext:sdes:rtp-stream-id"
#define RepairedRtpStreamIdUrl "urn:ietf:params:rtp-hdrext:sdes:repaired-rtp-stream-id"
#define VideoTimingUrl "http://www.webrtc.org/experiments/rtp-hdrext/video-timing"
#define ColorSpaceUrl "http://www.webrtc.org/experiments/rtp-hdrext/color-space"
#define CsrcAudioLevelUrl "urn:ietf:params:rtp-hdrext:csrc-audio-level"
#define FrameMarkingUrl "http://tools.ietf.org/html/draft-ietf-avtext-framemarking-07"
#define VideoContentTypeUrl "http://www.webrtc.org/experiments/rtp-hdrext/video-content-type"
#define PlayoutDelayUrl "http://www.webrtc.org/experiments/rtp-hdrext/playout-delay"
#define VideoRotationUrl "urn:3gpp:video-orientation"
#define TOffsetUrl "urn:ietf:params:rtp-hdrext:toffset"
#define Av1Url "https://aomediacodec.github.io/av1-rtp-spec/#dependency-descriptor-rtp-header-extension"
#define EncryptUrl "urn:ietf:params:rtp-hdrext:encryp"

enum RtpExtType
{
    kRtpExtensionNone,
    kRtpExtensionAudioLevel,
    kRtpExtensionAbsoluteSendTime,
    kRtpExtensionTWCC,
    kRtpExtensionMid,
    kRtpExtensionRtpStreamId,
    kRtpExtensionRepairedRtpStreamId,
    kRtpExtensionVideoTiming,
    kRtpExtensionColorSpace,
    kRtpExtensionCsrcAudioLevel,
    kRtpExtensionFrameMarking,
    kRtpExtensionVideoContentType,
    kRtpExtensionPlayoutDelay,
    kRtpExtensionVideoRotation,
    kRtpExtensionTOffset,
    kRtpExtensionAv1,
    kRtpExtensionEncrypt,

    kRtpExtensionTransportSequenceNumber,
    kRtpExtensionTransportSequenceNumber02,
    kRtpExtensionVideoLayersAllocation,
    kRtpExtensionGenericFrameDescriptor00,
    kRtpExtensionGenericFrameDescriptor = kRtpExtensionGenericFrameDescriptor00,
    kRtpExtensionGenericFrameDescriptor02,
    kRtpExtensionVideoFrameTrackingId,
    kRtpExtensionInbandComfortNoise,
    kRtpExtensionNumberOfExtensions
};

class RtpExtTypeMap
{
public:
    using Ptr = shared_ptr<RtpExtTypeMap>;

    void addId(int id, const string& uri);
    int getTypeById(int id);
    int getIdByType(int type);

private:
    unordered_map<int, int> _mapTypeToId;
    unordered_map<int, int> _mapIdToType;
};

class WebrtcRtpHeader {
public:
#if __BYTE_ORDER == __BIG_ENDIAN
    // 版本号，固定为2
    uint32_t version : 2;
    // padding
    uint32_t padding : 1;
    // 扩展
    uint32_t ext : 1;
    // csrc
    uint32_t csrc : 4;
    // mark
    uint32_t mark : 1;
    // 负载类型
    uint32_t pt : 7;
#else
    // csrc
    uint32_t csrc : 4;
    // 扩展
    uint32_t ext : 1;
    // padding
    uint32_t padding : 1;
    // 版本号，固定为2
    uint32_t version : 2;
    // 负载类型
    uint32_t pt : 7;
    // mark
    uint32_t mark : 1;
#endif
    // 序列号
    uint32_t seq : 16;
    // 时间戳
    uint32_t stamp;
    // ssrc
    uint32_t ssrc;
    // 负载，如果有csrc和ext，前面为 4 * csrc + (4 + 4 * ext_len)
    uint8_t payload;

public:
    // 返回csrc字段字节长度
    size_t getCsrcSize() const;
    // 返回csrc字段首地址，不存在时返回nullptr
    uint8_t *getCsrcData();

    // 返回ext字段字节长度
    size_t getExtSize() const;
    // 返回ext reserved值
    uint16_t getExtReserved() const;
    // 返回ext段首地址，不存在时返回nullptr
    uint8_t *getExtData();

    // 返回有效负载指针,跳过csrc、ext
    uint8_t *getPayloadData();
    // 返回有效负载总长度,不包括csrc、ext、padding
    ssize_t getPayloadSize(size_t rtp_size) const;
    // 打印调试信息
    // std::string dumpString(size_t rtp_size) const;

private:
    // 返回有效负载偏移量
    size_t getPayloadOffset() const;
    // 返回padding长度
    size_t getPaddingSize(size_t rtp_size) const;
};

// class WebrtcRtpExtOneByte
// {
// public:
//     virtual void decode();
//     virtual void encode();
// public:
//     int id;
//     int length;
//     uint8_t* value;
// };

class WebrtcRtpExt
{
public:
    using Ptr = shared_ptr<WebrtcRtpExt>;

    int getId() {return id_;}
    int getType() {return type_;}
    void setId(int id) {id_ = id;}
    void setType(int type) {type_ = type;}
    uint8_t* data() { return value_;}
    int size() const { return length_;}
    bool isOneByte() {return isOneByte_;}
    void setOneByte(bool flag) {isOneByte_ = flag;}
    string getUri() {return uri_;}

public:
    void checkValid(int type, int minSize) const;
    void getAudioLevel(int& vad, int level) const;
    uint32_t getAbsSendTime() const;
    uint16_t getTransportCCSeq()const;
    string getSdesMid()const;
    string getRtpStreamId()const;
    string getRepairedRtpStreamId()const;
    void getVideoTiming(uint8_t &flags,
                            uint16_t &encode_start,
                            uint16_t &encode_finish,
                            uint16_t &packetization_complete,
                            uint16_t &last_pkt_left_pacer,
                            uint16_t &reserved_net0,
                            uint16_t &reserved_net1)const;
    uint8_t getVideoContentType()const;
    void getVideoOrientation(bool &camera_bit, bool &flip_bit, bool &first_rotation, bool &second_rotation)const;
    void getPlayoutDelay(uint16_t &min_delay, uint16_t &max_delay)const;
    uint32_t getTransmissionOffset()const;
    uint8_t getFramemarkingTID()const;


public:
    bool isOneByte_ = true;
    int id_;
    int type_;
    int length_;
    uint8_t* value_;
    string uri_;
};

class WebrtcRtpPacket
{
public:
    using Ptr = shared_ptr<WebrtcRtpPacket>;
    enum { kRtpVersion = 2, kRtpHeaderSize = 12, kRtpTcpHeaderSize = 4 };

    WebrtcRtpPacket(const StreamBuffer::Ptr& buffer, int rtpOverTcpHeaderSize = 0);
    WebrtcRtpPacket(const int length, int rtpOverTcpHeaderSize = 0);

    void parse();
    void parseOneByteRtpExt();
    void parseTwoByteRtpExt();

    // static RtpPacket::Ptr create(const shared_ptr<TrackInfo>& trackInfo, int len, uint64_t pts, uint16_t seq, bool mark);

    // 获取rtp头
    WebrtcRtpHeader *getHeader();
    void setrtpExtTypeMap(const RtpExtTypeMap::Ptr& extType) {_rtpExtTypeMap = extType;}
    // const WebrtcRtpHeader *getHeader() const;

    // 打印调试信息
    // std::string dumpString() const;

    // 主机字节序的seq
    uint16_t getSeq();
    uint32_t getStamp();
    // 主机字节序的时间戳，已经转换为毫秒
    uint64_t getStampMS(bool ntp = false);
    // 主机字节序的ssrc
    uint32_t getSSRC();
    // 有效负载，跳过csrc、ext
    uint8_t *getPayload();
    // 有效负载长度，不包括csrc、ext、padding
    size_t getPayloadSize();
    char* data();
    size_t size();
    StreamBuffer::Ptr buffer();

    // 音视频类型
    string type_;
    // 音频为采样率，视频一般为90000
    uint32_t samplerate_;
    // ntp时间戳
    uint64_t ntpStamp_;

    int trackIndex_;

private:
    uint16_t _seq = 0;
    uint32_t _size = 0;
    int _rtpOverTcpHeaderSize = 0;
    WebrtcRtpHeader* _header;
    RtpExtTypeMap::Ptr _rtpExtTypeMap;
    StreamBuffer::Ptr _rtpOverTcpHeader;
    StreamBuffer::Ptr _data;
    unordered_map<int, WebrtcRtpExt::Ptr> _mapRtpExtOneByte;
    unordered_map<int, WebrtcRtpExt::Ptr> _mapRtpExtTwoByte;
};


#endif //WebrtcRtpPacket_H
