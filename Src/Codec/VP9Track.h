#ifndef VP9Track_H
#define VP9Track_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"

// using namespace std;

class VP9Track : public TrackInfo
{
public:
    using Ptr = std::shared_ptr<VP9Track>;

    VP9Track();
    virtual ~VP9Track() {}

public:
    static VP9Track::Ptr createTrack(int index, int payloadType, int samplerate);

public:
    std::string getSdp() override;
    std::string getConfig() override;
    void getWidthAndHeight(int& width, int& height, int& fps);
    bool isBFrame(unsigned char* data, int size);
    bool isReady() override {return _hasReady;}
    
    void setConfig(const std::string& config);
    
    void onFrame(const FrameBuffer::Ptr& frame); 
    static void registerTrackInfo();

private:
    uint32_t _profile = 0;
    uint32_t _codec_intialization_data_size = 0;
    uint8_t _chroma_subsampling = 0;
    uint8_t _video_full_range_flag = 0;
    uint8_t _colour_primaries = 0;
    uint8_t _transfer_characteristics = 0;
    uint8_t _matrix_coefficients = 0;
    uint8_t _level = 31;
    uint8_t _bitDepth = 8;
    std::string _VP9Cfg;
    std::string _codec_intialization_data;
};


#endif //H264Track_H
