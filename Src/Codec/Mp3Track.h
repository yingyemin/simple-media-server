#ifndef Mp3Track_H
#define Mp3Track_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"

// using namespace std;

class ID3v2
{
public:
    char identifier[3];
    uint16_t version;
    uint8_t flag;
    uint32_t size;
};

class MP3FrameHeader
{
public:
    // 全为1
    unsigned int sync1:11;                     //同步信息
    // 00-MPEG 2.5 01-未定义 10-MPEG 2 11-MPEG 1
    unsigned int version:2;                    //版本
    // 00-未定义 01-Layer 3 10-Layer 2 11-Layer 1
    unsigned int layer:2;                      //层
    // 0-校验 1-不校验
    // 如果校验，帧头最后两个字节为crc值
    unsigned int crc_check:1;                  //CRC校验

    // 比特率，单位是kbps
    unsigned int bit_rate_index:4;             //比特率索引
    // 采样率
    // 对于MPEG-1： 00-44.1kHz 01-48kHz 10-32kHz 11-未定义
    // 对于MPEG-2： 00-22.05kHz 01-24kHz 10-16kHz 11-未定义
    // 对于MPEG-2.5： 00-11.025kHz 01-12kHz 10-8kHz 11-未定义
    unsigned int sample_rate_index:2;          //采样率索引
    // 帧的填充大小
    // 一帧的字节数 Size=((采样个数 * (1 / 采样率))* 帧的比特率)/8 + 帧的填充大小
    unsigned int padding:1;                    //帧长调节位
    unsigned int reserved:1;                   //保留字

    // 00-立体声Stereo 01-Joint Stereo 10-双声道 11-单声道
    unsigned int channel_mode:2;               //声道模式
    // 当声道模式为01是才使用
    // Value   强度立体声    MS立体声
    // 00             off             off
    // 01             on              off
    // 10             off              on
    // 11              on             on
    unsigned int mode_extension:2;             //扩展模式，仅用于联合立体声
    // 0-不合法 1-合法
    unsigned int copyright:1;                  //版权标志
    // 0-非原版 1-原版
    unsigned int original:1;                   //原版标志
    // 00-未定义 01-50/15ms 10-保留 11-CCITT J.17
    unsigned int emphasis:2;                   //强调方式

    uint16_t crc;
    // mpeg 1：单声道17个字节，双声道32字节
    // mpeg 2：单声道占9字节，双声道占17字节
    std::string sideInfo; // 7/19/32 字节
};

class Mp3Track : public TrackInfo
{
public:
    using Ptr = std::shared_ptr<Mp3Track>;

public:
    static Mp3Track::Ptr createTrack(int index, int payloadType, int samplerate);

    Mp3Track();
    virtual ~Mp3Track() {}

public:
    std::string getSdp() override;
    void onFrame(const FrameBuffer::Ptr& frame) override;
    static void registerTrackInfo();

private:
};


#endif //Mp3Track_H
