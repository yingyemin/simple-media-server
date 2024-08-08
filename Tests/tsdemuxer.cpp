#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <cassert>
#include <map>
#include <list>

#include <algorithm>
// 用来处理字符串流的
// 尤其是在解析各种格式的时候
// 因为需要按照字节处理

// reference SRS research/hls code
const static bool kPrintTSPacketHeader = true;
const static bool kPrintTsPacketAdapationField = true;
const static bool kPrintTsPacketPayloadPES = true;
const static bool kPrintTsPacketPayloadPAT = false;
const static bool KPrintTsPacketPayloadPMT = false;
const static bool kPrintTsAacAdts = true;
class Debug
{
public:
    ~Debug()
    {
        std::cout << buf_.str() << std::endl;
    }
    template <class T>
    Debug &operator<<(const T &value)
    {
        stream_ << value << " ";

        return *this;
    }

private:
    std::stringbuf buf_;
    std::ostream stream_{&buf_};
};

template <>
Debug &Debug::operator<<<uint8_t>(const uint8_t &value)
{
    stream_ << (int)value << " ";
    return *this;
}

template <>
Debug &Debug::operator<<<int8_t>(const int8_t &value)
{
    stream_ << (int)value << " ";
    return *this;
}

class BufferStream
{
public:
    BufferStream()
    {
    }
    BufferStream(char *buf, int32_t size)
        : header_(buf), pos_(buf), size_(size) {}

public:
    void init(char *buf, int32_t size)
    {
        header_ = pos_ = buf;
        size_ = size;
    }
    void reset()
    {
        pos_ = header_;
    }
    bool empty()
    {
        return !header_ || !pos_ || pos_ >= (header_ + size_);
    }
    bool skip(int32_t size)
    {
        pos_ += size;
        return true;
    }
    bool require(int size)
    {
        return !empty() && size <= (header_ + size_ - pos_);
    }
    int pos()
    {
        return pos_ - header_;
    }
    int32_t size()
    {
        return size_;
    }

public:
    int8_t read1Bytes()
    {
        // 断言保证能从buf中读取到1个字节
        assert(require(1));
        int8_t data = 0;
        char *p = (char *)&data;
        *p++ = *pos_++;
        return data;
    }
    int16_t read2Bytes()
    {
        assert(require(2));
        int16_t data = 0;
        char *p = (char *)&data;
        p[1] = *pos_++;
        p[0] = *pos_++;
        return data;
    }

    int32_t read3Bytes()
    {
        assert(require(3));
        int32_t data;
        char *p = (char *)&data;
        p[2] = *pos_++;
        p[1] = *pos_++;
        p[0] = *pos_++;
        return data;
    }

    int32_t read4Bytes()
    {
        assert(require(4));
        int32_t data = 0;
        char *p = (char *)&data;
        p[3] = *pos_++;
        p[2] = *pos_++;
        p[1] = *pos_++;
        p[0] = *pos_++;
        return data;
    }

    int64_t read5Bytes()
    {
        assert(require(4));
        int64_t data = 0;
        char *p = (char *)&data;
        p[4] = *pos_++;
        p[3] = *pos_++;
        p[2] = *pos_++;
        p[1] = *pos_++;
        p[0] = *pos_++;
        return data;
    }

    int64_t read6Bytes()
    {
        assert(require(5));
        int64_t data = 0;
        char *p = (char *)&data;
        p[5] = *pos_++;
        p[4] = *pos_++;
        p[3] = *pos_++;
        p[2] = *pos_++;
        p[1] = *pos_++;
        p[0] = *pos_++;
        return data;
    }

    void read(char *buff, int size)
    {
        assert(require(size));
        memcpy(buff, pos_, size);
        pos_ += size;
    }

private:
    char *header_{nullptr};
    char *pos_{nullptr};
    int32_t size_{0};
};
class TsFileBuffer
{
public:
    TsFileBuffer() = default;
    ~TsFileBuffer()
    {
        close();
    }

public:
    bool open(const std::string &fileName)
    {
        if (tsFstream_.is_open())
        {
            tsFstream_.close();
        }
        tsFstream_.open(fileName, std::ios::in | std::ios::binary);
        return tsFstream_.is_open();
    }

    bool close()
    {
        if (tsFstream_.is_open())
        {
            tsFstream_.close();
        }
        return tsFstream_.is_open();
    }

    void read(char *buffer, int count)
    {
        tsFstream_.read(buffer, count);
    }

    void seek(int size)
    {
        tsFstream_.seekg(size, std::ios::cur);
        return;
    }
    bool atEnd()
    {
        return tsFstream_.eof();
    }

private:
    std::ifstream tsFstream_;
};

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

enum TSStreamType
{
    // ITU-T | ISO/IEC Reserved
    TSStreamTypeReserved = 0x00,
    /*defined by ffmpeg*/
    TSStreamTypeVideoMpeg1 = 0x01,
    TSStreamTypeVideoMpeg2 = 0x02,
    TSStreamTypeAudioMpeg1 = 0x03,
    TSStreamTypeAudioMpeg2 = 0x04,
    TSStreamTypePrivateSection = 0x05,
    TSStreamTypePrivateData = 0x06,
    TSStreamTypeAudioAAC = 0x0f,
    TSStreamTypeVideoMpeg4 = 0x10,
    TSStreamTypeVideoH264 = 0x1b,
    TSStreamTypeAudioAC3 = 0x81,
    TSStreamTypeAudioDTS = 0x8a,
};

class TsPacketHeader;
class TsPayload;
class TsPayloadPES;
class TsAdaptationField;
class TsPayloadPAT;
class TsPayloadPMT;

struct TsPidInfo
{
    TsPidType pid_type_{TsPidTypeReserved};
    TSPidTable pid_{TSPidTable::TSPidTableNULL};
};
class TsMessage
{
public:
    TSPidTable pid_{TSPidTable::TSPidTableNULL};
    TsPidType type_{TsPidType::TsPidTypeReserved};
    TSStreamType stream_type_{TSStreamType::TSStreamTypeReserved};

    uint16_t PES_packet_length_{0};
    uint8_t stream_id_{0};

    int32_t packet_start_code_prefix_{0};
    int64_t pts_{0};
    int64_t dts_{0};

    int32_t packet_data_size_{0};
    int32_t parsed_data_size_{0};
    int8_t *packet_data_{nullptr};

