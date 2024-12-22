#ifndef AV1Track_H
#define AV1Track_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"

using namespace std;

class AV1Info
{
public:
    int width = 0;
    int height = 0;
    int fps = 0;
    int seq_profile = 0;
    int initial_presentation_delay_present = 0;
    int seq_level_idx_0  = 0;
    int seq_tier_0  = 0;
    int initial_presentation_delay_minus_one = 0;
    int buffer_delay_length_minus_1 = 0;
};

class AV1Track : public TrackInfo
{
public:
    using Ptr = shared_ptr<AV1Track>;

    AV1Track();
    virtual ~AV1Track() {}

public:
    static AV1Track::Ptr createTrack(int index, int payloadType, int samplerate);

public:
    void setVps(const FrameBuffer::Ptr& sequence) {_sequence = sequence;}
    string getSdp() override;
    string getConfig() override;
    void getWidthAndHeight(int& width, int& height, int& fps);
    bool isBFrame(unsigned char* data, int size);
    bool isReady() override {return !!_sequence;}
    
    void getVpsSpsPps(FrameBuffer::Ptr& vps, FrameBuffer::Ptr& sps, FrameBuffer::Ptr& pps) override
    {
        vps = _sequence;
    }
    
    void onFrame(const FrameBuffer::Ptr& frame); 

    static void registerTrackInfo();

public:
    FrameBuffer::Ptr _sequence;
    string _av1Cfg;
    AV1Info info;
};


#endif //H264Track_H
