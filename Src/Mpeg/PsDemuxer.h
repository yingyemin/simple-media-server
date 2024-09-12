#ifndef PsDemuxer_H
#define PsDemuxer_H

#include <arpa/inet.h>
#include <string>
#include <unordered_map>

#include "Common/Track.h"
#include "Net/Buffer.h"
#include "Common/Frame.h"
#include "Util/TimeClock.h"

// #pragma pack(1)

class PsDemuxer
{
public:
    using Ptr = shared_ptr<PsDemuxer>;
    PsDemuxer();
    ~PsDemuxer(){}
public:
    // gb28181 program stream struct define
    struct PsPacketStartCode
    {
        uint8_t start_code[3];
        uint8_t stream_id[1];
    }__attribute__((packed));

    struct PsPacketHeader
    {
        PsPacketStartCode start;// 4
        uint8_t info[9];
        uint8_t stuffingLength;
    }__attribute__((packed));

    struct PsPacketBBHeader
    {
        PsPacketStartCode start;
        uint16_t    length;
    }__attribute__((packed));

    struct PsePacket
    {
        PsPacketStartCode     start;
        uint16_t    length;
        uint8_t         info[2];
        uint8_t         stuffingLength;
    }__attribute__((packed));

    struct PsMapPacket
    {
        PsPacketStartCode  start;
        uint16_t length;
    }__attribute__((packed));

public:
    int64_t  parsePsTimestamp(const uint8_t* p);
    virtual int onPsStream(char* ps_data, int ps_size, uint32_t timestamp, uint32_t ssrc);

    void setOnDecode(const function<void(const FrameBuffer::Ptr& frame)> cb);
    void onDecode(const FrameBuffer::Ptr& data, int index, int pts, int dts);
    void addTrackInfo(const shared_ptr<TrackInfo>& trackInfo);
    void setOnTrackInfo(const function<void(const shared_ptr<TrackInfo>& trackInfo)>& cb);
    void setOnReady(const function<void()>& cb);
    FrameBuffer::Ptr createFrame(int index);

    void clear();

private:
    bool _hasAudio = false;
    bool _hasVideo = false;

    bool _hasReady = false;
    bool _hasVideoReady = false;
    // bool _hasAudioReady = false;

    bool _firstAac = true;
    bool _firstVps = true;
    bool _firstSps = true;
    bool _firstPps = true;
    uint8_t _waitPackets = 0;
    uint8_t _audio_es_type = 0;
    uint8_t _video_es_type = 0;
    int64_t _lastVideoPts = -1;
    string _audioCodec = "unknown";
    string _videoCodec = "unknown";
    TimeClock _timeClock;
    StringBuffer _remainBuffer;
    StringBuffer _videoStream;
    FrameBuffer::Ptr _videoFrame;
    unordered_map<int, shared_ptr<TrackInfo>> _mapTrackInfo;
    function<void(const FrameBuffer::Ptr& frame)> _onFrame;
    function<void(const shared_ptr<TrackInfo>& trackInfo)> _onTrackInfo;
    function<void()> _onReady;
};

// #pragma pack()

#endif //PsDemuxer_H

