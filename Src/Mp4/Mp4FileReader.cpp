#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "Mp4FileReader.h"
#include "Logger.h"
#include "Util/String.h"
#include "Codec/H264Frame.h"
#include "Codec/H265Frame.h"
#include "Codec/AacTrack.h"

using namespace std;

Mp4FileReader::Mp4FileReader(const string& filepath)
    :MP4Demuxer()
    ,_filepath(filepath)
{

}

Mp4FileReader::~Mp4FileReader()
{}

void Mp4FileReader::write(const char* data, int size)
{
    _file.write(data, size);
}

void Mp4FileReader::read(char* data, int size)
{
    _file.read(data, size);
}

void Mp4FileReader::seek(uint64_t offset)
{
    _file.seek(offset);
}

size_t Mp4FileReader::tell()
{
    return _file.tell();
}

bool Mp4FileReader::open()
{
    if (!_file.open(_filepath, "rb+")) {
        logWarn << "open mp4 file failed: " << _filepath;
        return false;
    }

    return true;
}

void Mp4FileReader::onFrame(const StreamBuffer::Ptr& buffer, int trackIndex, int pts, int dts, bool keyframe)
{
    if (_mapTrackInfo.find(trackIndex) == _mapTrackInfo.end()) {
        return ;
    }

    auto trackInfo = _mapTrackInfo[trackIndex];
    FrameBuffer::Ptr frame;
    if (trackInfo->codec_ == "h265" || trackInfo->codec_ == "h264") {
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
            } else {
                frame = make_shared<H264Frame>();
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
    } else if (trackInfo->codec_ == "g711a" || trackInfo->codec_ == "g711u") {
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

void Mp4FileReader::onTrackInfo(const TrackInfo::Ptr& trackInfo)
{
    _mapTrackInfo.emplace(trackInfo->index_, trackInfo);
    if (_onTrackInfo) {
        _onTrackInfo(trackInfo);
    }
}

void Mp4FileReader::onReady()
{
    if (_onReady) {
        _onReady();
    }
}

void Mp4FileReader::setOnFrame(const std::function<void (const FrameBuffer::Ptr &frame)>& cb)
{
    _onFrame = cb;
}

void Mp4FileReader::setOnReady(const function<void()>& cb)
{
    _onReady = cb;
}

void Mp4FileReader::setOnTrackInfo(const std::function<void (const TrackInfo::Ptr &trackInfo)> &cb)
{
    _onTrackInfo = cb;
}
