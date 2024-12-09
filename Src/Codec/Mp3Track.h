#ifndef Mp3Track_H
#define Mp3Track_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"

using namespace std;

class Mp3Track : public TrackInfo
{
public:
    using Ptr = shared_ptr<Mp3Track>;

public:
    static Mp3Track::Ptr createTrack(int index, int payloadType, int samplerate);

    Mp3Track();
    virtual ~Mp3Track() {}

public:
    string getSdp() override;

private:
};


#endif //Mp3Track_H