    void append(int8_t *p, int32_t size);
    bool is_video();
    bool isEnd();
};
class TsContext
{
public:
    ~TsContext();
    TsPidInfo *pidInfo(int16_t pid);
    void pushPidInfo(TSPidTable pidtable, TsPidType type);
    TsMessage *message(TSPidTable pidtable);
    TsMessage *consumerMessage();
    void pushConsumerMessage(TsMessage *message);

private:
    std::map<int16_t, TsPidInfo *> pidInfos_;
    std::map<int16_t, TsMessage *> msgs_;
    std::list<TsMessage *> consumerMsgs_;
};
class TsPacket
{
public:
    TsPacket();
    ~TsPacket();
    TsPacketHeader *header_{nullptr};
    TsAdaptationField *field_{nullptr};
    TsPayload *payload_{nullptr};

public:
    void demux(TsContext *ctx, BufferStream *stream);
};

class TsPacketHeader
{
public:
    int8_t sync_{0};                         // 8bit, 0x47 TsPacket开始标志位
    int8_t transport_error_indicator_{0};    // 1 bit
    int8_t payload_unit_start_indicator_{0}; // 1 bit
    int8_t transport_priority_{0};           // 1 bit
    int16_t pid_{0};                         // 13 bit
    int8_t transport_scrambling_control_{0}; // 2 bit
    int8_t adaptation_field_control_{0};     // 2 bit
    int8_t continuity_counter_{0};           // 4 bit
public:
    void demux(BufferStream *buffer);
    void print();
};
class TsAdaptationField
{
public:
    uint8_t adaptation_field_length_{0};            // 8bit,长度
    int8_t discontinuity_indicator_{0};             // 1bit 连续性
    int8_t random_access_indicator_{0};             // 1bit 随机访问
    int8_t elementary_stream_priority_indicator{0}; // 1bit 优先级标志位
    int8_t pcr_flag_{0};                            // 1bit pcr标志位，表明当前field包含一个prc field
    int8_t opcr_flag_{0};                           // 1bit opcr标志位，表明当前field包含一个opcr field
    int8_t splicing_point_flag_{0};                 // 1bit splicing_point_flag标志位， 表明当前field包含一个splicing_poing
    int8_t transport_private_data_flag_{0};         // 1bit， 私有数据
    int8_t adaptation_field_extension_flag_{0};     // 1bit, 扩展数据

    // if pcr_flag 6Bytes
    int64_t program_clock_reference_base_{0}; // 33bit
    // 6bit reserved
    int16_t program_clock_reference_extension_{0}; // 9bit
    int64_t pcr_{0};

    // if opcr_flag 6Bytes
    int64_t original_program_clock_reference_base_{0}; // 33bit
    // 6bit reserved
    int16_t original_program_clock_reference_extension_{0}; // 9bit
    int64_t original_pcr_{0};

    // if splicing_point_flag 1Bytes
    int8_t splice_countdown_{0}; // 8bit

    // if transport_private_data_flag
    int8_t transport_private_data_length_{0}; // 8bit
    int8_t *private_data_byte_{nullptr};

    // if adaptation_field_extension_flag
    int8_t adaptation_field_extension_length_{0}; // 8bit
    int8_t ltw_flag_{0};                          // 1bit
    int8_t piecewise_rate_flag_{0};               // 1bit
    int8_t seamless_splice_flag_{0};              // 1bit
    // 5bit resvered
    // if ltw_flag
    int8_t ltw_valid_falg_{0}; // 1bit
    int16_t ltw_offset_{0};    // 15bit

    // if piecewise_rate_flag
    // 2 bit resvered
    int32_t piecewise_rate_{0}; // 22bit

    // if seamless_splice_flag
    int8_t splice_type_{0};  // 4bit
    int8_t dts_next_au0{0};  // 3bit
    int8_t marker_bit0{0};   // 1bit
    int16_t dts_next_au1{0}; // 15bit
    int8_t marker_bit1{0};   // 1bit
    int16_t dts_next_au2{0}; // 15bit
    int8_t marker_bit2{0};   // 1bit
    // left bytes reserved
    int8_t *reserved_{0};

    void print();

    void demux(BufferStream *buffer);
};

class TsPayload
{
public:
    TsPayloadPES *pes_{nullptr};
    TsPayloadPAT *pat_{nullptr};
    TsPayloadPMT *pmt_{nullptr};
    uint8_t pointer_field_{0};

public:
    TsPayload();
    ~TsPayload();
    void read_point_field(TsPacket *pkt, BufferStream *stream);
    void demux(TsContext *ctx, TsPacket *pkt, BufferStream *stream);
};

class TsPayloadPES
{
public:
    int32_t packet_start_code_prefix_{0}; // 24bit
    uint8_t stream_id_{0};                // 8bit (see table 2-18)
    uint16_t PES_packet_length_{0};       // 16bit

    // fixed value, '01' 2bit
    int8_t PES_scrambling_control_{0};    // 2bit
    int8_t PES_priority_{0};              // 1bit
    int8_t data_alignment_indicator_{0};  // 1bit
    int8_t copyright_{0};                 // 1bit
    int8_t original_or_copy_{0};          // 1bit
    int8_t PTS_DTS_flags_{0};             // 2bit
    int8_t ESCR_flag_{0};                 // 1bit
    int8_t ES_rate_flag_{0};              // 1bit
    int8_t DSM_trick_mode_flag_{0};       // 1bit
    int8_t additional_copy_info_flag_{0}; // 1bit
    int8_t PES_CRC_flag_{0};              // 1bit
    int8_t PES_extension_flag_{0};        // 1bit
    uint8_t PES_header_data_length_{0};   // 8bit
    // if PTS_DTS_flags set '10' or '11'
    int64_t PTS_{0}; // 33bit
    int64_t DTS_{0}; // 33bit

    int64_t ESCR_base_{0};       // 33bit
    uint16_t ESCR_extension_{0}; // 9bit
    uint32_t ES_rate_{0};        // 22bit

    uint8_t trick_mode_control_{0}; // 3bit
    int8_t trick_mode_value_{0};    // 5bit

    // if additional_copy_info_flag set 1
    int8_t additional_copy_info_{0}; // 7bit

    // if PES_CRC_flag set 1
    int16_t previous_PES_packet_CRC_{0}; // 16bit

