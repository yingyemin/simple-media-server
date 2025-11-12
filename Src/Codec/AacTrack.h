#ifndef AacTrack_H
#define AacTrack_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"

// using namespace std;

class AacTrack : public TrackInfo
{
public:
    using Ptr = std::shared_ptr<AacTrack>;

    AacTrack();
    virtual ~AacTrack() {}

public:
    static AacTrack::Ptr createTrack(int index, int payloadType, int samplerate);

public:
    std::string getSdp() override;
    std::string getConfig() override {return _aacConfig;}
    static std::shared_ptr<StreamBuffer> getMuteConfig();
    bool isReady() override {return !_aacConfig.empty();}
    std::string getAacInfo();
    void setAacInfo(const std::string& aacConfig);
    void setAacInfo(int profile, int channel, int sampleRate);
    std::string getAdtsHeader(int frameSize);
    void setAacInfoByAdts(const char* data, int len);
    void onFrame(const std::shared_ptr<FrameBuffer>& frame);

    static void registerTrackInfo();

private:
    std::string _aacConfig;
};

#endif //AacTrack_H