#include "FrameMediaSource.h"
#include "Log/Logger.h"

using namespace std;

FrameMediaSource::FrameMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop)
    : MediaSource(urlParser, loop)
{
    // _cache = std::make_shared<toolkit::List<Frame::Ptr>>();
    // TODO get it from config
    _ring = std::make_shared<FrameRingType>(250, nullptr);
    logInfo << "create frame ring: " << _ring;
}

FrameMediaSource::~FrameMediaSource()
{
    logInfo << "~FrameMediaSource";
}

void FrameMediaSource::onFrame(const FrameBuffer::Ptr& frame)
{
    // for (auto& sink : _mapSink) {
        // logInfo << "on frame to sink";
        bool keyframe = false;
        if (frame->getTrackType() == VideoTrackType) {
            if (frame->startFrame()) {
                keyframe = true;
            }
        }
        logInfo << "keyframe: " << keyframe << ", size: " << frame->size() << ", type: " << (int)frame->getNalType();
        _ring->write(frame, keyframe);
    // }
}

void FrameMediaSource::addTrack(const shared_ptr<TrackInfo>& track)
{
    logInfo << "on add track to sink";
    MediaSource::addTrack(track);

    for (auto& sink : _mapSink) {
        logInfo << "on add track to sink";
    //     if (sink.second.lock()) {
    //         sink.second.lock()->addTrack(track);
    //     }
        sink.second->addTrack(track);
    }
}

void FrameMediaSource::addSink(const MediaSource::Ptr &sink)
{
    MediaSource::addSink(sink);

    for (auto& track: _mapTrackInfo) {
        if (!track.second) {
            break;
        }
        logInfo << "on add track to sink";
        sink->addTrack(track.second);
    }
    sink->onReady();
    weak_ptr<FrameMediaSource> weak_self = static_pointer_cast<FrameMediaSource>(shared_from_this());
    if (!_ring) {
            auto lam = [weak_self](int size) {
            auto strongSelf = weak_self.lock();
            if (!strongSelf) {
                return;
            }
            // strongSelf->onReaderChanged(size);
        };
        _ring = std::make_shared<FrameRingType>(_ring_size, std::move(lam));
    }
    _ring->addOnWrite(sink.get(), [weak_self, sink](FrameRingDataType in, bool is_key){
        auto strong_self = weak_self.lock();
        if (!strong_self) {
            return ;
        }
        // logInfo << "is key: " << is_key;
        sink->onFrame(in);
        // for (auto& sinkW: strong_self->_mapSink) {
        //     // auto sink = sinkW.second.lock();
        //     auto sink = sinkW.second;
        //     if (sink) {
        //         // logInfo << "sink->onFrame(in)";
        //         sink->onFrame(in);
        //     }
        // }
    });
}

void FrameMediaSource::delSink(const MediaSource::Ptr& sink)
{
    logInfo << "FrameMediaSource::delSink";
    MediaSource::delSink(sink);
    _ring->delOnWrite(sink.get());
    if (_mapSink.size() == 0) {
        for (auto& source : _mapSource) {
            source.second->delSink(static_pointer_cast<FrameMediaSource>(shared_from_this()));
        }
        release();
    }
}