    // if PES_Extension_flag set 1
    int8_t PES_private_data_flag_{0};                // 1bit
    int8_t pack_header_field_flag_{0};               // 1bit
    int8_t program_packet_sequence_counter_flag_{0}; // 1bit
    int8_t P_STD_buffer_flag_{0};                    // 1bit
    // reserved 3bit
    int8_t PES_extension_flag_2_{0}; // 1bit
    // if PES_private_data_flag set 1
    int8_t *PES_private_data_{nullptr}; // 128bit

    // if pack_header_field_flag set 1
    uint8_t pack_field_length_{0}; // 8bit
    // pack_header()
    int8_t *pack_field_{nullptr};

    // if program_packet_sequence_counter_flag set 1
    // marker_bit 1bit
    uint8_t program_packet_sequence_counter_{0}; // 7bit
    // marker_bit 1bit
    int8_t MPEG1_MPEG2_indentifier_{0}; // 1bit
    uint8_t original_stuff_length_{0};  // 6bit

    // if P-STD_buffer_flag set 1
    // '01' 2bit
    int8_t P_STD_buffer_scale_{0};  // 1bit
    uint16_t P_STD_buffer_size_{0}; // 13bit

    // if PES_extension_flag_2 set 1
    // marker_bit 1bit
    uint8_t PES_extension_field_length_{0}; // 7bit
    int8_t *PES_extension_field_reserved_{nullptr};

    int8_t *stuffing_byte_{nullptr};        // size: N1
    int8_t *PES_packet_data_byte_{nullptr}; // size：N2
    int demux(TsContext *ctx, TsPacket *pkt, BufferStream *stream);
    void print();
};

class TsPayloadPAT
{

public:
    uint8_t table_id_{0};                // 8 bit,固定为0x00，标志该表示PAT表
    int8_t section_syntax_indicator_{0}; // 1bit 段语法标志位，固定为1
    int8_t zero_{0};                     // 1bit, 固定值0
    // reserved 2bit
    uint16_t section_length_{0};      // 12bit
    uint16_t transport_stream_id_{0}; // 16bit
    // reserved 2bit
    uint8_t version_number_{0};        // 5bit
    int8_t current_next_indicator_{0}; // 1bit
    uint8_t section_number_{0};        // 8bit
    uint8_t last_section_number_{0};   // 8bit

    // multiple 4B program data.
    // program_number 16bit
    // reserved 2bit
    // 13bits data:0x1fff
    // if program_number program_map_PID 13bit
    // else network_PID 13bytes.
    int program_size_{0};
    int32_t *progreams_{nullptr};

    int32_t CRC_32_{0}; // 32bit
    void demux(TsContext *ctx, BufferStream *stream);
    void print();
};

class TsPayloadPMT
{
public:
    uint8_t table_id_{0};                // 8bit
    int8_t section_syntax_indicator_{0}; // 1bit
    // constBit 0 1bit
    // reserved 2bit
    int16_t section_length_{0}; // 12bit
    int16_t program_number_{0}; // 16bit
    // reserved 2bit
    int8_t version_number_{0};         // 5bit
    int8_t current_next_indicator_{0}; // 1bit
    uint8_t section_number_{0};        // 8bit
    uint8_t last_section_number_{0};   // 8bit
    // reserved 3bit
    uint16_t PCR_PID_{0}; // 13bit
    // reserved 4bit
    uint16_t program_info_length_{0}; // 12bit

    int8_t *program_info_descriptor_{nullptr};

    int16_t ES_Info_length_{0};
    int8_t *ES_Info_descriptor_{nullptr};

    int32_t CRC_32_{0}; // 32bit
public:
    void demux(TsContext *ctx, TsPacket *pkt, BufferStream *stream);
    void print();
};

class TsAacAdts
{
    // reference aac-iso-13818-7.pdf page 26 Audio data transport stream ADTS
public:
    // adts_fixed_header
    int16_t syncword_{0};                 // 12 bits
    int8_t ID_{0};                        // 1 bits
    uint8_t layer_{0};                    // 2 bits
    int8_t protection_absent_{0};         // 1 bits
    uint8_t profile_{0};                  // 2 bits
    uint8_t sampling_frequency_index_{0}; // 4 bits
    int8_t private_bit_{0};               // 1bit
    uint8_t channel_configuretion_{0};    // 3bits
    int8_t original_copy_{0};             // 1bit
    int8_t home_{0};                      // 1bit

    // adts_variable_header
    int8_t copyright_identification_bit_{0};        // 1bit
    int8_t copyright_identification_start_{0};      // 1bit
    int16_t frame_length_{0};                       // 13bits
    int16_t adts_buffer_fullness_{0};               // 11bits
    uint8_t number_of_raw_data_blocks_in_frame_{0}; // 2bit

    int8_t *raw_data_{nullptr};
    int32_t raw_data_size_{0};
    void demux(BufferStream *stream);
    void print();
};

class TsH264Codec
{
public:
    ~TsH264Codec();

public:
    void demux(BufferStream *stream);
    bool match_start_code(BufferStream *stream);
    bool match_next_code(BufferStream *stream);
    void parse();
public:
    int8_t *raw_data_{nullptr};
    int32_t raw_data_size_{0};
};

class TsMuxer
{
public:
    ~TsMuxer();
    bool open(const std::string &filePath);
    bool write(char *p, int32_t size);

private:
    std::ofstream ofstream_;
};
void TsMessage::append(int8_t *p, int32_t size)
{
    if (size <= 0)
    {
        return;
    }
    if (packet_data_size_ - parsed_data_size_ < size)
    {
        packet_data_ = (int8_t *)realloc(packet_data_, parsed_data_size_ + size);
        packet_data_size_ = parsed_data_size_ + size;
    }
    memcpy(packet_data_ + parsed_data_size_, p, size);
    parsed_data_size_ += size;
}

bool TsMessage::is_video()
{
    return type_ == TsPidType::TsPidTypeVideo;
}

bool TsMessage::isEnd()
{
    // 对于PES_packet_length = 0 需要单独处理下
    return packet_data_size_ <= parsed_data_size_ && (PES_packet_length_ > 0);
}

TsContext::~TsContext()
{
    for (const auto pidInfo : pidInfos_)
    {
        delete pidInfo.second;
    }
}

TsPidInfo *TsContext::pidInfo(int16_t pid)
{
    if (pidInfos_.count(pid) > 0)
    {
        return pidInfos_.at(pid);
    }
    return nullptr;
}

