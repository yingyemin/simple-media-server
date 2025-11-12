#ifndef H266Track_H
#define H266Track_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"

// using namespace std;

class H266Track : public TrackInfo
{
public:
    using Ptr = std::shared_ptr<H266Track>;

    H266Track();
    virtual ~H266Track() {}

public:
    static H266Track::Ptr createTrack(int index, int payloadType, int samplerate);

public:
    void setVps(const std::shared_ptr<FrameBuffer>& vps) {_vps = vps;}
    void setSps(const std::shared_ptr<FrameBuffer>& sps) {_sps = sps;}
    void setPps(const std::shared_ptr<FrameBuffer>& pps) {_pps = pps;}
    std::string getSdp() override;
    std::string getConfig() override;
    void setConfig(const std::string& config);
    void getWidthAndHeight(int& width, int& height, int& fps);
    bool isBFrame(unsigned char* data, int size);
    bool isReady() override {return _sps && _pps;}
    
    void getVpsSpsPps(std::shared_ptr<FrameBuffer>& vps, std::shared_ptr<FrameBuffer>& sps, std::shared_ptr<FrameBuffer>& pps) override
    {
        vps = _vps;
        sps = _sps;
        pps = _pps;
    }
    
    void onFrame(const std::shared_ptr<FrameBuffer>& frame); 
    static void registerTrackInfo();

public:
    std::shared_ptr<FrameBuffer> _vps;
    std::shared_ptr<FrameBuffer> _sps;
    std::shared_ptr<FrameBuffer> _pps;
    std::string _hvcc;
};

#endif //H266Track_H