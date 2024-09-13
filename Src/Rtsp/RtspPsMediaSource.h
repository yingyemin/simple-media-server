#ifndef RtspPsMediaSource_H
#define RtspPsMediaSource_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <deque>

#include "RtspMediaSource.h"
#include "Common/DataQue.h"
#include "Common/StampAdjust.h"
#include "RtspPsDecodeTrack.h"
#include "RtspPsEncodeTrack.h"

using namespace std;

class RtspPsMediaSource : public RtspMediaSource
{
public:
    using Ptr = shared_ptr<RtspPsMediaSource>;
    using Wptr = weak_ptr<RtspPsMediaSource>;
    using DataType = shared_ptr<deque<RtpPacket::Ptr> >;
    using QueType = DataQue<DataType>;

    RtspPsMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr, bool muxer = false);
    virtual ~RtspPsMediaSource();

public:
    void addTrack(const RtspPsDecodeTrack::Ptr& track);
    void addDecodeTrack(const shared_ptr<TrackInfo>& track);
    void addTrack(const shared_ptr<TrackInfo>& track) override;
    void addSink(const MediaSource::Ptr &src) override;
    void delSink(const MediaSource::Ptr &src) override;
    void onFrame(const FrameBuffer::Ptr& frame) override;
    void onReady() override;
    RtspTrack::Ptr getTrack(int index);
    // unordered_map<int/*index*/, RtspTrack::Ptr> getTrack() {return _mapRtspTrack;}

    void setSdp(const string& sdp);
    string getSdp();
    int playerCount();
    void getClientList(const function<void(const list<ClientInfo>& info)>& func) override;
    QueType::Ptr getRing() {return _ring;}

    void addControl2Index(const string& control, int index)
    {
        _mapControl2Index[control] = index;
    }

    int getIndexByControl(const string& control)
    {
        if (_mapControl2Index.find(control) == _mapControl2Index.end()) {
            return -1;
        }
        return _mapControl2Index[control];
    }

private:
    bool _muxer;
    bool _probeFinish = false;
    bool _start = false;
    int _ringSize = 512;

    mutex _mtxSdp;
    string _sdp;

    QueType::Ptr _ring;
    DataType _cache;

    mutex _mtxTrack;
    RtspPsDecodeTrack::Ptr _psDecode;
    RtspPsEncodeTrack::Ptr _psEncode;
    unordered_map<int/*index*/, StampAdjust::Ptr> _mapStampAdjust;

    unordered_map<string, int> _mapControl2Index;
};


#endif //RtspPsMediaSource_H