void TsContext::pushPidInfo(TSPidTable pidTable, TsPidType type)
{

    if (pidInfos_.count(pidTable) > 0)
    {
        return;
    }
    auto pidInfo = new TsPidInfo;
    pidInfo->pid_ = pidTable;
    pidInfo->pid_type_ = type;
    pidInfos_.insert({pidTable, pidInfo});
}

TsMessage *TsContext::message(TSPidTable table)
{
    if (msgs_.count(table) <= 0 || (nullptr == msgs_.at(table)))
    {
        msgs_[table] = new TsMessage;
    }
    return msgs_.at(table);
}

TsMessage *TsContext::consumerMessage()
{

    if (!consumerMsgs_.empty())
    {
        auto msg = consumerMsgs_.front();
        consumerMsgs_.erase(consumerMsgs_.begin());
        return msg;
    }

    for (auto msg : msgs_)
    {
        if (msg.second->isEnd())
        {
            msgs_.erase(msg.first);
            return msg.second;
        }
    }
    return nullptr;
}

void TsContext::pushConsumerMessage(TsMessage *msg)
{
    consumerMsgs_.push_back(msg);
    msgs_[msg->pid_] = nullptr;
}

TsPacket::TsPacket()
{
    header_ = new TsPacketHeader;
    field_ = new TsAdaptationField;
    payload_ = new TsPayload;
}
TsPacket::~TsPacket()
{
    delete header_;
    delete field_;
    delete payload_;
}
void TsPacket::demux(TsContext *ctx, BufferStream *stream)
{
    header_->demux(stream);
    if (header_->adaptation_field_control_ == TsAdaptationType::TsAdaptationTypeAdaptationOnly || header_->adaptation_field_control_ == TsAdaptationType::TsAdaptationTypeBoth)
    {
        // 处理adaptation field
        field_->demux(stream);
    }
    if (header_->adaptation_field_control_ == TsAdaptationType::TsAdaptationTypePayloadOnly || header_->adaptation_field_control_ == TsAdaptationType::TsAdaptationTypeBoth)
    {
        // 处理具体的数据
        payload_->demux(ctx, this, stream);
    }
}

void TsPacketHeader::demux(BufferStream *buffer)
{

    sync_ = buffer->read1Bytes();
    pid_ = buffer->read2Bytes();
    transport_error_indicator_ = (pid_ >> 15) & 0x01;
    payload_unit_start_indicator_ = (pid_ >> 14) & 0x01;
    transport_priority_ = (pid_ >> 13) & 0x01;
    pid_ &= 0x1FFF;

    continuity_counter_ = buffer->read1Bytes();
    transport_scrambling_control_ = (continuity_counter_ >> 6) & 0x03;
    adaptation_field_control_ = (continuity_counter_ >> 4) & 0x03;
    continuity_counter_ &= 0x0F;
    print();
}
void TsPacketHeader::print()
{
    if (!kPrintTSPacketHeader)
    {
        return;
    }
    Debug() << "TsPacket:"
            << "sync:" << sync_ << "transport_error:" << static_cast<int>(transport_error_indicator_)
            << "payload_unit_start_:" << static_cast<int>(payload_unit_start_indicator_)
            << "ts_priority_:" << static_cast<int>(transport_priority_)
            << "pid:" << pid_
            << "scrambling:" << static_cast<int>(transport_scrambling_control_)
            << "ad_control_:" << static_cast<int>(adaptation_field_control_)
            << "continuity:" << static_cast<int>(continuity_counter_);
}

TsPayload::TsPayload()
{
    pes_ = new TsPayloadPES;
    pat_ = new TsPayloadPAT;
    pmt_ = new TsPayloadPMT;
}
TsPayload::~TsPayload()
{
    delete pes_;
    delete pat_;
    delete pmt_;
}
void TsPayload::read_point_field(TsPacket *pkt, BufferStream *stream)
{
    if (pkt->header_->payload_unit_start_indicator_)
    {
        pointer_field_ = stream->read1Bytes();
    }
}
void TsPayload::demux(TsContext *ctx, TsPacket *pkt, BufferStream *stream)
{
    if (pkt->header_->pid_ == TSPidTablePAT)
    {
        read_point_field(pkt, stream);
        pat_->demux(ctx, stream);
        return;
    }
    if (kPrintTsPacketPayloadPAT)
    {
        return;
    }
    TsPidInfo *info = ctx->pidInfo(pkt->header_->pid_);
    if (info && info->pid_type_ == TsPidType::TsPidTypePMT)
    {
        read_point_field(pkt, stream);
        pmt_->demux(ctx, pkt, stream);
        return;
    }
    if (KPrintTsPacketPayloadPMT)
    {
        return;
    }
    if (info && (info->pid_type_ == TsPidType::TsPidTypeAudio || info->pid_type_ == TsPidType::TsPidTypeVideo))
    {

        TsMessage *msg = ctx->message(info->pid_);

        if (!pkt->header_->payload_unit_start_indicator_)
        {
            if (msg->packet_start_code_prefix_ != 0x01)
            {
                Debug() << "PES: decode continous packet error, msg is empty";
                return;
            }
            int32_t size = stream->size() - stream->pos();
            auto tmp = new int8_t[size];
            stream->read((char *)tmp, size);
            msg->append(tmp, size);
            delete tmp;
            Debug() << "PES: msg size :" << size;
            return;
        }
        else
        {
            if (msg->packet_start_code_prefix_ == 0x01)
            {
                ctx->pushConsumerMessage(msg);
            }
        }
        pes_->demux(ctx, pkt, stream);
        return;
    }
}

