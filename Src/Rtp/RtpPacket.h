#ifndef RtpPacket_H
#define RtpPacket_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"

using namespace std;

class RtpHeader {
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

class RtpPacket
{
public:
    using Ptr = shared_ptr<RtpPacket>;
    enum { kRtpVersion = 2, kRtpHeaderSize = 12, kRtpTcpHeaderSize = 4 };

    RtpPacket(const StreamBuffer::Ptr& buffer, int rtpOverTcpHeaderSize = 0);
    RtpPacket(const int length, int rtpOverTcpHeaderSize = 0);

    static RtpPacket::Ptr create(const shared_ptr<TrackInfo>& trackInfo, int len, uint64_t pts, uint16_t seq, bool mark);

    virtual void parse() {}
    virtual void setRtxFlag(bool flag) {}
    // 获取rtp头
    virtual RtpHeader *getHeader();
    virtual void resetHeader() {}
    // const RtpHeader *getHeader() const;

    // 打印调试信息
    // std::string dumpString() const;

    // 主机字节序的seq
    virtual uint16_t getSeq();
    virtual uint32_t getStamp();
    // 主机字节序的时间戳，已经转换为毫秒
    virtual uint64_t getStampMS(bool ntp = false);
    // 主机字节序的ssrc
    virtual uint32_t getSSRC();
    // 有效负载，跳过csrc、ext
    virtual uint8_t *getPayload();
    // 有效负载长度，不包括csrc、ext、padding
    virtual size_t getPayloadSize();
    virtual char* data();
    virtual size_t size();
    virtual StreamBuffer::Ptr buffer();
    virtual int getStartSize() {return _rtpOverTcpHeaderSize;}

public:
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
    RtpHeader* _header;
    StreamBuffer::Ptr _rtpOverTcpHeader;
    StreamBuffer::Ptr _data;
};


#endif //RtpPacket_H
