#ifndef Track_H
#define Track_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Frame.h"
#include "UrlParser.h"

using namespace std;

enum PayloadType
{
    PayloadType_G711U = 0,
    PayloadType_G723 = 4,
    PayloadType_G711A = 8,
    PayloadType_G722 = 9,
    PayloadType_Mp3 = 14,
    PayloadType_G729 = 18,
    PayloadType_SVACA = 20,
    PayloadType_JPEG = 26,
    PayloadType_MP2T = 33,
    PayloadType_AV1X = 35,
    PayloadType_AAC = 97,
    PayloadType_H26X = 96,
    PayloadType_OPUS = 103,
    PayloadType_VP8 = 105,
    PayloadType_VP9 = 106,
    PayloadType_AV1 = 107,
    PayloadType_H266 = 108,
};

enum TrackType
{
    VideoTrackType = 0,
    AudioTrackType,
    DataTrackType,
    SubtitleTrackType,
    InvalidTrackType
};

class TrackInfo
{
public:
    using Ptr = shared_ptr<TrackInfo>;
    using funcCreateTrackInfo = function<TrackInfo::Ptr(int trackType, int payloadType, int samplerate)>;

public:
    virtual string getSdp() { return "";}
    virtual string getConfig() {return "";}
    virtual void getWidthAndHeight(int& width, int& height, int& fps) {}
    virtual bool isBFrame(unsigned char* data, int size) {return false;}
    virtual void getVpsSpsPps(FrameBuffer::Ptr& vps, FrameBuffer::Ptr& sps, FrameBuffer::Ptr& pps)
    {
        vps = nullptr;
        sps = nullptr;
        pps = nullptr;
    }
    virtual void setVps(const FrameBuffer::Ptr& vps) {}
    virtual void setSps(const FrameBuffer::Ptr& sps) {}
    virtual void setPps(const FrameBuffer::Ptr& pps) {}

    virtual bool isReady() {return _hasReady;}

    virtual void onFrame(const FrameBuffer::Ptr& frame) {_hasReady = true;}

    void setUrlParser(const UrlParser& parser)
    {
        _parser = parser;
    }

    static TrackInfo::Ptr createTrackInfo(const string& codecName);
    static void registerTrackInfo(const string& codecName, const funcCreateTrackInfo& func);

public:
    bool _hasReady = false;
    int payloadType_;
    int samplerate_;
    int timebase_ = 0;
    int channel_;
    int index_;
    int ssrc_;
    int bitPerSample_ = 16;
    int _bitrate = 0;
    int _width = 0;
    int _height = 0;
    int fps_ = 0;
    uint8_t _dependent_slice_segments_enabled_flag = 0;
    uint8_t _num_extra_slice_header_bits = 0;
    uint32_t _PicSizeInCtbsY = 0;
    string trackType_;
    string codec_;
    UrlParser _parser;

    static unordered_map<string, funcCreateTrackInfo> _mapCreateTrack;
};

// class Track : public enable_shared_from_this<Track>
// {
// public:
//     using Ptr = shared_ptr<Track>;

//     Track();
//     ~Track() {}

// public:

// private:
// };


#endif //Track_H
