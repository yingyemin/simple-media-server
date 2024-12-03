#ifndef G711Track_H
#define G711Track_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"

using namespace std;

class G711aTrack : public TrackInfo
{
public:
    using Ptr = shared_ptr<G711aTrack>;

public:
    static G711aTrack::Ptr createTrack(int index, int payloadType, int samplerate);

    G711aTrack();
    virtual ~G711aTrack() {}

public:
    string getSdp() override;

private:
};

class G711uTrack : public TrackInfo
{
public:
    using Ptr = shared_ptr<G711uTrack>;

public:
    static G711uTrack::Ptr createTrack(int index, int payloadType, int samplerate);

    G711uTrack();
    virtual ~G711uTrack() {}

public:
    string getSdp() override;

private:
};


#endif //G711Track_H
