#ifndef T1003_H
#define T1003_H

#include <cstdint>

// 表示终端上传音视频属性的类
class T1003 {
public:
    // 构造函数
    T1003();

    // Getter 和 Setter 方法
    uint8_t getAudioFormat() const;
    void setAudioFormat(uint8_t format);

    uint8_t getAudioChannels() const;
    void setAudioChannels(uint8_t channels);

    uint8_t getAudioSamplingRate() const;
    void setAudioSamplingRate(uint8_t rate);

    uint8_t getAudioBitDepth() const;
    void setAudioBitDepth(uint8_t depth);

    uint16_t getAudioFrameLength() const;
    void setAudioFrameLength(uint16_t length);

    uint8_t getAudioSupport() const;
    void setAudioSupport(uint8_t support);

    uint8_t getVideoFormat() const;
    void setVideoFormat(uint8_t format);

    uint8_t getMaxAudioChannels() const;
    void setMaxAudioChannels(uint8_t channels);

    uint8_t getMaxVideoChannels() const;
    void setMaxVideoChannels(uint8_t channels);

private:
    // 输入音频编码方式，占用 1 字节
    uint8_t audioFormat;
    // 输入音频声道数，占用 1 字节
    uint8_t audioChannels;
    // 输入音频采样率，占用 1 字节
    uint8_t audioSamplingRate;
    // 输入音频采样位数，占用 1 字节
    uint8_t audioBitDepth;
    // 音频帧长度，占用 2 字节
    uint16_t audioFrameLength;
    // 是否支持音频输出，占用 1 字节
    uint8_t audioSupport;
    // 视频编码方式，占用 1 字节
    uint8_t videoFormat;
    // 终端支持的最大音频物理通道，占用 1 字节
    uint8_t maxAudioChannels;
    // 终端支持的最大视频物理通道，占用 1 字节
    uint8_t maxVideoChannels;
};

#endif // T1003_H
