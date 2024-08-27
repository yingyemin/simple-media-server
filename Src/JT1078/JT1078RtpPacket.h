#ifndef JT1078RtpPacket_H
#define JT1078RtpPacket_H

#include "Buffer.h"
#include <unordered_map>

using namespace std;

enum JT1078_SUBMARK
{
    JT1078_Atomic,
    JT1078_First,
    JT1078_Last,
    JT1078_Intermediate,
    JT1078_UnSupportSubMark,
};

enum JT1078_STREAM_TYPE
{
    JT1078_VideoI,
    JT1078_VideoP,
    JT1078_VideoB,
    JT1078_Audio,
    JT1078_Passthrough,
    JT1078_UnSupportDataType,
};


class JT1078RtpHeader
{
public:
    uint32_t            framHeadMark;           //帧头标识
#if __BYTE_ORDER == __BIG_ENDIAN
    uint8_t             version : 2;                     //固定为2
    uint8_t             p : 1;                     //固定为0
    uint8_t             extend : 1;                     //RTP头是否需要扩展位，固定为0
    uint8_t             cc : 4;                    //固定为1
    uint8_t             mark : 1;                     //标志位，确定是否是完整数据帧的边界
    uint8_t             pt : 7;                    //负载类型
#else
    uint8_t             cc : 4;                    //固定为1
    uint8_t             extend : 1;                     //RTP头是否需要扩展位，固定为0
    uint8_t             p : 1;                     //固定为0
    uint8_t             version : 2;                     //固定为2
    uint8_t             pt : 7;                    //负载类型
    uint8_t             mark : 1;                     //标志位，确定是否是完整数据帧的边界
#endif
    uint16_t            seq;        //RTP数据包序号每发送一个RTP数据包序列号加1
    uint8_t             simNo[6];      //SIM卡号
    uint8_t             logicChannelNumber;    //逻辑通道号
#if __BYTE_ORDER == __BIG_ENDIAN
    uint8_t             dataType : 4;              //数据类型
    uint8_t             subPackageHandleMark : 4;  //分包处理标记
#else
    uint8_t             subPackageHandleMark : 4;  //分包处理标记
    uint8_t             dataType : 4;              //数据类型
#endif
    // uint64_t            timeStamp;             //时间戳
    // uint16_t            lastKeyframeInterval;     //与上一帧的时间间隔
    // uint16_t            lastFrameInterval;      //与上一帧的时间间隔
    // uint16_t            bodyLen;                //数据体长度
};

class JT1078RtpPacket
{
public:
    using Ptr = shared_ptr<JT1078RtpPacket>;
    JT1078RtpPacket(const StreamBuffer::Ptr& buffer);

public:
    // 有效负载，跳过csrc、ext
    char *getPayload();
    // 有效负载长度，不包括csrc、ext、padding
    size_t getPayloadSize();
    char* data();
    size_t size();
    StreamBuffer::Ptr buffer();
    string getCodecType();
    JT1078_STREAM_TYPE getStreamType();
    JT1078_SUBMARK getSubMark();
    string getSimCode();
    void setPayloadIndex(int index) {_payloadIndex = index;}
    uint64_t getTimestamp();
    string getTrackType();
    uint16_t getSeq();
    int getLogicNo() {return _header->logicChannelNumber;}
    JT1078RtpHeader* getHeader() {return _header;}

public:
    // 音视频类型
    string type_;
    // 音频为采样率，视频一般为90000
    uint32_t samplerate_;

    int trackIndex_;

private:
    JT1078_STREAM_TYPE  _streamType = JT1078_UnSupportDataType;
    JT1078_SUBMARK      _subMark = JT1078_UnSupportSubMark;
    uint64_t            _timeStamp;             //时间戳
    uint16_t            _lastKeyframeInterval;     //与上一帧的时间间隔
    uint16_t            _lastFrameInterval;      //与上一帧的时间间隔
    uint16_t            _bodyLen;                //数据体长度
    uint16_t            _payloadIndex;
    string              _codec;
    string              _type;
    std::string         _simCode;
    JT1078RtpHeader* _header;
    StreamBuffer::Ptr _buffer;
};



#endif //JT1078RtpPacket_H
