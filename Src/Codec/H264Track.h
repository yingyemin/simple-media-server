#ifndef H264Track_H
#define H264Track_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"

using namespace std;

class H264Track : public TrackInfo
{
public:
    using Ptr = shared_ptr<H264Track>;

    H264Track();
    virtual ~H264Track() {}

public:
    void setSps(const FrameBuffer::Ptr& sps) {_sps = sps;}
    void setPps(const FrameBuffer::Ptr& pps) {_pps = pps;}
    string getSdp() override;
    string getConfig() override;
    void getWidthAndHeight(int& width, int& height, int& fps);

public:
    FrameBuffer::Ptr _sps;
    FrameBuffer::Ptr _pps;
    string _avcc;
};


#endif //H264Track_H
