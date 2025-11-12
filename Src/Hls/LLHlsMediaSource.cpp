#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "LLHlsMediaSource.h"
#include "Logger.h"
#include "Util/String.hpp"

using namespace std;

LLHlsMediaSource::LLHlsMediaSource(const UrlParser& urlParser, const EventLoop::Ptr& loop, bool muxer)
    :MediaSource(urlParser, loop)
    ,_muxer(muxer)
{
}

LLHlsMediaSource::~LLHlsMediaSource()
{
    logInfo << "~LLHlsMediaSource";
    
    if (_hlsMuxer) {
        _hlsMuxer->release();
    }
}

void LLHlsMediaSource::onReady()
{
    // MediaSource::onReady();
}

void LLHlsMediaSource::onHlsReady()
{
    MediaSource::onReady();
}

void LLHlsMediaSource::addTrack(const shared_ptr<TrackInfo>& track)
{
    MediaSource::addTrack(track);
    
    std::weak_ptr<LLHlsMediaSource> weakSelf = std::static_pointer_cast<LLHlsMediaSource>(shared_from_this());
    if (!_hlsMuxer) {
        _hlsMuxer = make_shared<LLHlsMuxer>(_urlParser);
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

    _hlsMuxer->start();
}


void LLHlsMediaSource::onFrame(const FrameBuffer::Ptr& frame)
{
    if (_hlsMuxer) {
        _hlsMuxer->onFrame(frame);
    }
}

string LLHlsMediaSource::getM3u8(void* key)
{
    if (_hlsMuxer) {
        return _hlsMuxer->getM3u8(key);
    }

    return "";
}
    
FrameBuffer::Ptr LLHlsMediaSource::getTsBuffer(const string& key)
{
    if (_hlsMuxer) {
        return _hlsMuxer->getTsBuffer(key);
    }

    return nullptr;
}