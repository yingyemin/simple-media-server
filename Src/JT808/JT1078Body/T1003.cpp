#include "T1003.h"

// 构造函数，初始化成员变量
T1003::T1003() :
    audioFormat(0),
    audioChannels(0),
    audioSamplingRate(0),
    audioBitDepth(0),
    audioFrameLength(0),
    audioSupport(0),
    videoFormat(0),
    maxAudioChannels(0),
    maxVideoChannels(0) {}

// 获取输入音频编码方式
uint8_t T1003::getAudioFormat() const {
    return audioFormat;
}

// 设置输入音频编码方式
void T1003::setAudioFormat(uint8_t format) {
    audioFormat = format;
}

// 获取输入音频声道数
uint8_t T1003::getAudioChannels() const {
    return audioChannels;
}

// 设置输入音频声道数
void T1003::setAudioChannels(uint8_t channels) {
    audioChannels = channels;
}

// 获取输入音频采样率
uint8_t T1003::getAudioSamplingRate() const {
    return audioSamplingRate;
}

// 设置输入音频采样率
void T1003::setAudioSamplingRate(uint8_t rate) {
    audioSamplingRate = rate;
}

// 获取输入音频采样位数
uint8_t T1003::getAudioBitDepth() const {
    return audioBitDepth;
}

// 设置输入音频采样位数
void T1003::setAudioBitDepth(uint8_t depth) {
    audioBitDepth = depth;
}

// 获取音频帧长度
uint16_t T1003::getAudioFrameLength() const {
    return audioFrameLength;
}

// 设置音频帧长度
void T1003::setAudioFrameLength(uint16_t length) {
    audioFrameLength = length;
}

// 获取是否支持音频输出
uint8_t T1003::getAudioSupport() const {
    return audioSupport;
}

// 设置是否支持音频输出
void T1003::setAudioSupport(uint8_t support) {
    audioSupport = support;
}

// 获取视频编码方式
uint8_t T1003::getVideoFormat() const {
    return videoFormat;
}

// 设置视频编码方式
void T1003::setVideoFormat(uint8_t format) {
    videoFormat = format;
}

// 获取终端支持的最大音频物理通道
uint8_t T1003::getMaxAudioChannels() const {
    return maxAudioChannels;
}

// 设置终端支持的最大音频物理通道
void T1003::setMaxAudioChannels(uint8_t channels) {
    maxAudioChannels = channels;
}

// 获取终端支持的最大视频物理通道
uint8_t T1003::getMaxVideoChannels() const {
    return maxVideoChannels;
}

// 设置终端支持的最大视频物理通道
void T1003::setMaxVideoChannels(uint8_t channels) {
    maxVideoChannels = channels;
}
