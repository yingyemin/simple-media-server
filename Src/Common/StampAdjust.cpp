#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "StampAdjust.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/TimeClock.h"
#include "Common/Config.h"

using namespace std;

AudioStampAdjust::AudioStampAdjust(int samplerate)
    :_samplerate(samplerate)
{

}

AudioStampAdjust::~AudioStampAdjust()
{

}

void AudioStampAdjust::inputStamp(uint64_t& pts, uint64_t& dts, int samples)
{
    if (_startTime == 0) {
        _startTime = TimeClock::now();
        _lastPts = pts;

        pts = 0;
        dts = pts > dts ? (pts - dts) : 0;

        _adjustPts = 0;

        return ;
    }

    int step = pts - _lastPts;
    // TODO: 从配置里读
    // logInfo << "audio step: " << step;
    if ((step < -500 || step > 500 || step == 0)) {
        if (_samplerate) {
            step = (samples * 1.0 / _samplerate) * 1000;
        } else {
            step = _avgStep;
        }
        step = 1;
    }

    // if (_codec == "g711a" || _codec == "g711u" && samples > 8) {
    //     // 先写成8000 / 1000， 后续需要用真实的_samplerate / 1000
    //     // 不等于0，表明时间戳不是倍数（考虑丢包），时间戳异常，需要调整
    //     if (step % (samples / 8) != 0) {
    //         step = samples / 8;
    //     }
    // }
    
    _totalSysTime = TimeClock::now() - _startTime;
    _totalStamp += step;
    ++_count;
    _avgStep = _totalStamp / _count;

    // logInfo << "step: " << step;
    // logInfo << "audio _avgStep: " << _avgStep;
    // logInfo << "audio _lastPts: " << _lastPts;
    // logInfo << "audio pts: " << pts;
    // logInfo << "audio _adjustPts: " << _adjustPts;
    _adjustPts += step;
    _lastPts = pts;
    if (pts > dts && pts - dts < 10000) {
        dts = _adjustPts + pts - dts;
    } else {
        dts = (_adjustPts > (dts - pts)) ? (_adjustPts - (dts - pts)) : 0;
    }
    pts = _adjustPts;

    // logInfo << "_totalSysTime: " << _totalSysTime;
    // logInfo << "_totalStamp: " << _totalStamp;
    // if (_totalSysTime > _totalStamp && ((_totalSysTime - _totalStamp) > 5000)) {
    //     auto diff = _totalSysTime - _totalStamp;
    //     auto diffTmp = diff % step;

    //     _adjustPts += diff - diffTmp;
    // }
}

////////////////VideoStampAdjust///////////////////////////

VideoStampAdjust::VideoStampAdjust(int fps)
    :_fps(fps)
{

}

VideoStampAdjust::~VideoStampAdjust()
{

}

void VideoStampAdjust::inputStamp(uint64_t& pts, uint64_t& dts, int samples)
{
    // 第一次，简单赋值后返回
    if (_startTime == 0) {
        _startTime = TimeClock::now();
        _lastDts = dts;

        dts = 0;
        pts = dts > pts ? (dts - pts) : 0;

        _adjustDts = 0;

        return ;
    }

    // 每隔100个包，计算一下fps
    if (_count > 0 && _count % 100 == 0) {
        // sps里的fps可能是实际fps的两倍
        auto avgFps = (_count * 1000.0 / _totalSysTime);
        if (_fps > 30 && _fps / avgFps == 2.0) {
            _guessFps = _fps / 2;
        } else if (_fps > 0) {
            _guessFps = _fps;
        } else {
            // 平均帧率
            // logInfo << "_count: " << _count;
            // logInfo << "_totalSysTime: " << _totalSysTime;
            // logInfo << "avgFps: " << avgFps;
            // logInfo << "_guessFps: " << _guessFps;
            _guessFps = avgFps;
        }
    }

    // 当前帧与上一帧的增量
    int step = dts - _lastDts;
    // logInfo << "video step: " << step;
    // TODO: 从配置里读
    // 增量太大或者太小或者为0，认为不合理，通过计算的帧率重新算一下
    if ((step < -500 || step > 500 || step == 0) && _guessFps) {
        // step = (samples * 1.0 / _guessFps) * 1000;
        step = 1;
        // logInfo << "step: " << step;
    }
    
    // 系统时间过了多久
    _totalSysTime = TimeClock::now() - _startTime;
    // pts计算的时长
    _totalStamp += step;
    // 总帧数
    ++_count;
    // 平均增量
    _avgStep = _totalStamp / _count;

    // logInfo << "video pts: " << pts;
    // logInfo << "video dts: " << dts;
    // logInfo << "video _adjustPts: " << _adjustPts;
    // 将增量加到_adjustPts（从0开始往上加增量）
    _adjustDts += step;
    
    _lastDts = dts;
    if (dts > pts && dts - dts < 10000) {
        pts = _adjustDts + dts - pts;
    } else {
        pts = (_adjustDts > (pts - dts)) ? (_adjustDts - (pts - dts)) : 0;
    }
    dts = _adjustDts;
    // logInfo << "video adjust pts: " << pts;
    // logInfo << "video adjust dts: " << dts;

    // 系统时间计算的总时间大于pts计算的总时间，超过5秒
    // logInfo << "_totalSysTime: " << _totalSysTime;
    // logInfo << "_totalStamp: " << _totalStamp;
    // if (_totalSysTime > _totalStamp && ((_totalSysTime - _totalStamp) > 5000)) {
    //     auto diff = _totalSysTime - _totalStamp;
    //     auto diffTmp = diff % step;

    //     _adjustPts += diff - diffTmp;
    // }
}