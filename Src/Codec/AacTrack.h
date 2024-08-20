#ifndef AacTrack_H
#define AacTrack_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"

using namespace std;

class AacTrack : public TrackInfo
{
public:
    using Ptr = shared_ptr<AacTrack>;

    AacTrack();
    virtual ~AacTrack() {}

public:
    string getSdp() override;
    string getConfig() override {return _aacConfig;};
    string getAacInfo();
    void setAacInfo(const string& aacConfig);
    void setAacInfo(int profile, int channel, int sampleRate);
    string getAdtsHeader(int frameSize);
    void setAacInfoByAdts(const char* data, int len);

private:
    string _aacConfig;
};


#endif //AacTrack_H
