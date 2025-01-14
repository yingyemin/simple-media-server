#ifndef VP8Track_H
#define VP8Track_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"

using namespace std;

class VP8Track : public TrackInfo
{
public:
    using Ptr = shared_ptr<VP8Track>;

    VP8Track();
    virtual ~VP8Track() {}

public:
    static VP8Track::Ptr createTrack(int index, int payloadType, int samplerate);

public:
    string getSdp() override;
    string getConfig() override;
    void getWidthAndHeight(int& width, int& height, int& fps);
    bool isBFrame(unsigned char* data, int size);
    bool isReady() override {return _hasReady;}
    
    void setConfig(const string& config);
    
    void onFrame(const FrameBuffer::Ptr& frame); 
    static void registerTrackInfo();

private:
    uint32_t _profile = 0;
    uint8_t _level = 31;
    uint8_t _bitDepth = 8;
    string _vp8Cfg;
};


#endif //H264Track_H
