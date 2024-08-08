#ifndef H265Track_H
#define H265Track_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Common/Track.h"
#include "Common/Frame.h"

using namespace std;

class H265Track : public TrackInfo
{
public:
    using Ptr = shared_ptr<H265Track>;

    H265Track();
    virtual ~H265Track() {}

public:
    void setVps(const FrameBuffer::Ptr& vps) {_vps = vps;}
    void setSps(const FrameBuffer::Ptr& sps) {_sps = sps;}
    void setPps(const FrameBuffer::Ptr& pps) {_pps = pps;}
    string getSdp() override;
    void getWidthAndHeight(int& width, int& height);

public:
    FrameBuffer::Ptr _vps;
    FrameBuffer::Ptr _sps;
    FrameBuffer::Ptr _pps;
    string _hvcc;
};


#endif //H264Track_H
