#ifndef OpusTrack_H
#define OpusTrack_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"

using namespace std;

class OpusTrack : public TrackInfo
{
public:
    using Ptr = shared_ptr<OpusTrack>;

public:
    static OpusTrack::Ptr createTrack(int index, int payloadType, int samplerate);

    OpusTrack();
    virtual ~OpusTrack() {}

public:
    string getSdp() override;
    
    static void registerTrackInfo();

private:
};


#endif //OpusTrack_H
