#ifndef Fmp4MediaSource_H
#define Fmp4MediaSource_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <mutex>

#include "Common/MediaSource.h"
#include "Fmp4Muxer.h"
#include "Fmp4Demuxer.h"
#include "Common/DataQue.h"

using namespace std;

class Fmp4MediaSource : public MediaSource
{
public:
    using Ptr = shared_ptr<Fmp4MediaSource>;
    using Wptr = weak_ptr<Fmp4MediaSource>;
    using RingDataType = Buffer::Ptr;
    using RingType = DataQue<RingDataType>;

    Fmp4MediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop = nullptr, bool muxer = false);
    ~Fmp4MediaSource();

public:
    void addTrack(const Fmp4Demuxer::Ptr& track);
    void addDecodeTrack(const shared_ptr<TrackInfo>& track);
    void addTrack(const shared_ptr<TrackInfo>& track) override;
    void addSink(const MediaSource::Ptr &src) override;
    void delSink(const MediaSource::Ptr &src) override;
    void onFrame(const FrameBuffer::Ptr& frame) override;
    void onReady() override;
    int playerCount() override;
    void getClientList(const function<void(const list<ClientInfo>& info)>& func) override;
    uint64_t getBytes() override { return _ring ? _ring->getBytes() : 0;}
    unordered_map<int/*index*/, Fmp4Demuxer::Ptr> getDecodeTrack()
    {
        return _mapFmp4DecodeTrack;
    }

    Fmp4Muxer::Ptr getEncodeTrack()
    {
        return _fmp4EncodeTrack;
    }

    Buffer::Ptr getFmp4Header() { return _fmp4Header; }

    RingType::Ptr getRing() {return _ring;}

    void inputFmp4(const Buffer::Ptr& buffer);
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

    Buffer::Ptr _fmp4Header;
    Fmp4Muxer::Ptr _fmp4EncodeTrack;
    mutex _mtxTrack;
    unordered_map<int/*index*/, Fmp4Demuxer::Ptr> _mapFmp4DecodeTrack;
};


#endif //Fmp4MediaSource_H
