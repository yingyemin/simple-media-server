#ifndef WebrtcRtcpPacket_H
#define WebrtcRtcpPacket_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"

using namespace std;

enum RtcpType
{
    RtcpType_INVALID,
    RtcpType_SR = 200,
    RtcpType_RR,
    RtcpType_SDES,
    RtcpType_BYE,
    RtcpType_APP,
    RtcpType_RTPFB,
    RtcpType_PSFB,
    RtcpType_XR
};

enum RtcpRtpFBFmt
{
    RtcpRtpFBFmt_NACK = 1,
    RtcpRtpFBFmt_TMMBR,
    RtcpRtpFBFmt_TMMBN,
    RtcpRtpFBFmt_SR_REQ,
    RtcpRtpFBFmt_RAMS,
    RtcpRtpFBFmt_ECN,
    RtcpRtpFBFmt_PS,
    RtcpRtpFBFmt_TWCC = 15
};

enum RtcpPSFBFmt
{
    RtcpPSFBFmt_INVALID,
    RtcpPSFBFmt_PLI,
    RtcpPSFBFmt_SLI,
    RtcpPSFBFmt_RPSI,
    RtcpPSFBFmt_FIR,
    RtcpPSFBFmt_REMB= 15
};

class RtcpHeader
{
public:
#if __BYTE_ORDER == __BIG_ENDIAN
    uint8_t version : 2;
    uint8_t padding : 1;
    uint8_t rc :5;
#else
    uint8_t rc :5;
    uint8_t padding : 1;
    uint8_t version : 2;
#endif
    uint8_t type:8;
	uint16_t length:16;
    uint32_t ssrc : 32;
};

class RtcpPacket
{
public:
    using Ptr = shared_ptr<RtcpPacket>;
    RtcpPacket(const StreamBuffer::Ptr& buffer, int pos);
    RtcpPacket() {}

public:
    virtual size_t size() {return _length;}
    virtual char* data() {return _buffer->data() + _pos;};
    virtual void parse();

    void setOnRtcp(const function<void(const RtcpPacket::Ptr& rtcp)>& cb) {_onRtcp = cb;}
    void onRtcp(const RtcpPacket::Ptr& rtcp);
    RtcpHeader* getHeader() {return _header;}

protected:
    RtcpHeader* _header;
    StreamBuffer::Ptr _buffer;
    char* _payload = nullptr;
    int _payloadLen = 0;
    int _pos = 0;
    int _length = 0;
    function<void(const RtcpPacket::Ptr& rtcp)> _onRtcp;
};

class RtcpSR : public RtcpPacket
{
public:
    RtcpSR(const StreamBuffer::Ptr& buffer, int pos);

public:
    void parse();

private:
    // ntp 时间
    uint64_t _ntp;
    // rtp 时间
    uint32_t _rtpTs;
    // 发包数
    uint32_t _sendPackets;
    // 字节数
    uint32_t _sendBytes;
};

class RtcpRRBlock
{
public:
    // 对应的ssrc
    uint32_t _ssrc;
    // 丢包率：（期望包数-实际包数）/期望包数 * 255
    uint8_t  _fractionLost;
    // 累计丢包数
    uint32_t _lostPackets;
    // 收到的最大包序列，高16位表示循环数，低16位为最大seq
    uint32_t _highestSn;
    // 数据包到达的抖动，反应网络情况
    uint32_t _jitter;
    // 上一个sr包的ntp时间，只有中间32位，sr包里的是64位
    uint32_t _lsr;
    // 上一次收到sr包到现在的时间,单位1/65536秒
    uint32_t _dlsr;
};

class RtcpRR : public RtcpPacket
{
public:
    RtcpRR(const StreamBuffer::Ptr& buffer, int pos);

public:
    void parse();

private:
    vector<RtcpRRBlock> _reportBlocks;
};

class RtcpXrDlsrBlock
{
public:
    uint32_t ssrc_;
    uint32_t lrr_;
    uint32_t dlrr_;
};

class RtcpXrRrtBlock
{
public:
    int blockType_ = 4;
    uint64_t ntp_;
};

class RtcpXR : public RtcpPacket
{
public:
    RtcpXR(const StreamBuffer::Ptr& buffer, int pos);

public:
    void parse();

private:
    RtcpXrRrtBlock _rrtBlock;
    vector<RtcpXrDlsrBlock> _dlsrBlocks;
};

