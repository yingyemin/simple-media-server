#ifndef H266Track_H
#define H266Track_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"

using namespace std;

class H266Track : public TrackInfo
{
public:
    using Ptr = shared_ptr<H266Track>;

    H266Track();
    virtual ~H266Track() {}

public:
    static H266Track::Ptr createTrack(int index, int payloadType, int samplerate);

public:
    void setVps(const FrameBuffer::Ptr& vps) {_vps = vps;}
    void setSps(const FrameBuffer::Ptr& sps) {_sps = sps;}
    void setPps(const FrameBuffer::Ptr& pps) {_pps = pps;}
    string getSdp() override;
    string getConfig() override;
    void getWidthAndHeight(int& width, int& height, int& fps);
    bool isBFrame(unsigned char* data, int size);
    bool isReady() override {return _vps && _sps && _pps;}
    
    void getVpsSpsPps(FrameBuffer::Ptr& vps, FrameBuffer::Ptr& sps, FrameBuffer::Ptr& pps) override
    {
        vps = _vps;
        sps = _sps;
        pps = _pps;
    }
    
    void onFrame(const FrameBuffer::Ptr& frame); 

public:
    FrameBuffer::Ptr _vps;
    FrameBuffer::Ptr _sps;
    FrameBuffer::Ptr _pps;
    string _hvcc;
};


#endif //H264Track_H
