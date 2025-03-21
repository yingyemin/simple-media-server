﻿#ifndef H264Track_H
#define H264Track_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"

using namespace std;

class H264Track : public TrackInfo
{
public:
    using Ptr = shared_ptr<H264Track>;

    H264Track();
    virtual ~H264Track() {}

public:
    static H264Track::Ptr createTrack(int index, int payloadType, int samplerate);

public:
    void setSps(const FrameBuffer::Ptr& sps) {_sps = sps;}
    void setPps(const FrameBuffer::Ptr& pps);
    string getSdp() override;
    string getConfig() override;
    void setConfig(const string& config);
    bool isReady() override {return _sps && _pps;}
    void getWidthAndHeight(int& width, int& height, int& fps);
    
    void getVpsSpsPps(FrameBuffer::Ptr& vps, FrameBuffer::Ptr& sps, FrameBuffer::Ptr& pps) override
    {
        vps = nullptr;
        sps = _sps;
        pps = _pps;
    }

    void onFrame(const FrameBuffer::Ptr& frame); 
    static void registerTrackInfo();

public:
    FrameBuffer::Ptr _sps;
    FrameBuffer::Ptr _pps;
    string _avcc;
};


#endif //H264Track_H