int TsPayloadPES::demux(TsContext *ctx, TsPacket *pkt, BufferStream *stream)
{
    assert(stream != nullptr);
    {
        packet_start_code_prefix_ = stream->read4Bytes();
        stream_id_ = packet_start_code_prefix_ & 0x000000FF;
        packet_start_code_prefix_ = (packet_start_code_prefix_ >> 8) & 0xFFFFFF;
        if (packet_start_code_prefix_ != 0x01)
        {
            Debug() << "Payload decode error, packet_start_code_prefix is not 0x01";
            return -1;
        }
        PES_packet_length_ = stream->read2Bytes();
        int pos = stream->pos();
        if (stream_id_ != PES_program_stream_map &&
            stream_id_ != PES_padding_stream &&
            stream_id_ != PES_private_stream_2 &&
            stream_id_ != PES_ECM_stream &&
            stream_id_ != PES_EMM_stream &&
            stream_id_ != PES_program_stream_directory &&
            stream_id_ != PES_DSMCC_stream &&
            stream_id_ != PES_H_222_1_type_E)
        {
            {
                int8_t value = stream->read1Bytes();
                int8_t fixedValue = (value >> 6) & 0x03;
                if (fixedValue != 0x02)
                {
                    Debug() << "PayloadPES decode error, fixedValue is not 0x02" << fixedValue;
                    return -1;
                }
                PES_scrambling_control_ = (value >> 4) & 0x03;
                PES_priority_ = (value >> 3) & 0x01;
                data_alignment_indicator_ = (value >> 2) & 0x01;
                copyright_ = (value >> 1) & 0x01;
                original_or_copy_ = (value >> 0) & 0x01;
            }
            {
                int8_t value = stream->read1Bytes();
                PTS_DTS_flags_ = (value >> 6) & 0x03;
                ESCR_flag_ = (value >> 5) & 0x01;
                ES_rate_flag_ = (value >> 4) & 0x01;
                DSM_trick_mode_flag_ = (value >> 3) & 0x01;
                additional_copy_info_flag_ = (value >> 2) & 0x01;
                PES_CRC_flag_ = (value >> 1) & 0x01;
                PES_extension_flag_ = (value >> 0) & 0x01;
            }
            PES_header_data_length_ = stream->read1Bytes();
            int pos_header = stream->pos();

            if (PTS_DTS_flags_ & 0x02)
            {
                int8_t preValue = stream->read1Bytes();
                if (((preValue >> 4) & 0x03) != PTS_DTS_flags_)
                {
                    Debug() << "PayloadPES deocde error, fixedvalue not equal PTS_DTS_flags_";
                    return -1;
                }
                PTS_ = (preValue >> 1) & 0x07;
                PTS_ << 30;
                PTS_ |= (stream->read2Bytes() >> 1) & 0x7FFF;
                PTS_ <<= 15;
                PTS_ |= ((stream->read2Bytes() >> 1) & 0x7FFF);
            }
            if (PTS_DTS_flags_ & 0x01)
            {
                int8_t preValue = stream->read1Bytes();
                if (((preValue >> 4) & 0x0f) != 0x01)
                {
                    Debug() << "PayloadPES decode error, fixedvalue not equal 0x01";
                    return -1;
                }
                DTS_ = (preValue >> 1) & 0x07;
                DTS_ << 30;
                DTS_ |= (stream->read2Bytes() >> 1) & 0x7FFF;
                DTS_ <<= 15;
                DTS_ |= ((stream->read2Bytes() >> 1) & 0x7FFF);
            }
            if (ESCR_flag_)
            {
                int64_t value = stream->read4Bytes();
                value <<= 32;
                value |= stream->read2Bytes();
                value >>= 1;
                ESCR_extension_ = value & 0x1ff;
                value >>= 10;
                ESCR_base_ = value & 0x7fff;
                value >> 1;
                ESCR_base_ |= (value & 0x3fff8000);
                value >> 1;
                ESCR_base_ |= (value & 0x1c0000000);
            }
            if (ES_rate_flag_)
            {
                ES_rate_ = stream->read3Bytes();
                ES_rate_ >>= 1;
            }
            if (DSM_trick_mode_flag_)
            {
                trick_mode_value_ = stream->read1Bytes();
                trick_mode_control_ = trick_mode_value_ & 0xD0;
                trick_mode_control_ >>= 3;
                trick_mode_value_ &= 0x1f;
            }
            if (additional_copy_info_flag_)
            {
                additional_copy_info_ = stream->read1Bytes();
                additional_copy_info_ &= 0x7f;
            }
            if (PES_CRC_flag_)
            {
                previous_PES_packet_CRC_ = stream->read2Bytes();
            }
            if (PES_extension_flag_)
            {
                int flag = stream->read1Bytes();
                PES_private_data_flag_ = (flag >> 7) & 0x01;
                pack_header_field_flag_ = (flag >> 6) & 0x01;
                program_packet_sequence_counter_flag_ = (flag >> 5) & 0x01;
                P_STD_buffer_flag_ = (flag >> 4) & 0x01;
                PES_extension_flag_2_ = flag & 0x01;
                if (PES_private_data_flag_)
                {
                    PES_private_data_ = new int8_t[128 / 8];
                    stream->read((char *)PES_private_data_, 128 / 8);
                }
                if (pack_header_field_flag_)
                {
                    pack_field_length_ = stream->read1Bytes();
                    pack_field_ = new int8_t[pack_field_length_];
                    stream->read((char *)pack_field_, pack_field_length_);
                }
                if (program_packet_sequence_counter_flag_)
                {
                    program_packet_sequence_counter_ = stream->read1Bytes() & 0x7f;
                    original_stuff_length_ = stream->read1Bytes();
                    MPEG1_MPEG2_indentifier_ = (original_stuff_length_ >> 6) & 0x01;
                    original_stuff_length_ &= 0x3f;
                }
                if (P_STD_buffer_flag_)
                {
                    P_STD_buffer_size_ = stream->read2Bytes();
                    P_STD_buffer_scale_ = (P_STD_buffer_size_ >> 13) & 0x01;
                    P_STD_buffer_size_ &= 0x1FFF;
                }
                if (PES_extension_flag_2_)
                {
                    PES_extension_field_length_ = (stream->read1Bytes()) & 0x7f;
                    PES_extension_field_reserved_ = new int8_t[PES_extension_field_length_];
                    stream->read((char *)PES_extension_field_reserved_, PES_extension_field_length_);
                }
            }
            int stuffing_byte_length = PES_header_data_length_ - (stream->pos() - pos_header);
            stuffing_byte_ = new int8_t[stuffing_byte_length];
            stream->read((char *)stuffing_byte_, stuffing_byte_length);

            // 提取后面携带的数据
            TsPidInfo *pid = ctx->pidInfo(pkt->header_->pid_);
            if (pid == nullptr)
            {
                Debug() << "PES: pid" << pkt->header_->pid_ << "is invalid";
                return -1;
            }
            TsMessage *msg = ctx->message(pid->pid_);
            msg->pid_ = pid->pid_;
            msg->type_ = pid->pid_type_;
            msg->stream_id_ = stream_id_;
            msg->packet_start_code_prefix_ = packet_start_code_prefix_;
            msg->dts_ = DTS_;
            msg->pts_ = PTS_;
            msg->PES_packet_length_ = PES_packet_length_;

            int32_t header_size = stream->pos() - pos;
            if (PES_packet_length_ > 0)
            {
                msg->packet_data_size_ = PES_packet_length_ - header_size;
            }
            else
            {
                msg->packet_data_size_ = stream->size() - stream->pos();
            }
            int32_t cpSize = std::min(msg->packet_data_size_, stream->size() - stream->pos());
            msg->packet_data_ = new int8_t[msg->packet_data_size_];
            stream->read((char *)msg->packet_data_, cpSize);
            msg->parsed_data_size_ += cpSize;
            Debug() << "packet_data_size" << msg->packet_data_size_
                    << "PES_packet_length" << PES_packet_length_
                    << "packet_header_size" << header_size
                    << "cpSize" << cpSize;
        }
    }
    print();

    return 0;
}
void TsPayloadPES::print()
{
    if (!kPrintTsPacketPayloadPES)
    {
        return;
    }
    Debug() << "start_code" << packet_start_code_prefix_
            << "stream_id" << stream_id_
            << "pes_packet_length" << PES_packet_length_
            << "PTS" << PTS_
            << "DTS" << DTS_;
}
void TsPayloadPAT::demux(TsContext *ctx, BufferStream *stream)
{
    assert(stream);
    table_id_ = stream->read1Bytes();
    section_length_ = stream->read2Bytes();
    section_syntax_indicator_ = (section_length_ >> 15) & 0x01;
    section_length_ &= 0xfff;
    int pos = stream->pos();
    transport_stream_id_ = stream->read2Bytes();
    current_next_indicator_ = stream->read1Bytes();
    version_number_ = (current_next_indicator_ >> 1) & 0x1f;
    section_number_ = stream->read1Bytes();
    last_section_number_ = stream->read1Bytes();
    program_size_ = (section_length_ - 5 - 4) / 4; // 4 is CRC_32 bytes
    progreams_ = new int32_t[program_size_];
    for (int i = 0; i < program_size_; ++i)
    {
        int32_t value = stream->read4Bytes();
        progreams_[i] = value;
        int16_t program_number = (value >> 16) & 0xffff;
        int16_t program_map_PID = value & 0x1fff;
        if (kPrintTsPacketPayloadPAT)
        {
            Debug() << "PMT table's pid ->" << program_map_PID;
        }
        if (program_number > 0)
        {
            ctx->pushPidInfo((TSPidTable)(program_map_PID), TsPidType::TsPidTypePMT);
        }
    }
    print();
}

