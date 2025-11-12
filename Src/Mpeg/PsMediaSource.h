#ifndef PsMediaSource_H
#define PsMediaSource_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>

#include "Common/MediaSource.h"
#include "PsMuxer.h"
#include "PsDemuxer.h"
#include "Common/DataQue.h"

// using namespace std;

class PsMediaSource : public MediaSource
{
public:
    using Ptr = std::shared_ptr<PsMediaSource>;
    using Wptr = std::weak_ptr<PsMediaSource>;
    using RingDataType = std::shared_ptr<std::list<FrameBuffer::Ptr> >;
    using RingType = DataQue<RingDataType>;

    PsMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr, bool muxer = false);
    ~PsMediaSource();

public:
    void addTrack(const PsDemuxer::Ptr& track);
    void addDecodeTrack(const std::shared_ptr<TrackInfo>& track);
    void addTrack(const std::shared_ptr<TrackInfo>& track) override;
    void addSink(const MediaSource::Ptr &src) override;
    void delSink(const MediaSource::Ptr &src) override;
    void onFrame(const FrameBuffer::Ptr& frame) override;
    void onReady() override;
    int playerCount() override;
    void getClientList(const std::function<void(const std::list<ClientInfo>& info)>& func) override;
    uint64_t getBytes() override { return _ring ? _ring->getBytes() : 0;}
    std::unordered_map<int/*index*/, PsDemuxer::Ptr> getDecodeTrack()
    {
        return _mapPsDecodeTrack;
    }

    PsMuxer::Ptr getEncodeTrack()
    {
        return _psEncodeTrack;
    }

    RingType::Ptr getRing() {return _ring;}

    void inputPs(const FrameBuffer::Ptr& buffer);
    void stopDecode();
    void startDecode();

    void startEncode();
    void stopEncode();

private:
    bool _muxer;
    bool _decodeFlag = false;
    bool _encodeFlag = false;
    int _ringSize = 512;

    RingType::Ptr _ring;
    RingDataType _cache;

    PsMuxer::Ptr _psEncodeTrack;
    std::mutex _mtxTrack;
    std::unordered_map<int/*index*/, PsDemuxer::Ptr> _mapPsDecodeTrack;
};


#endif //GB28181MediaSource_H
