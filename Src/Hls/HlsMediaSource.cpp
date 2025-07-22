#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "HlsMediaSource.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

HlsMediaSource::HlsMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop, bool muxer)
    :MediaSource(urlParser, loop)
    ,_muxer(muxer)
{
}

HlsMediaSource::~HlsMediaSource()
{
    logInfo << "~HlsMediaSource";
    if (_hlsMuxer) {
        _hlsMuxer->release();
    }
}

void HlsMediaSource::onReady()
{
    // MediaSource::onReady();
    
    _hlsMuxer->start();
}

int HlsMediaSource::playerCount()
{
    return _hlsMuxer->playerCount();
}

void HlsMediaSource::onHlsReady()
{
    MediaSource::onReady();
}

void HlsMediaSource::addTrack(const shared_ptr<TrackInfo>& track)
{
    MediaSource::addTrack(track);
    
    std::weak_ptr<HlsMediaSource> weakSelf = std::static_pointer_cast<HlsMediaSource>(shared_from_this());
    if (!_hlsMuxer) {
        _hlsMuxer = make_shared<HlsMuxer>(_urlParser);
    }
    _hlsMuxer->addTrackInfo(track);
    _hlsMuxer->setOnNoPlayer([weakSelf](){
        auto self = weakSelf.lock();
        if (!self) {
            return ;
        }

        self->onReaderChanged(0);
    });

    _hlsMuxer->setOnReady([weakSelf](){
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }
        self->onHlsReady();
    });

    _hlsMuxer->setOnDelConnection([weakSelf](void* key){
        auto self = weakSelf.lock();
        if (!self) {
            return;
        }
        self->delConnection(key);
    });
}


void HlsMediaSource::onFrame(const FrameBuffer::Ptr& frame)
{
    if (_hlsMuxer) {
        _hlsMuxer->onFrame(frame);
    }
}

string HlsMediaSource::getM3u8(void* key)
{
    if (_hlsMuxer) {
        return _hlsMuxer->getM3u8(key);
    }

    return "";
}
    
FrameBuffer::Ptr HlsMediaSource::getTsBuffer(const string& key)
{
    if (_hlsMuxer) {
        return _hlsMuxer->getTsBuffer(key);
    }

    return nullptr;
}