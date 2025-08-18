#include "Fmp4Demuxer.h"
#include "Util/String.h"
#include "Codec/AacTrack.h"
#include "Codec/G711Track.h"
#include "Codec/H264Frame.h"
#include "Codec/H264Track.h"
#include "Codec/H265Frame.h"
#include "Codec/H266Frame.h"
#include "Codec/H265Track.h"
#include "Log/Logger.h"

#include <arpa/inet.h>
#include <functional>

using namespace std;

Fmp4Demuxer::Fmp4Demuxer()
{}

Fmp4Demuxer::~Fmp4Demuxer()
{}

void Fmp4Demuxer::write(const char* data, int size)
{
    // _file.write(data, size);
}

void Fmp4Demuxer::read(char* data, int size)
{
    data = _buffer->data() + _offset;
}

void Fmp4Demuxer::seek(uint64_t offset)
{
    _offset = offset;
}

size_t Fmp4Demuxer::tell()
{
    return _offset;
}

// bool Fmp4Demuxer::open()
// {
//     if (!_file.open(_filepath, "rb+")) {
//         logWarn << "open mp4 file failed: " << _filepath;
//         return false;
//     }

//     return true;
// }

void Fmp4Demuxer::onFrame(const StreamBuffer::Ptr& buffer, int trackIndex, int pts, int dts, bool keyframe)
{
    if (_mapTrackInfo.find(trackIndex) == _mapTrackInfo.end()) {
        return ;
    }

    auto trackInfo = _mapTrackInfo[trackIndex];
    FrameBuffer::Ptr frame;
    if (trackInfo->codec_ == "h265" || trackInfo->codec_ == "h264" || trackInfo->codec_ == "h266") {
        uint32_t offset = 0;
        uint64_t bytes = buffer->size();
        auto data = buffer->data();

        while (offset < bytes) {
            uint32_t frame_len;
            memcpy(&frame_len, data + offset, 4);
            frame_len = ntohl(frame_len);
            if (frame_len + offset + 4 > bytes) {
                return ;
            }
            memcpy(data + offset, "\x0\x0\x0\x1", 4);
            if (trackInfo->codec_ == "h265" ) {
                frame = make_shared<H265Frame>();
            } else if (trackInfo->codec_ == "h264") {
                frame = make_shared<H264Frame>();
            } else if (trackInfo->codec_ == "h266") {
                frame = make_shared<H266Frame>();
            }
            frame->_buffer.assign("\x0\x0\x0\x1", 4);
            frame->_buffer.append(data + offset + 4, frame_len);
            frame->_trackType = VideoTrackType;
            frame->_startSize = 4;
            frame->_pts = pts;
            frame->_index = trackIndex;
            frame->_dts = dts;
            frame->_codec = trackInfo->codec_;

            if (_onFrame) {
                _onFrame(frame);
            }

            offset += (frame_len + 4);
        }
        // frame = make_shared<H265Frame>();
        // frame->_buffer.assign("\x0\x0\x0\x1", 4);
        // frame->_buffer.append(buffer->data(), buffer->size());
        // frame->_trackType = VideoTrackType;
        // frame->_startSize = 4;
        // frame->_pts = pts;
        // frame->_index = trackIndex;
        // frame->_dts = dts;
        // frame->_codec = trackInfo->codec_;

        return ;
    } else if (trackInfo->codec_ == "h264") {
        // frame = make_shared<H264Frame>();
        // frame->_buffer.assign("\x0\x0\x0\x1", 4);
        // frame->_buffer.append(buffer->data(), buffer->size());
        // frame->_trackType = VideoTrackType;
        // frame->_startSize = 4;
        // frame->_pts = pts;
        // frame->_index = trackIndex;
        // frame->_dts = dts;
        // frame->_codec = trackInfo->codec_;
    } else if (trackInfo->codec_ == "aac") {
        auto aacTrack = dynamic_pointer_cast<AacTrack>(trackInfo);
        frame = make_shared<FrameBuffer>();
        frame->_buffer.assign(aacTrack->getAacInfo());
        frame->_buffer.append(buffer->data(), buffer->size());
        frame->_trackType = AudioTrackType;
        frame->_startSize = 7;
        frame->_pts = pts;
        frame->_index = trackIndex;
        frame->_dts = dts;
        frame->_codec = trackInfo->codec_;
    } else if (trackInfo->codec_ == "g711a" || trackInfo->codec_ == "g711u" || trackInfo->codec_ == "mp3") {
        frame = make_shared<FrameBuffer>();
        frame->_buffer.assign(buffer->data(), buffer->size());
        frame->_trackType = AudioTrackType;
        frame->_startSize = 0;
        frame->_pts = pts;
        frame->_index = trackIndex;
        frame->_dts = dts;
        frame->_codec = trackInfo->codec_;
    }

    if (_onFrame) {
        _onFrame(frame);
    }
}

void Fmp4Demuxer::onTrackInfo(const TrackInfo::Ptr& trackInfo)
{
    _mapTrackInfo.emplace(trackInfo->index_, trackInfo);
    if (_onTrackInfo) {
        _onTrackInfo(trackInfo);
    }
}

void Fmp4Demuxer::onReady()
{
    if (_onReady) {
        _onReady();
    }
}

void Fmp4Demuxer::setOnFrame(const std::function<void (const FrameBuffer::Ptr &frame)>& cb)
{
    _onFrame = cb;
}

void Fmp4Demuxer::setOnReady(const function<void()>& cb)
{
    _onReady = cb;
}

void Fmp4Demuxer::setOnTrackInfo(const std::function<void (const TrackInfo::Ptr &trackInfo)> &cb)
{
    _onTrackInfo = cb;
}