void TsPayloadPAT::print()
{
    if (!kPrintTsPacketPayloadPAT)
    {
        return;
    }
    Debug() << "PAT:"
            << "table_id:" << table_id_
            << "syntax:" << section_syntax_indicator_
            << "section_length:" << section_length_
            << "transport_stream_id:" << transport_stream_id_
            << "version_number:" << version_number_
            << "current_next_indicator:" << current_next_indicator_
            << "section_number:" << section_number_
            << "last_sectin_number" << last_section_number_
            << "program_size" << program_size_
            << "CRC_32" << CRC_32_;
}

void TsPayloadPMT::demux(TsContext *ctx, TsPacket *pkt, BufferStream *stream)
{
    table_id_ = stream->read1Bytes();
    section_length_ = stream->read2Bytes();
    section_syntax_indicator_ = (section_length_ >> 15) & 0x01;
    section_length_ &= 0xfff;
    int pos = stream->pos();
    program_number_ = stream->read2Bytes();

    current_next_indicator_ = stream->read1Bytes();
    version_number_ = (current_next_indicator_ >> 1) & 0x1f;
    current_next_indicator_ &= 0x01;

    section_number_ = stream->read1Bytes();
    last_section_number_ = stream->read1Bytes();

    PCR_PID_ = stream->read2Bytes();
    PCR_PID_ &= 0x1fff;
    program_info_length_ = stream->read2Bytes();
    program_info_length_ &= 0xfff;

    if (program_info_length_)
    {
        program_info_descriptor_ = new int8_t[program_info_length_];
        stream->read((char *)program_info_descriptor_, program_info_length_);
    }
    int endPos = pos + section_length_ - 4; // 计算N1总共占有多少个字节

    while (stream->pos() < endPos)
    {
        uint8_t stream_type = stream->read1Bytes();
        uint16_t elementary_PID = stream->read2Bytes();
        elementary_PID &= 0x1fff;
        uint16_t ES_Info_length = stream->read2Bytes();
        ES_Info_length &= 0xfff;
        stream->skip(ES_Info_length);
        std::string stream_type_str;
        if (stream_type == TSStreamType::TSStreamTypeAudioAAC)
        {
            stream_type_str = "AAC";
            ctx->pushPidInfo((TSPidTable)elementary_PID, TsPidType::TsPidTypeAudio);
        }
        else if (stream_type == TSStreamType::TSStreamTypeVideoH264)
        {
            stream_type_str = "H264";
            ctx->pushPidInfo((TSPidTable)elementary_PID, TsPidType::TsPidTypeVideo);
        }
        if (KPrintTsPacketPayloadPMT)
        {
            Debug() << "PMT elementary_PID->" << elementary_PID
                    << "stream type->" << stream_type_str;
        }
    }

    CRC_32_ = stream->read4Bytes();
}

void TsPayloadPMT::print()
{
    if (!KPrintTsPacketPayloadPMT)
    {
        return;
    }
    Debug() << "table_id:" << table_id_
            << "section_syntax_indicator_:" << section_syntax_indicator_
            << "section_length:" << section_length_
            << "program_number:" << program_number_
            << "version_number:" << version_number_
            << "current_next_indicator:" << current_next_indicator_
            << "secton_number:" << section_number_
            << "last_section_number:" << last_section_number_
            << "PCR_PID:" << PCR_PID_
            << "program_info_length" << program_info_length_;
}

void TsAdaptationField::print()
{
    if (!kPrintTsPacketAdapationField)
    {
        return;
    }
    Debug() << "ad field:"
            << "len:" << adaptation_field_length_ << " "
            << "discontinuity_indicator:" << discontinuity_indicator_ << " "
            << "random_access_indicator:" << random_access_indicator_ << " "
            << "elementary_stream_priority_indicator" << elementary_stream_priority_indicator << " "
            << "pcr_flag_" << pcr_flag_ << " "
            << "opcr_flag_" << opcr_flag_ << " "
            << "splicing_point_flag_" << splicing_point_flag_ << " "
            << "transport_private_data_flag_" << transport_private_data_flag_ << " "
            << "adaptation_field_extension_flag_" << adaptation_field_extension_flag_
            << "pcr_" << pcr_ << "original_pcr_" << original_pcr_;
}

