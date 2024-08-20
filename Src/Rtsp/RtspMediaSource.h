#ifndef RtspMediaSource_H
#define RtspMediaSource_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <deque>

#include "Common/MediaSource.h"
#include "RtspTrack.h"
#include "Common/DataQue.h"
#include "Common/StampAdjust.h"

using namespace std;

class RtspMediaSource : public MediaSource
{
public:
    using Ptr = shared_ptr<RtspMediaSource>;
    using Wptr = weak_ptr<RtspMediaSource>;
    using DataType = shared_ptr<deque<RtpPacket::Ptr> >;
    using QueType = DataQue<DataType>;

    RtspMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr, bool muxer = false);
    virtual ~RtspMediaSource();

public:
    virtual void addTrack(const RtspTrack::Ptr& track);
    virtual void addTrack(const shared_ptr<TrackInfo>& track) override;
    virtual void addSink(const MediaSource::Ptr &src) override;
    virtual void delSink(const MediaSource::Ptr &src) override;
    virtual void onFrame(const FrameBuffer::Ptr& frame) override;
    virtual RtspTrack::Ptr getTrack(int index);
    virtual unordered_map<int/*index*/, RtspTrack::Ptr> getTrack() {return _mapRtspTrack;}

    virtual void setSdp(const string& sdp);
    virtual string getSdp();
    virtual QueType::Ptr getRing() {return _ring;}
    virtual int playerCount() override;
    virtual void getClientList(const function<void(const list<ClientInfo>& info)>& func) override;

    virtual void addControl2Index(const string& control, int index)
    {
        _mapControl2Index[control] = index;
    }

    virtual int getIndexByControl(const string& control)
    {
        if (_mapControl2Index.find(control) == _mapControl2Index.end()) {
            return -1;
        }
        return _mapControl2Index[control];
    }

private:
    bool _muxer;
    bool _start = false;
    int _ringSize = 512;

    mutex _mtxSdp;
    string _sdp;

    QueType::Ptr _ring;
    DataType _cache;

    mutex _mtxTrack;
    unordered_map<int/*index*/, RtspTrack::Ptr> _mapRtspTrack;
    unordered_map<int/*index*/, StampAdjust::Ptr> _mapStampAdjust;

    unordered_map<string, int> _mapControl2Index;
};


#endif //RtspMediaSource_H
