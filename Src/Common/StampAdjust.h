#ifndef StampAdjust_H
#define StampAdjust_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"

using namespace std;

class StampAdjust
{
public:
    using Ptr = shared_ptr<StampAdjust>;
    virtual void inputStamp(uint64_t& pts, uint64_t& dts, int samples) {}
};

class AudioStampAdjust : public StampAdjust, public enable_shared_from_this<AudioStampAdjust>
{
public:
    using Ptr = shared_ptr<AudioStampAdjust>;

    AudioStampAdjust(int samplerate = 8000);
    ~AudioStampAdjust();

public:
    void inputStamp(uint64_t& pts, uint64_t& dts, int samples) override;

private:
    int _samplerate;
    uint64_t _count = 0;
    uint64_t _lastPts;
    uint64_t _avgStep = 0;
    uint64_t _totalStamp = 0;
    uint64_t _startTime = 0;
    uint64_t _totalSysTime;

    uint64_t _adjustPts;
};

class VideoStampAdjust : public StampAdjust, public enable_shared_from_this<VideoStampAdjust>
{
public:
    using Ptr = shared_ptr<VideoStampAdjust>;

    VideoStampAdjust(int fps = 0);
    ~VideoStampAdjust();

public:
    void inputStamp(uint64_t& pts, uint64_t& dts, int samples = 1) override;

private:
    int _fps;
    int _guessFps = 25;
    uint64_t _count = 0;
    uint64_t _lastPts;
    uint64_t _avgStep = 0;
    uint64_t _totalStamp = 0;
    uint64_t _startTime = 0;
    uint64_t _totalSysTime;

    uint64_t _adjustPts;
};


#endif //Track_H