void TsAdaptationField::demux(BufferStream *buffer)
{
    adaptation_field_length_ = buffer->read1Bytes();
    if (adaptation_field_length_ > 0)
    {
        int pos_header = buffer->pos();
        {
            int8_t value = buffer->read1Bytes();
            adaptation_field_extension_flag_ = value & 0x01;
            value >>= 1;
            transport_private_data_flag_ = value & 0x01;
            value >>= 1;
            splicing_point_flag_ = value & 0x01;
            value >>= 1;
            opcr_flag_ = value & 0x01;
            value >>= 1;
            pcr_flag_ = value & 0x01;
            value >>= 1;
            elementary_stream_priority_indicator = value & 0x01;
            value >>= 1;
            random_access_indicator_ = value & 0x01;
            value >>= 1;
            discontinuity_indicator_ = value & 0x01;
        }
        if (pcr_flag_)
        {
            program_clock_reference_base_ = buffer->read6Bytes();
            program_clock_reference_extension_ = program_clock_reference_base_ & 0x1ff;
            program_clock_reference_base_ = (program_clock_reference_base_ >> 15) & 0x1FFFFFFFFLL;
            pcr_ = program_clock_reference_extension_;
            pcr_ = (pcr_ << 33) & 0x3fe00000000LL;
            pcr_ |= program_clock_reference_base_;
        }
        if (opcr_flag_)
        {
            original_program_clock_reference_base_ = buffer->read6Bytes();
            original_program_clock_reference_extension_ = original_program_clock_reference_base_ & 0x1ff;
            original_program_clock_reference_base_ = (original_program_clock_reference_base_ >> 15) & 0x1FFFFFFFFLL;
            original_pcr_ = original_program_clock_reference_extension_;
            original_pcr_ = (original_pcr_ << 33) & 0x3fe00000000LL;
            original_pcr_ |= original_program_clock_reference_base_;
        }

        if (splicing_point_flag_)
        {
            splice_countdown_ = buffer->read1Bytes();
        }
        if (transport_private_data_flag_)
        {
            transport_private_data_length_ = buffer->read1Bytes();
            if (transport_private_data_length_ > 0)
            {
                private_data_byte_ = new int8_t[transport_private_data_length_];
                buffer->read((char *)private_data_byte_, transport_private_data_length_);
            }
        }
        if (adaptation_field_extension_flag_)
        {
            adaptation_field_extension_length_ = buffer->read1Bytes();
            int size = 0;
            int value = buffer->read1Bytes();
            size += 1;
            ltw_flag_ = (value >> 7) & 0x01;
            piecewise_rate_flag_ = (value >> 6) & 0x01;
            seamless_splice_flag_ = (value >> 5) & 0x01;
            if (ltw_flag_)
            {
                int value = buffer->read2Bytes();
                size += 2;
                ltw_valid_falg_ = (value >> 15) & 0x01;
                ltw_offset_ = (value)&0x7ffff;
            }
            if (piecewise_rate_flag_)
            {
                int value = buffer->read2Bytes() & 0x3fff;
                piecewise_rate_ = value << 8 | buffer->read1Bytes();
                size += 3;
            }

            if (seamless_splice_flag_)
            {
                int8_t value = buffer->read1Bytes();
                splice_type_ = (value >> 4) & 0x0f;
                dts_next_au0 = (value >> 1) & 0x07;
                marker_bit0 = value & 0x01;

                int16_t value2 = buffer->read2Bytes();
                dts_next_au1 = (value2 >> 1) & 0x7f;
                marker_bit1 = value2 & 0x01;

                value2 = buffer->read2Bytes();
                dts_next_au2 = (value2 >> 1) & 0x7f;
                marker_bit2 = value2 & 0x01;
                size += 5;
            }
            int reservedSize = adaptation_field_extension_length_ - size;
            if (reservedSize > 0)
            {
                reserved_ = new int8_t[reservedSize];
                buffer->read((char *)reserved_, reservedSize);
            }
        }

        int32_t stuffing_size = adaptation_field_length_ - (buffer->pos() - pos_header);
        buffer->skip(stuffing_size);
    }
    print();
}

void TsAacAdts::demux(BufferStream *stream)
{
    assert(stream);
    syncword_ = stream->read2Bytes();
    protection_absent_ = syncword_ & 0x01;
    layer_ = (syncword_ >> 1) & 0x03;
    ID_ = (syncword_ >> 3) & 0x01;
    syncword_ = (syncword_ >> 4) & 0xfff;
    if (syncword_ != 0xfff)
    {
        Debug() << "AACADTS: syncword is not 0xfff";
        return;
    }
    int64_t temp = stream->read5Bytes();
    number_of_raw_data_blocks_in_frame_ = temp & 0x03;
    temp >>= 2;
    adts_buffer_fullness_ = temp & 0x7ff;
    temp >>= 11;
    frame_length_ = temp & 0x1fff;
    temp >>= 13;
    copyright_identification_start_ = temp & 0x01;
    temp >>= 1;
    copyright_identification_bit_ = temp & 0x01;
    temp >>= 1;
    home_ = temp & 0x01;
    temp >>= 1;
    original_copy_ = temp & 0x01;
    temp >>= 1;
    channel_configuretion_ = temp & 0x07;
    temp >>= 3;
    private_bit_ = temp & 0x01;
    temp >>= 1;
    sampling_frequency_index_ = temp & 0xf;
    temp >>= 4;
    profile_ = temp & 0x03;

    print();
    if (number_of_raw_data_blocks_in_frame_ == 0)
    {
        // todo check
        if (protection_absent_ != '0')
        {
            raw_data_size_ = frame_length_ - (stream->pos());
            raw_data_ = new int8_t[raw_data_size_];
            stream->read((char *)raw_data_, raw_data_size_);
        }
        return;
    }
}
void TsAacAdts::print()
{
    if (!kPrintTsAacAdts)
    {
        return;
    }
    Debug() << "AACADTS: syncword" << syncword_
            << "ID" << ID_
            << "layer" << layer_
            << "protection_absent" << protection_absent_
            << "profile" << profile_
            << "frame_length" << frame_length_
            << "number_of_raw_data_block" << number_of_raw_data_blocks_in_frame_
            << "raw_data_size" << raw_data_size_
            << "sampling_frequency_index" << sampling_frequency_index_;
}