class RtcpSdes : public RtcpPacket
{
public:
    RtcpSdes(const StreamBuffer::Ptr& buffer, int pos);

public:
    void parse();
};

class RtcpApp : public RtcpPacket
{
public:
    RtcpApp(const StreamBuffer::Ptr& buffer, int pos);

public:
    void parse();

private:
    string _name;
};

class RtcpNack : public RtcpPacket
{
public:
    RtcpNack(const StreamBuffer::Ptr& buffer, int pos);
    RtcpNack() {}

public:
    void parse();
    vector<uint16_t> getLossPacket() {return _lossSn;}

    StreamBuffer::Ptr encode();
    void setSsrc(uint32_t ssrc) {_ssrc = ssrc;}
    void setLossSn(const vector<uint16_t>& lossSn) {_lossSn = lossSn;}

private:
    uint32_t _ssrc;
    vector<uint16_t> _lossSn;
};

enum PacketChunkStatus
{
    PacketNotReceived = 0,
    PacketReceivedSmall,
    PacketReceivedlarge,
    Reserved
};

class PacketChunkInfo
{
public:
    bool isRunLengthChunk;
    PacketChunkStatus status;
    uint64_t recvTime;
    uint32_t seq;
};

class RunLengthChunk
{
public:
    uint8_t type;
    uint8_t statusSymbol;
    uint16_t length;
};

class StatusVectorChunk
{
public:
    uint8_t type;
    uint8_t symbolSize;
    uint16_t symbolList;
};

class RtcpTWCC : public RtcpPacket
{
public:
    RtcpTWCC(const StreamBuffer::Ptr& buffer, int pos);
    RtcpTWCC() {}

public:
    void parse();
    void setSsrc(uint32_t ssrc) {_ssrc = ssrc;}
    void setBaseSn(uint16_t baseSeqNum) {_baseSeqNum = baseSeqNum;}
    void setreferenceTime(uint16_t referenceTime) {_referenceTime = referenceTime;}
    void setFbPktCnt(uint16_t fbPktCnt) {_fbPktCnt = fbPktCnt;}
    void addPacket(const PacketChunkInfo& pktChunk) {_pktChunks.emplace_back(std::move(pktChunk));}
    StringBuffer::Ptr encode();
    uint16_t getFbPktCnt() {return _fbPktCnt;}
    vector<PacketChunkInfo> getPktChunks() {return _pktChunks;}

private:
    uint32_t _ssrc;
    uint16_t _baseSeqNum;
    uint16_t _pktStatusCnt;
    uint16_t _referenceTime; // 单位：64ms
    uint16_t _fbPktCnt;
    vector<PacketChunkInfo> _pktChunks;
    // vector<uint8_t> _recvDeltas; // 单位：0.25ms
};

class RtcpPli : public RtcpPacket
{
public:
    RtcpPli(const StreamBuffer::Ptr& buffer, int pos);
    RtcpPli() {}

public:
    void parse();
    StreamBuffer::Ptr encode(int ssrc);

private:
    uint32_t _ssrc;
};

class RtcpSli : public RtcpPacket
{
public:
    RtcpSli(const StreamBuffer::Ptr& buffer, int pos);

public:
    void parse();

private:
    uint32_t _ssrc;
};

class RtcpFir : public RtcpPacket
{
public:
    RtcpFir(const StreamBuffer::Ptr& buffer, int pos) : RtcpPacket(buffer, pos) {}

public:
    void parse() {}

private:
    uint32_t _ssrc;
};

class RtcpRemb : public RtcpPacket
{
public:
    RtcpRemb(const StreamBuffer::Ptr& buffer, int pos) : RtcpPacket(buffer, pos) {}

public:
    void parse() {}

private:
    uint32_t _ssrc;
};

class RtcpRpsi : public RtcpPacket
{
public:
    RtcpRpsi(const StreamBuffer::Ptr& buffer, int pos);

public:
    void parse();

private:
    uint32_t _ssrc;
};

class RtcpBye : public RtcpPacket
{
public:
    RtcpBye(const StreamBuffer::Ptr& buffer, int pos);

public:
    void parse();

private:
    vector<int> _ssrcs;
    string _reason;
};

#endif //WebrtcRtcpPacket_H
