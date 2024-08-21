#ifndef TsMediaSource_H
#define TsMediaSource_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>

#include "Common/MediaSource.h"
#include "TsMuxer.h"
#include "TsDemuxer.h"
#include "Common/DataQue.h"

using namespace std;

class TsMediaSource : public MediaSource
{
public:
    using Ptr = shared_ptr<TsMediaSource>;
    using Wptr = weak_ptr<TsMediaSource>;
    using RingDataType = shared_ptr<list<StreamBuffer::Ptr> >;
    using RingType = DataQue<RingDataType>;

    TsMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr, bool muxer = false);
    ~TsMediaSource();

public:
    void addTrack(const TsDemuxer::Ptr& track);
    void addDecodeTrack(const shared_ptr<TrackInfo>& track);
    void addTrack(const shared_ptr<TrackInfo>& track) override;
    void addSink(const MediaSource::Ptr &src) override;
    void delSink(const MediaSource::Ptr &src) override;
    void onFrame(const FrameBuffer::Ptr& frame) override;
    void onReady() override;
    int playerCount() override;
    void getClientList(const function<void(const list<ClientInfo>& info)>& func) override;
    uint64_t getBytes() override { return _ring ? _ring->getBytes() : 0;}
    unordered_map<int/*index*/, TsDemuxer::Ptr> getDecodeTrack()
    {
        return _mapTsDecodeTrack;
    }

    TsMuxer::Ptr getEncodeTrack()
    {
        return _tsEncodeTrack;
    }

    RingType::Ptr getRing() {return _ring;}

    void inputTs(const StreamBuffer::Ptr& buffer);
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

    TsMuxer::Ptr _tsEncodeTrack;
    mutex _mtxTrack;
    unordered_map<int/*index*/, TsDemuxer::Ptr> _mapTsDecodeTrack;
};


#endif //GB28181MediaSource_H
