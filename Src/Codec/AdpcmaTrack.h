#ifndef AdpcmaTrack_H
#define AdpcmaTrack_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"

// using namespace std;

class AdpcmaTrack : public TrackInfo
{
public:
    using Ptr = std::shared_ptr<AdpcmaTrack>;

public:
    static AdpcmaTrack::Ptr createTrack(int index, int payloadType, int samplerate);

    AdpcmaTrack();
    virtual ~AdpcmaTrack() {}

public:
    std::string getSdp() override;
    
    static void registerTrackInfo();
};


#endif //AdpcmaTrack_H
