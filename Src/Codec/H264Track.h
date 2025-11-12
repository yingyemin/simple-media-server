#ifndef H264Track_H
#define H264Track_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"

// using namespace std;

class H264Track : public TrackInfo
{
public:
    using Ptr = std::shared_ptr<H264Track>;

    H264Track();
    virtual ~H264Track() {}

public:
    static H264Track::Ptr createTrack(int index, int payloadType, int samplerate);

public:
    void setSps(const std::shared_ptr<FrameBuffer>& sps) {_sps = sps;}
    void setPps(const std::shared_ptr<FrameBuffer>& pps);
    std::string getSdp() override;
    std::string getConfig() override;
    void setConfig(const std::string& config);
    bool isReady() override {return _sps && _pps;}
    void getWidthAndHeight(int& width, int& height, int& fps);
    
    void getVpsSpsPps(std::shared_ptr<FrameBuffer>& vps, std::shared_ptr<FrameBuffer>& sps, std::shared_ptr<FrameBuffer>& pps) override
    {
        vps = nullptr;
        sps = _sps;
        pps = _pps;
    }

    void onFrame(const std::shared_ptr<FrameBuffer>& frame); 
    static void registerTrackInfo();

public:
    std::shared_ptr<FrameBuffer> _sps;
    std::shared_ptr<FrameBuffer> _pps;
    std::string _avcc;
};

#endif //H264Track_H