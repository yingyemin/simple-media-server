#ifndef RtspPollingMediaSource_H
#define RtspPollingMediaSource_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>
#include <deque>

#include "Common/FrameMediaSource.h"
#include "RtspMediaSource.h"
#include "RtspTrack.h"
#include "Common/DataQue.h"
#include "Common/StampAdjust.h"

// using namespace std;

class RtspPollingMediaSource : public RtspMediaSource
{
public:
    using Ptr = std::shared_ptr<RtspPollingMediaSource>;
    using Wptr = std::weak_ptr<RtspPollingMediaSource>;
    using DataType = std::shared_ptr<std::deque<RtpPacket::Ptr> >;
    using QueType = DataQue<DataType>;

    RtspPollingMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr, bool muxer = false);
    virtual ~RtspPollingMediaSource();

public:
    virtual void addTrack(const RtspTrack::Ptr& track);
    virtual void onReady() override;
    virtual void addTrack(const std::shared_ptr<TrackInfo>& track) override;
    virtual void addSink(const MediaSource::Ptr &src) override;
    virtual void delSink(const MediaSource::Ptr &src) override;
    virtual void onFrame(const FrameBuffer::Ptr& frame) override;
    virtual RtspTrack::Ptr getTrack(int index);
    virtual std::unordered_map<int/*index*/, RtspTrack::Ptr> getTrack() {return _mapRtspTrack;}

    virtual void setSdp(const std::string& sdp);
    virtual std::string getSdp();
    virtual QueType::Ptr getRing() {return _ring;}
    virtual int playerCount() override;
    virtual void getClientList(const std::function<void(const std::list<ClientInfo>& info)>& func) override;
    uint64_t getBytes() override { return _ring ? _ring->getBytes() : 0;}

    virtual void addControl2Index(const std::string& control, int index)
    {
        _mapControl2Index[control] = index;
    }

    virtual int getIndexByControl(const std::string& control)
    {
        if (_mapControl2Index.find(control) == _mapControl2Index.end()) {
            return -1;
        }
        return _mapControl2Index[control];
    }

    virtual void setEnableHuge(bool enabled) {_enableHugeRtp = enabled;}
    virtual void setFastPts(bool enabled) {_enableFastPts = enabled;}

    void setStreamList(const std::vector<UrlParser>& streamList) {_streamList = streamList;}

    void start(const std::function<void()>& onReady);
    void onSource(const FrameMediaSource::Ptr& source);

private:
    bool _muxer;
    bool _start = false;
    bool _startPolling = false;
    bool _probeFinish = false;
    bool _enableHugeRtp = false;
    bool _enableFastPts = false;
    int _ringSize = 512;
    uint64_t _streamIndex = 0;
    uint64_t _lastRtpStmp = 0;

    std::mutex _mtxSdp;
    std::string _sdp;

    UrlParser _videoUrlParser;

    FrameMediaSource::Ptr _videoMediaSource;

    MediaSource::FrameRingType::DataQueReaderT::Ptr _videoPlayReader;
    MediaSource::FrameRingType::DataQueReaderT::Ptr _audioPlayReader;

    std::shared_ptr<StampAdjust> _videoStampAdjust;
    std::shared_ptr<StampAdjust> _audioStampAdjust;

    QueType::Ptr _ring;
    DataType _cache;

    std::vector<UrlParser> _streamList;

    std::mutex _mtxTrack;
    std::unordered_map<int/*index*/, RtspTrack::Ptr> _mapRtspTrack;
    std::unordered_map<int/*index*/, StampAdjust::Ptr> _mapStampAdjust;

    std::unordered_map<std::string, int> _mapControl2Index;
};


#endif //RtspPollingMediaSource_H
