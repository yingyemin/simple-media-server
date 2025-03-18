#ifndef StampAdjust_H
#define StampAdjust_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"

using namespace std;

enum StampMode
{
    useSourceStamp,
    useSystemTime,
    useSamplerate
};

class StampAdjust
{
public:
    using Ptr = shared_ptr<StampAdjust>;
    virtual void inputStamp(uint64_t& pts, uint64_t& dts, int samples) {}
    virtual void setCodec(const string& codec) {}
    virtual void setStampMode(StampMode mode) {}
};

class AudioStampAdjust : public StampAdjust, public enable_shared_from_this<AudioStampAdjust>
{
public:
    using Ptr = shared_ptr<AudioStampAdjust>;

    AudioStampAdjust(int samplerate = 8000);
    ~AudioStampAdjust();

public:
    void inputStamp(uint64_t& pts, uint64_t& dts, int samples) override;
    void setCodec(const string& codec) override {_codec = codec;}
    void setStampMode(StampMode mode) {_stampMode = mode;}

private:
    StampMode _stampMode = useSourceStamp;
    int _samplerate = 0;
    uint64_t _count = 0;
    uint64_t _lastPts;
    uint64_t _avgStep = 0;
    uint64_t _totalStamp = 0;
    uint64_t _startTime = 0;
    uint64_t _totalSysTime;

    uint64_t _adjustPts;

    string _codec;
};

class VideoStampAdjust : public StampAdjust, public enable_shared_from_this<VideoStampAdjust>
{
public:
    using Ptr = shared_ptr<VideoStampAdjust>;

    VideoStampAdjust(int fps = 0);
    ~VideoStampAdjust();

public:
    void inputStamp(uint64_t& pts, uint64_t& dts, int samples = 1) override;
    void setStampMode(StampMode mode) {_stampMode = mode;}

private:
    StampMode _stampMode = useSourceStamp;
    int _fps;
    int _guessFps = 25;
    uint64_t _count = 0;
    uint64_t _lastDts;
    uint64_t _avgStep = 0;
    uint64_t _totalStamp = 0;
    uint64_t _startTime = 0;
    uint64_t _totalSysTime;

    uint64_t _adjustDts;
};


#endif //Track_H