TsH264Codec::~TsH264Codec()
{
    delete raw_data_;
}

bool TsH264Codec::match_start_code(BufferStream *stream)
{
    int32_t start_code = stream->read3Bytes();
    if (start_code == 0x000001)
    {
        return true;
    }
    else
    {
        stream->skip(-3);
        return false;
    }
}

bool TsH264Codec::match_next_code(BufferStream *stream)
{
    int32_t start_code = stream->read3Bytes();
    stream->skip(-3);
    if (start_code == 0x000001 ||
        start_code == 0x000000)
    {
        return true;
    }
    return false;
}

void TsH264Codec::demux(BufferStream *stream)
{
    // match_start_code函数匹配3个字节的开始码 0x00 00 00 01
    // 如果第一次匹配失败，那么可能是4个字节的开始码 0x00 00 00 00 01
    // 所以这个时候就要去判断第一个字节是不是等于0x00，如果不是那么这个包就是有问题的

    while (!match_start_code(stream))
    {
        int firstValue = stream->read1Bytes();
        if (firstValue != 0x00)
        {
            Debug() << "not find start code: 0x00000001 or 0x000001";
            return;
        }
    }
    if (stream->empty())
    {
        Debug() << "not finde start code";
        return;
    }
    // 寻找下一个 start_code
    int pos = stream->pos();
    while (1)
    {

        if ((stream->size() - stream->pos()) < 3)
        {
            stream->skip(stream->size() - stream->pos());
            break;
        }
        if (match_next_code(stream))
        {
           break;
        }
        stream->skip(1);
    }
    raw_data_size_ = stream->pos() - pos;
    raw_data_ = new int8_t[raw_data_size_];
    stream->skip(-raw_data_size_);
    stream->read((char*)raw_data_,raw_data_size_);
    if((stream->size() - stream->pos()) == 3) {
        stream->skip(3);
    }
    Debug() << "H264Codec: raw_data_size:" << raw_data_size_;
    parse();
}


// 解析 NAL unit，refer  H.264-AVC-ISO_IEC_14496-10 page 44
void TsH264Codec::parse()
{
    if(raw_data_ == nullptr || raw_data_size_ <= 0) {
        return;
    }
    int8_t nal_unit_type = *raw_data_;
    int8_t nal_ref_idc = (nal_unit_type >> 5) & 0x03;
    nal_unit_type &= 0x01f;

    if(nal_ref_idc) {
        Debug() << "H264Codec got a sps/pps";
    }
    if(nal_unit_type == 5) {
        Debug() << "H264Codec got a IDR";
    } else if(nal_unit_type == 7) {
        Debug() << "H264Codec got a sps";
    } else if(nal_unit_type == 8) {
        Debug() << "H264Codec got a pps";
    } else if(nal_unit_type == 9) {
        Debug() << "H264Codec got a picture delimiter";
    }
}

TsMuxer::~TsMuxer()
{
    ofstream_.close();
}

bool TsMuxer::open(const std::string &filePath)
{
    if (ofstream_.is_open())
    {
        return true;
    }
    ofstream_.open(filePath, std::ios::out | std::ios::binary);
    return ofstream_.is_open();
}

bool TsMuxer::write(char *p, int32_t size)
{
    if (!ofstream_.is_open())
    {
        return false;
    }
    ofstream_.write(p, size);
    return true;
}

void consumer(TsContext *ctx, TsMuxer *aacMuxer, TsMuxer *h264Muxer)
{
    TsMessage *msg = nullptr;
    while ((msg = ctx->consumerMessage()))
    {
        Debug() << "CONSUMER:"
                << "isVideo:" << msg->is_video()
                << "packet_data_size" << msg->packet_data_size_
                << "parse_packet_data_size" << msg->parsed_data_size_;
        if (!msg->is_video())
        {
            TsAacAdts adts;
            BufferStream stream((char *)msg->packet_data_, msg->packet_data_size_);
            adts.demux(&stream);
            if (aacMuxer)
            {
                aacMuxer->write((char *)msg->packet_data_, msg->packet_data_size_);
            }
        }
        else
        {
            BufferStream stream((char*)msg->packet_data_, msg->packet_data_size_);
            while(!stream.empty()) {
                TsH264Codec codec;
                codec.demux(&stream);
            }
            if (h264Muxer)
            {
                h264Muxer->write((char *)msg->packet_data_, msg->packet_data_size_);
            }
        }
        delete msg;
    }
}
#if 0
const static std::string kTsFileName = R"(C:\code\demo\ts-parser\src\ts.ts)";
const static std::string kTsAACFileName = R"(C:\code\demo\ts-parser\src\ts.aac)";
const static std::string kTsH264FileName = R"(C:\code\demo\ts-parser\src\ts.h264)";

#else
const static std::string kTsFileName = R"(E:\code\opensource\ts-parser\src\ts.ts)";
const static std::string kTsAACFileName = R"(E:\code\opensource\ts-parser\src\ts.aac)";
const static std::string kTsH264FileName = R"(E:\code\opensource\ts-parser\src\ts.h264)";
#endif

const static int32_t kTsPacketSize = 188;

int main()
{
    TsFileBuffer buffer;
    Debug() << "open file:" << kTsFileName << buffer.open(kTsFileName);
    char *buf = new char[kTsPacketSize];
    BufferStream stream(buf, kTsPacketSize);
    int packetCount = 0;
    auto tsContext = new TsContext;

    TsMuxer aacMuxer;
    Debug() << "open file:" << kTsAACFileName << aacMuxer.open(kTsAACFileName);

    TsMuxer h264Muxer;
    Debug() << "open file:" << kTsH264FileName << h264Muxer.open(kTsH264FileName);

    while (!buffer.atEnd())
    {
        Debug() << "packet no:" << ++packetCount;
        stream.reset();
        buffer.read(buf, kTsPacketSize);

        TsPacket packet;
        packet.demux(tsContext, &stream);
        // seekg会将eof标志位清除，淦
        consumer(tsContext, &aacMuxer, &h264Muxer);
        Debug() << "---------------------------";
    }

    getchar();
    return 0;
}