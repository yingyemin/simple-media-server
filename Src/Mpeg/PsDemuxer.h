#ifndef PsDemuxer_H
#define PsDemuxer_H

#if defined(_WIN32)
#include "Util/Util.h"
#else
#include <arpa/inet.h>
#endif
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
    using Ptr = std::shared_ptr<PsDemuxer>;
    PsDemuxer();
    ~PsDemuxer(){}
public:
#if defined(_WIN32)
    // ��ʼ���� 1 �ֽڶ���
    #pragma pack(push, 1)
    // gb28181 program stream struct define
    struct PsPacketStartCode
    {
        uint8_t start_code[3];
        uint8_t stream_id[1];
    };

    struct PsPacketHeader
    {
        PsPacketStartCode start;// 4
        uint8_t info[9];
        uint8_t stuffingLength;
    };

    struct PsPacketBBHeader
    {
        PsPacketStartCode start;
        uint16_t    length;
    };

    struct PsePacket
    {
        PsPacketStartCode     start;
        uint16_t    length;
        uint8_t         info[2];
        uint8_t         stuffingLength;
    };

    struct PsMapPacket
    {
        PsPacketStartCode  start;
        uint16_t length;
    };
    #pragma pack(pop)
#else
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
#endif

public:
    uint64_t  parsePsTimestamp(const uint8_t* p);
    virtual int onPsStream(char* ps_data, int ps_size, uint32_t timestamp, uint32_t ssrc, bool live = false);
    virtual int seek(char* ps_data, int ps_size, uint32_t timestamp, uint32_t ssrc, bool live = false);

    void setOnDecode(const std::function<void(const FrameBuffer::Ptr& frame)>& cb);
    void onDecode(const FrameBuffer::Ptr& data, int index, uint64_t pts, uint64_t dts);
    void addTrackInfo(const std::shared_ptr<TrackInfo>& trackInfo);
    void setOnTrackInfo(const std::function<void(const std::shared_ptr<TrackInfo>& trackInfo)>& cb);
    void setOnReady(const std::function<void()>& cb);
    void onReady(int trackType);
    FrameBuffer::Ptr createFrame(int index);

    void clear();

private:
    // bool _hasAudio = false;
    // bool _hasVideo = false;

    // bool _hasReady = false;
    // bool _hasVideoReady = false;
    // bool _hasAudioReady = false;

    // bool _firstAac = true;
    // bool _firstVps = true;
    // bool _firstSps = true;
    // bool _firstPps = true;
    bool _newPs = false;
    uint8_t _waitPackets = 0;
    uint8_t _audio_es_type = 0;
    uint8_t _video_es_type = 0;
    uint8_t _audio_es_id = 0;
    uint8_t _video_es_id = 0;
    uint64_t _lastVideoPts = -1;
    std::string _audioCodec = "unknown";
    std::string _videoCodec = "unknown";
    TimeClock _timeClock;
    StringBuffer _remainBuffer;
    StringBuffer _videoStream;
    FrameBuffer::Ptr _videoFrame;
    std::unordered_map<int, std::shared_ptr<TrackInfo>> _mapTrackInfo;
    std::function<void(const FrameBuffer::Ptr& frame)> _onFrame;
    std::function<void(const std::shared_ptr<TrackInfo>& trackInfo)> _onTrackInfo;
    std::function<void()> _onReady;
};

// #pragma pack()

#endif //PsDemuxer_H

