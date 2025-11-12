#ifndef TsDemuxer_H
#define TsDemuxer_H

#include <string>
#include <unordered_map>

#include "Common/Track.h"
#include "Net/Buffer.h"
#include "Common/Frame.h"
#include "Util/TimeClock.h"
#include "Mpeg.h"

enum TSPidTable
{
    // Program Association Table (see Table 2-25)
    TSPidTablePAT = 0x00,
    // Conditional Access Table (see Table 2-27)
    TSPidTableCAT = 0x01,
    // Transport Stream Description Table
    TSPidTableTSDT = 0x02,
    // null packets (see Table 2-3)
    TSPidTableNULL = 0x01FFF
};

enum TsAdaptationType
{
    TsAdaptationTypeReserved = 0x00,
    TsAdaptationTypePayloadOnly = 0x01,
    TsAdaptationTypeAdaptationOnly = 0x02,
    TsAdaptationTypeBoth = 0x03,
};

/**
 * Table 2-18 – Stream_id assignments. page 52.
 */
enum TSPESStreamId
{
    PES_program_stream_map = 0b10111100, // 0xbc
    PES_private_stream_1 = 0b10111101,   // 0xbd
    PES_padding_stream = 0b10111110,     // 0xbe
    PES_private_stream_2 = 0b10111111,   // 0xbf

    // 110x xxxx
    // ISO/IEC 13818-3 or ISO/IEC 11172-3 or ISO/IEC 13818-7 or ISO/IEC
    // 14496-3 audio stream number x xxxx
    // (stream_id>>5)&0x07 == PES_audio_prefix
    PES_audio_prefix = 0b110,

    // 1110 xxxx
    // ITU-T Rec. H.262 | ISO/IEC 13818-2 or ISO/IEC 11172-2 or ISO/IEC
    // 14496-2 video stream number xxxx
    // (stream_id>>4)&0x0f == PES_audio_prefix
    PES_video_prefix = 0b1110,

    PES_ECM_stream = 0b11110000,           // 0xf0
    PES_EMM_stream = 0b11110001,           // 0xf1
    PES_DSMCC_stream = 0b11110010,         // 0xf2
    PES_13522_stream = 0b11110011,         // 0xf3
    PES_H_222_1_type_A = 0b11110100,       // 0xf4
    PES_H_222_1_type_B = 0b11110101,       // 0xf5
    PES_H_222_1_type_C = 0b11110110,       // 0xf6
    PES_H_222_1_type_D = 0b11110111,       // 0xf7
    PES_H_222_1_type_E = 0b11111000,       // 0xf8
    PES_ancillary_stream = 0b11111001,     // 0xf9
    PES_SL_packetized_stream = 0b11111010, // 0xfa
    PES_FlexMux_stream = 0b11111011,       // 0xfb
    // reserved data stream
    // 1111 1100 … 1111 1110
    PES_program_stream_directory = 0b11111111, // 0xff
};

enum TsPidType
{
    TsPidTypeReserved,
    TsPidTypePMT,
    TsPidTypeVideo,
    TsPidTypeAudio,
};

struct TsPidInfo
{
    TsPidType pid_type_{TsPidTypeReserved};
    TSPidTable pid_{TSPidTable::TSPidTableNULL};
    uint8_t seq_{16};
};

class TsMessage
{
public:
    TSPidTable pid_{TSPidTable::TSPidTableNULL};
    TsPidType type_{TsPidType::TsPidTypeReserved};
    int stream_type_{STREAM_TYPE_RESERVED};

    uint16_t PES_packet_length_{0};
    uint8_t stream_id_{0};

    int32_t packet_start_code_prefix_{0};
    uint64_t pts_{0};
    uint64_t dts_{0};

    int32_t packet_data_size_{0};
    int32_t parsed_data_size_{0};
    StringBuffer packet_data_;

    void append(int8_t *p, int32_t size);
    bool is_video();
    bool isEnd();
};

class TsDemuxer
{
public:
    using Ptr = std::shared_ptr<TsDemuxer>;
    TsDemuxer();
    ~TsDemuxer();

public:
    // int64_t  parsePsTimestamp(const uint8_t* p);
    virtual void onTsPacket(char* ps_data, int ps_size, uint32_t timestamp);

    void setOnDecode(const std::function<void(const FrameBuffer::Ptr& frame)>& cb);
    void onDecode(const char* data, int len, int index, uint64_t pts, uint64_t dts);
    void addTrackInfo(const std::shared_ptr<TrackInfo>& trackInfo);
    void setOnTrackInfo(const std::function<void(const std::shared_ptr<TrackInfo>& trackInfo)>& cb);
    void setOnReady(const std::function<void()>& cb);

    std::shared_ptr<TsPidInfo> pidInfo(int16_t pid);
    void pushPidInfo(TSPidTable pidtable, TsPidType type);
    std::shared_ptr<TsMessage> message(TSPidTable pidtable);
    void pushConsumerMessage(std::shared_ptr<TsMessage> message);
    void createTrackInfo(const std::string& codec, int type);
    void clear();

private:
    bool _hasAudio = false;
    bool _hasVideo = false;
    bool _firstAac = true;
    bool _hasReady = false;
    bool _firstVps = true;
    bool _firstSps = true;
    bool _firstPps = true;
    uint8_t _waitPackets = 0;
    uint8_t _audio_es_type = 0;
    uint8_t _video_es_type = 0;
    int64_t _lastVideoPts = -1;
    std::string _audioCodec = "unknwon";
    std::string _videoCodec = "unknwon";
    TimeClock _timeClock;
    StringBuffer _remainBuffer;
    StringBuffer _videoStream;
    std::unordered_map<int, std::shared_ptr<TrackInfo>> _mapTrackInfo;
    std::function<void(const FrameBuffer::Ptr& frame)> _onFrame;
    std::function<void(const std::shared_ptr<TrackInfo>& trackInfo)> _onTrackInfo;
    std::function<void()> _onReady;

    
    std::unordered_map<int16_t, std::shared_ptr<TsPidInfo>> pidInfos_;
    std::unordered_map<int16_t, std::shared_ptr<TsMessage>> msgs_;
};

#endif //PsDemuxer_H

