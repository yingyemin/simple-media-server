#include "TsDemuxer.h"
#include "Log/Logger.h"
#include "Codec/AacTrack.h"
#include "Codec/Mp3Track.h"
#include "Codec/G711Track.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"
#include "Codec/H264Frame.h"
#include "Codec/H265Frame.h"
#include "Codec/AacFrame.h"
#include "Util/String.h"

#include <fstream>
#include <list>
// 用来处理字符串流的
// 尤其是在解析各种格式的时候
// 因为需要按照字节处理

#define tsPacketSize 188


TsDemuxer::TsDemuxer()
{
    logInfo << "TsDemuxer()";
}

// reference SRS research/hls code
const static bool kPrintTSPacketHeader = true;
const static bool kPrintTsPacketAdapationField = true;
const static bool kPrintTsPacketPayloadPES = true;
const static bool kPrintTsPacketPayloadPAT = false;
const static bool KPrintTsPacketPayloadPMT = true;
const static bool kPrintTsAacAdts = true;

class TsPacket;

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
    void demux(const StreamBuffer::Ptr& buffer);
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
    shared_ptr<int8_t> private_data_byte_{nullptr};

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
    shared_ptr<int8_t> reserved_{0};

    void demux(const StreamBuffer::Ptr& buffer);
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
    uint64_t PTS_{0}; // 33bit
    uint64_t DTS_{0}; // 33bit

    uint64_t ESCR_base_{0};       // 33bit
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
    shared_ptr<int8_t> PES_private_data_{nullptr}; // 128bit

    // if pack_header_field_flag set 1
    uint8_t pack_field_length_{0}; // 8bit
    // pack_header()
    shared_ptr<int8_t> pack_field_{nullptr};

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
    shared_ptr<int8_t> PES_extension_field_reserved_{nullptr};

    shared_ptr<int8_t> stuffing_byte_{nullptr};        // size: N1
    shared_ptr<int8_t> PES_packet_data_byte_{nullptr}; // size：N2
    int demux(TsDemuxer *ctx, TsPacket *pkt, const StreamBuffer::Ptr& buffer);
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
    shared_ptr<int32_t> progreams_{nullptr};

    int32_t CRC_32_{0}; // 32bit
    void demux(TsDemuxer *ctx, const StreamBuffer::Ptr& buffer);
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

    shared_ptr<int8_t> program_info_descriptor_{nullptr};

    int16_t ES_Info_length_{0};
    shared_ptr<int8_t> ES_Info_descriptor_{nullptr};

    int32_t CRC_32_{0}; // 32bit
public:
    void demux(TsDemuxer *ctx, TsPacket *pkt, const StreamBuffer::Ptr& buffer);
};

class TsPayload
{
public:
    shared_ptr<TsPayloadPES> pes_{nullptr};
    shared_ptr<TsPayloadPAT> pat_{nullptr};
    shared_ptr<TsPayloadPMT> pmt_{nullptr};
    uint8_t pointer_field_{0};

public:
    TsPayload();
    ~TsPayload();
    void read_point_field(TsPacket *pkt, const StreamBuffer::Ptr& buffer);
    void demux(TsDemuxer *ctx, TsPacket *pkt, const StreamBuffer::Ptr& buffer);
};

class TsPacket
{
public:
    TsPacket();
    ~TsPacket();
    shared_ptr<TsPacketHeader> header_{nullptr};
    shared_ptr<TsAdaptationField> field_{nullptr};
    shared_ptr<TsPayload> payload_{nullptr};

public:
    void demux(TsDemuxer *ctx, const StreamBuffer::Ptr& buffer);
};

void TsMessage::append(int8_t *p, int32_t size)
{
    if (!p || size <= 0)
    {
        return;
    }
    // if (packet_data_size_ - parsed_data_size_ < size)
    // {
    //     packet_data_ = (int8_t *)realloc(packet_data_, parsed_data_size_ + size);
    //     packet_data_size_ = parsed_data_size_ + size;
    // }
    // memcpy(packet_data_ + parsed_data_size_, p, size);
    packet_data_.append((char*)p, size);
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

TsDemuxer::~TsDemuxer()
{
    // for (const auto pidInfo : pidInfos_)
    // {
    //     delete pidInfo.second;
    // }
}

shared_ptr<TsPidInfo> TsDemuxer::pidInfo(int16_t pid)
{
    if (pidInfos_.count(pid) > 0)
    {
        return pidInfos_.at(pid);
    }
    return nullptr;
}

void TsDemuxer::pushPidInfo(TSPidTable pidTable, TsPidType type)
{

    if (pidInfos_.count(pidTable) > 0)
    {
        return;
    }
    auto pidInfo = make_shared<TsPidInfo>();
    pidInfo->pid_ = pidTable;
    pidInfo->pid_type_ = type;
    pidInfos_.insert({pidTable, pidInfo});
}

shared_ptr<TsMessage> TsDemuxer::message(TSPidTable table)
{
    if (msgs_.count(table) <= 0 || (nullptr == msgs_.at(table)))
    {
        msgs_[table] = make_shared<TsMessage>();
    }
    return msgs_.at(table);
}

void TsDemuxer::pushConsumerMessage(shared_ptr<TsMessage> msg)
{
    if (!msg) {
        return ;
    }

    int index = msg->is_video() ? VideoTrackType : AudioTrackType;
    onDecode((char*)msg->packet_data_.data(), msg->packet_data_.size(), index, msg->pts_, msg->dts_);

    msgs_[msg->pid_] = nullptr;
}

TsPacket::TsPacket()
{
    header_ = make_shared<TsPacketHeader>();
    field_ = make_shared<TsAdaptationField>();
    payload_ = make_shared<TsPayload>();
}

TsPacket::~TsPacket()
{
    // delete header_;
    // delete field_;
    // delete payload_;
}

void TsPacket::demux(TsDemuxer *ctx, const StreamBuffer::Ptr& buffer)
{
    if (!ctx || !buffer) {
        return ;
    }
    // logInfo << "demux ts header";
    header_->demux(buffer);
    auto info = ctx->pidInfo(header_->pid_);
    if (info) {
        if (info->seq_ != 16 && header_->continuity_counter_ != (info->seq_ + 1) % 16) {
            logWarn << "loss packet, pid: " << header_->pid_ << ", last seq: " << (int)info->seq_
                    << ", cur seq: " << (int)header_->continuity_counter_;
        }
        info->seq_ = header_->continuity_counter_;
    }

    if (header_->adaptation_field_control_ == TsAdaptationType::TsAdaptationTypeAdaptationOnly || header_->adaptation_field_control_ == TsAdaptationType::TsAdaptationTypeBoth)
    {
        // 处理adaptation field
        // logInfo << "处理adaptation field";
        field_->demux(buffer);
    }
    if (header_->adaptation_field_control_ == TsAdaptationType::TsAdaptationTypePayloadOnly || header_->adaptation_field_control_ == TsAdaptationType::TsAdaptationTypeBoth)
    {
        // 处理具体的数据
        // logInfo << "处理具体的数据";
        payload_->demux(ctx, this, buffer);
    }
}

void TsPacketHeader::demux(const StreamBuffer::Ptr& buffer)
{
    if (buffer->size() < 4) {
        logWarn << "ts packet size < 4";
        return ;
    }
    uint8_t* payload = (uint8_t*)buffer->data();

    sync_ = payload[0];
    pid_ = payload[1] << 8 | payload[2];
    logDebug << "all byte pid: " << (int)pid_;
    logDebug << "payload[1]: " << (int)payload[1];
    logDebug << "payload[2]: " << (int)payload[2];
    transport_error_indicator_ = (pid_ >> 15) & 0x01;
    payload_unit_start_indicator_ = (pid_ >> 14) & 0x01;
    transport_priority_ = (pid_ >> 13) & 0x01;
    pid_ &= 0x1FFF;

    logDebug << "pid: " << (int)pid_;
    logDebug << "payload_unit_start_indicator_: " << (int)payload_unit_start_indicator_;

    continuity_counter_ = payload[3];
    transport_scrambling_control_ = (continuity_counter_ >> 6) & 0x03;
    adaptation_field_control_ = (continuity_counter_ >> 4) & 0x03;
    continuity_counter_ &= 0x0F;

    logDebug << "continuity_counter_: " << (int)continuity_counter_;

    buffer->substr(4);
}

TsPayload::TsPayload()
{
    pes_ = make_shared<TsPayloadPES>();
    pat_ = make_shared<TsPayloadPAT>();
    pmt_ = make_shared<TsPayloadPMT>();
}

TsPayload::~TsPayload()
{
    // delete pes_;
    // delete pat_;
    // delete pmt_;
}

void TsPayload::read_point_field(TsPacket *pkt, const StreamBuffer::Ptr& buffer)
{
    if (buffer->size() < 1 || !pkt) {
        return ;
    }

    if (pkt->header_->payload_unit_start_indicator_)
    {
        pointer_field_ = buffer->data()[0];
        buffer->substr(1);
    }
}

void TsPayload::demux(TsDemuxer *ctx, TsPacket *pkt, const StreamBuffer::Ptr& buffer)
{
    if (!ctx || !pkt || !buffer) {
        return ;
    }

    logDebug << "ts pid: " << (int) (pkt->header_->pid_);
    if (pkt->header_->pid_ == TSPidTablePAT)
    {
        logTrace << "demux pat";
        read_point_field(pkt, buffer);
        pat_->demux(ctx, buffer);
        return;
    }
    if (kPrintTsPacketPayloadPAT)
    {
        return;
    }
    auto info = ctx->pidInfo(pkt->header_->pid_);
    if (info && info->pid_type_ == TsPidType::TsPidTypePMT)
    {
        logTrace << "demux pmt";
        read_point_field(pkt, buffer);
        pmt_->demux(ctx, pkt, buffer);
        return;
    }
    // if (KPrintTsPacketPayloadPMT)
    // {
    //     return;
    // }
    if (info && (info->pid_type_ == TsPidType::TsPidTypeAudio || info->pid_type_ == TsPidType::TsPidTypeVideo))
    {
        logTrace << "demux media data";
        auto msg = ctx->message(info->pid_);
        if (!msg) {
            return ;
        }

        if (!pkt->header_->payload_unit_start_indicator_)
        {
            if (msg->packet_start_code_prefix_ != 0x01)
            {
                logInfo << "PES: decode continous packet error, msg is empty";
                return;
            }
            int32_t size = buffer->size();
            msg->append((int8_t*)buffer->data(), size);
            buffer->substr(size);
            logDebug << "PES: msg size :" << size;
            return;
        }
        else
        {
            logDebug << "msg->packet_start_code_prefix_" << (int)msg->packet_start_code_prefix_;
            if (msg->packet_start_code_prefix_ == 0x01)
            {
                ctx->pushConsumerMessage(msg);
            }
        }
        if (pes_->demux(ctx, pkt, buffer) != 0) {
            logWarn << "demux pes failed";
        }

        return;
    }
}

int TsPayloadPES::demux(TsDemuxer *ctx, TsPacket *pkt, const StreamBuffer::Ptr& buffer)
{
    if (!ctx || !pkt || !buffer) {
        return -1;
    }

    if (buffer->size() < 6) {
        return -1;
    }
    {
        auto payload = buffer->data();

        packet_start_code_prefix_ = readUint32BE(payload);
        stream_id_ = packet_start_code_prefix_ & 0x000000FF;
        packet_start_code_prefix_ = (packet_start_code_prefix_ >> 8) & 0xFFFFFF;
        if (packet_start_code_prefix_ != 0x01)
        {
            logInfo << "Payload decode error, packet_start_code_prefix is not 0x01";
            return -1;
        }
        PES_packet_length_ = readUint16BE(payload + 4);
        int pos = 6;
        int index = 6;
        if (stream_id_ != PES_program_stream_map &&
            stream_id_ != PES_padding_stream &&
            stream_id_ != PES_private_stream_2 &&
            stream_id_ != PES_ECM_stream &&
            stream_id_ != PES_EMM_stream &&
            stream_id_ != PES_program_stream_directory &&
            stream_id_ != PES_DSMCC_stream &&
            stream_id_ != PES_H_222_1_type_E)
        {
            if (buffer->size() < pos + 3) {
                return -1;
            }
            {
                int8_t value = payload[pos++];
                int8_t fixedValue = (value >> 6) & 0x03;
                if (fixedValue != 0x02)
                {
                    logInfo << "PayloadPES decode error, fixedValue is not 0x02" << fixedValue;
                    return -1;
                }
                PES_scrambling_control_ = (value >> 4) & 0x03;
                PES_priority_ = (value >> 3) & 0x01;
                data_alignment_indicator_ = (value >> 2) & 0x01;
                copyright_ = (value >> 1) & 0x01;
                original_or_copy_ = (value >> 0) & 0x01;
            }
            {
                int8_t value = payload[pos++];
                PTS_DTS_flags_ = (value >> 6) & 0x03;
                ESCR_flag_ = (value >> 5) & 0x01;
                ES_rate_flag_ = (value >> 4) & 0x01;
                DSM_trick_mode_flag_ = (value >> 3) & 0x01;
                additional_copy_info_flag_ = (value >> 2) & 0x01;
                PES_CRC_flag_ = (value >> 1) & 0x01;
                PES_extension_flag_ = (value >> 0) & 0x01;
            }
            PES_header_data_length_ = payload[pos++];
            int pos_header = pos;

            if (PTS_DTS_flags_ & 0x02)
            {
                if (buffer->size() < pos + 4) {
                    return -1;
                }
                int8_t preValue = payload[pos++];
                if (((preValue >> 4) & 0x03) != PTS_DTS_flags_)
                {
                    logInfo << "PayloadPES deocde error, fixedvalue not equal PTS_DTS_flags_";
                    return -1;
                }
                PTS_ = (preValue >> 1) & 0x07;
                PTS_ <<= 15;
                PTS_ |= (readUint16BE(payload + pos) >> 1) & 0x7FFF;
                PTS_ <<= 15;
                PTS_ |= ((readUint16BE(payload + pos + 2) >> 1) & 0x7FFF);
                pos += 4;
            }
            if (PTS_DTS_flags_ & 0x01)
            {
                if (buffer->size() < pos + 4) {
                    return -1;
                }
                int8_t preValue = payload[pos++];
                if (((preValue >> 4) & 0x0f) != 0x01)
                {
                    logInfo << "PayloadPES decode error, fixedvalue not equal 0x01";
                    return -1;
                }
                DTS_ = (preValue >> 1) & 0x07;
                DTS_ <<= 15;
                DTS_ |= (readUint16BE(payload + pos) >> 1) & 0x7FFF;
                DTS_ <<= 15;
                DTS_ |= ((readUint16BE(payload + pos + 2) >> 1) & 0x7FFF);
                pos += 4;
            }
            if (ESCR_flag_)
            {
                if (buffer->size() < pos + 6) {
                    return -1;
                }
                int64_t value = readUint32BE(payload + pos);
                value <<= 32;
                value |= readUint16BE(payload + pos + 4);
                value >>= 1;
                ESCR_extension_ = value & 0x1ff;
                value >>= 10;
                ESCR_base_ = value & 0x7fff;
                value >> 1;
                ESCR_base_ |= (value & 0x3fff8000);
                value >> 1;
                ESCR_base_ |= (value & 0x1c0000000);
                pos += 6;
            }
            if (ES_rate_flag_)
            {
                if (buffer->size() < pos + 3) {
                    return -1;
                }
                ES_rate_ = readUint24BE(payload + pos);
                ES_rate_ >>= 1;
                pos += 3;
            }
            if (DSM_trick_mode_flag_)
            {
                if (buffer->size() < pos + 1) {
                    return -1;
                }
                trick_mode_value_ = payload[pos++];
                trick_mode_control_ = trick_mode_value_ & 0xD0;
                trick_mode_control_ >>= 3;
                trick_mode_value_ &= 0x1f;
            }
            if (additional_copy_info_flag_)
            {
                if (buffer->size() < pos + 1) {
                    return -1;
                }
                additional_copy_info_ = payload[pos++];
                additional_copy_info_ &= 0x7f;
            }
            if (PES_CRC_flag_)
            {
                if (buffer->size() < pos + 2) {
                    return -1;
                }
                previous_PES_packet_CRC_ = readUint16BE(payload + pos);
                pos += 2;
            }
            if (PES_extension_flag_)
            {
                if (buffer->size() < pos + 1) {
                    return -1;
                }
                int flag = payload[pos++];
                PES_private_data_flag_ = (flag >> 7) & 0x01;
                pack_header_field_flag_ = (flag >> 6) & 0x01;
                program_packet_sequence_counter_flag_ = (flag >> 5) & 0x01;
                P_STD_buffer_flag_ = (flag >> 4) & 0x01;
                PES_extension_flag_2_ = flag & 0x01;
                if (PES_private_data_flag_)
                {
                    int sizePri = 128 / 8;
                    if (buffer->size() < pos + sizePri) {
                        return -1;
                    }
                    PES_private_data_.reset(new int8_t[sizePri], std::default_delete<int8_t[]>());
                    memcpy(PES_private_data_.get(), payload + pos, sizePri);
                    pos += sizePri;
                }
                if (pack_header_field_flag_)
                {
                    if (buffer->size() < pos + 1) {
                        return -1;
                    }
                    pack_field_length_ = payload[pos++];

                    if (buffer->size() < pos + pack_field_length_) {
                        return -1;
                    }
                    pack_field_.reset(new int8_t[pack_field_length_], std::default_delete<int8_t[]>());
                    memcpy(pack_field_.get(), payload + pos, pack_field_length_);
                    pos += pack_field_length_;
                }
                if (program_packet_sequence_counter_flag_)
                {
                    if (buffer->size() < pos + 2) {
                        return -1;
                    }
                    program_packet_sequence_counter_ = payload[pos++] & 0x7f;
                    original_stuff_length_ = payload[pos++];;
                    MPEG1_MPEG2_indentifier_ = (original_stuff_length_ >> 6) & 0x01;
                    original_stuff_length_ &= 0x3f;
                }
                if (P_STD_buffer_flag_)
                {
                    if (buffer->size() < pos + 2) {
                        return -1;
                    }
                    P_STD_buffer_size_ = readUint16BE(payload + pos);
                    P_STD_buffer_scale_ = (P_STD_buffer_size_ >> 13) & 0x01;
                    P_STD_buffer_size_ &= 0x1FFF;
                    pos += 2;
                }
                if (PES_extension_flag_2_)
                {
                    if (buffer->size() < pos + 1) {
                        return -1;
                    }
                    PES_extension_field_length_ = (payload[pos++]) & 0x7f;
                    
                    if (buffer->size() < pos + PES_extension_field_length_) {
                        return -1;
                    }
                    PES_extension_field_reserved_.reset(new int8_t[PES_extension_field_length_], std::default_delete<int8_t[]>());
                    memcpy(PES_extension_field_reserved_.get(), payload + pos, PES_extension_field_length_);
                    pos += PES_extension_field_length_;
                }
            }
            int stuffing_byte_length = PES_header_data_length_ - (pos - pos_header);
            if (buffer->size() < pos + stuffing_byte_length) {
                return -1;
            }
            stuffing_byte_.reset(new int8_t[stuffing_byte_length], std::default_delete<int8_t[]>());
            memcpy(stuffing_byte_.get(), payload + pos, stuffing_byte_length);
            pos += stuffing_byte_length;

            // 提取后面携带的数据
            auto pid = ctx->pidInfo(pkt->header_->pid_);
            if (pid == nullptr)
            {
                logInfo << "PES: pid" << pkt->header_->pid_ << "is invalid";
                return -1;
            }
            auto msg = ctx->message(pid->pid_);
            msg->pid_ = pid->pid_;
            msg->type_ = pid->pid_type_;
            msg->stream_id_ = stream_id_;
            msg->packet_start_code_prefix_ = packet_start_code_prefix_;
            msg->dts_ = DTS_;
            msg->pts_ = PTS_;
            msg->PES_packet_length_ = PES_packet_length_;

            int32_t header_size = pos - index;
            if (PES_packet_length_ > 0)
            {
                msg->packet_data_size_ = PES_packet_length_ - header_size;
            }
            else
            {
                // msg->packet_data_size_ = pos - index;
                msg->packet_data_size_ = buffer->size() - pos;
            }
            int32_t cpSize = msg->packet_data_size_ < (buffer->size() - pos) ? msg->packet_data_size_:(buffer->size() - pos); 
            
            if (buffer->size() < pos + cpSize) {
                return -1;
            }
            // msg->packet_data_ = new int8_t[msg->packet_data_size_];
            // memcpy(msg->packet_data_, payload + pos, cpSize);
            msg->packet_data_.assign(payload + pos, cpSize);
            pos += cpSize;
            msg->parsed_data_size_ += cpSize;
            logDebug << "packet_data_size: " << msg->packet_data_size_
                    << ", PES_packet_length: " << PES_packet_length_
                    << ", packet_header_size: " << header_size
                    << ", cpSize: " << cpSize;
        }
    }

    return 0;
}

void TsPayloadPAT::demux(TsDemuxer *ctx, const StreamBuffer::Ptr& buffer)
{
    if (!ctx || !buffer) {
        return ;
    }
    auto payload = buffer->data();
    if (buffer->size() < 8) {
        return ;
    }

    table_id_ = payload[0];
    section_length_ = readUint16BE(payload + 1);
    section_syntax_indicator_ = (section_length_ >> 15) & 0x01;
    section_length_ &= 0xfff;
    transport_stream_id_ = readUint16BE(payload + 3);
    current_next_indicator_ = payload[5];
    version_number_ = (current_next_indicator_ >> 1) & 0x1f;
    section_number_ = payload[6];
    last_section_number_ = payload[7];
    program_size_ = (section_length_ - 5 - 4) / 4; // 4 is CRC_32 bytes, 5 is transport_stream_id_~last_section_number_
    progreams_.reset(new int32_t[program_size_], std::default_delete<int32_t[]>());
    int index = 8;
    for (int i = 0; i < program_size_; ++i)
    {
        if (buffer->size() < index + 4) {
            return ;
        }
        int32_t value = readUint32BE(payload + index);
        index += 4;

        progreams_.get()[i] = value;
        int16_t program_number = (value >> 16) & 0xffff;
        int16_t program_map_PID = value & 0x1fff;
        // if (kPrintTsPacketPayloadPAT)
        // {
            logDebug << "PMT table's pid ->" << program_map_PID;
        // }
        if (program_number > 0)
        {
            ctx->pushPidInfo((TSPidTable)(program_map_PID), TsPidType::TsPidTypePMT);
        }
    }

    CRC_32_ = readUint32BE(payload + index);

    buffer->substr(index + 4);
}

void TsPayloadPMT::demux(TsDemuxer *ctx, TsPacket *pkt, const StreamBuffer::Ptr& buffer)
{
    if (!ctx || !buffer || !pkt) {
        return ;
    }
    auto payload = buffer->data();

    if (buffer->size() < 12) {
        return ;
    }

    table_id_ = payload[0];
    section_length_ = readUint16BE(payload + 1);
    section_syntax_indicator_ = (section_length_ >> 15) & 0x01;
    section_length_ &= 0xfff;
    int pos = 3;
    program_number_ = readUint16BE(payload + 3);

    current_next_indicator_ = payload[5];
    version_number_ = (current_next_indicator_ >> 1) & 0x1f;
    current_next_indicator_ &= 0x01;

    section_number_ = payload[6];
    last_section_number_ = payload[7];

    PCR_PID_ = readUint16BE(payload + 8);
    PCR_PID_ &= 0x1fff;
    program_info_length_ = readUint16BE(payload + 10);
    program_info_length_ &= 0xfff;

    
    if (buffer->size() < 12 + program_info_length_) {
        return ;
    }

    if (program_info_length_)
    {
        program_info_descriptor_.reset(new int8_t[program_info_length_], std::default_delete<int8_t[]>());
        memcpy(program_info_descriptor_.get(), payload + 12, program_info_length_);
    }
    int endPos = pos + section_length_ - 4; // 计算N1总共占有多少个字节

    pos = 12 + program_info_length_;

    while (pos < endPos)
    {
        if (buffer->size() < 5 + pos) {
            return ;
        }
        uint8_t stream_type = payload[pos];
        uint16_t elementary_PID = readUint16BE(payload + pos + 1);
        elementary_PID &= 0x1fff;
        uint16_t ES_Info_length = readUint16BE(payload + pos + 3);
        ES_Info_length &= 0xfff;
        
        if (buffer->size() < 5 + pos + ES_Info_length) {
            return ;
        }

        program_info_descriptor_.reset(new int8_t[ES_Info_length_], std::default_delete<int8_t[]>());
        memcpy(ES_Info_descriptor_.get(), payload + pos + 5, ES_Info_length_);

        pos += 5 + ES_Info_length;

        std::string stream_type_str;
        int pidType = TsPidTypeReserved;

        switch (stream_type) {
            case STREAM_TYPE_AUDIO_G711:
                stream_type_str = "g711a";
                pidType = TsPidTypeAudio;
                break;
            case STREAM_TYPE_AUDIO_MP3:
                stream_type_str = "mp3";
                pidType = TsPidTypeAudio;
                break;
            case STREAM_TYPE_AUDIO_G711ULAW:
                stream_type_str = "g711u";
                pidType = TsPidTypeAudio;
                break;
            case STREAM_TYPE_AUDIO_AAC: {
                stream_type_str = "aac";
                pidType = TsPidTypeAudio;
                break;
            }
            case STREAM_TYPE_AUDIO_OPUS: {
                stream_type_str = "opus";
                pidType = TsPidTypeAudio;
                break;
            }
            case STREAM_TYPE_VIDEO_HEVC:
                stream_type_str = "h265";
                pidType = TsPidTypeVideo;
                break;
            case STREAM_TYPE_VIDEO_H264: {
                stream_type_str = "h264";
                pidType = TsPidTypeVideo;
                break;
            }
            case STREAM_TYPE_VIDEO_VP8: {
                stream_type_str = "vp8";
                pidType = TsPidTypeVideo;
                break;
            }
            case STREAM_TYPE_VIDEO_VP9: {
                stream_type_str = "vp9";
                pidType = TsPidTypeVideo;
                break;
            }
            case STREAM_TYPE_VIDEO_AV1: {
                stream_type_str = "av1";
                pidType = TsPidTypeVideo;
                break;
            }
        }
        
        if (!stream_type_str.empty()) {
            ctx->pushPidInfo((TSPidTable)elementary_PID, (TsPidType)pidType);
            ctx->createTrackInfo(stream_type_str, pidType);
        }

        // if (KPrintTsPacketPayloadPMT)
        // {
            logDebug << "PMT elementary_PID->" << elementary_PID
                    << "stream type->" << stream_type_str;
        // }
    }

    CRC_32_ = readUint32BE(payload + pos);

    buffer->substr(pos + 4);
}

void TsAdaptationField::demux(const StreamBuffer::Ptr& buffer)
{
    if (!buffer) {
        return ;
    }
    auto payload = buffer->data();

    if (buffer->size() < 1) {
        return ;
    }

    adaptation_field_length_ = payload[0];
    int pos = 1;
    if (adaptation_field_length_ > 0)
    {
        int pos_header = pos;
        {
            if (buffer->size() < pos + 1) {
                return ;
            }

            int8_t value = payload[pos++];
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
            if (buffer->size() < pos + 6) {
                return ;
            }
            program_clock_reference_base_ = readUint48BE(payload + pos);
            pos += 6;
            program_clock_reference_extension_ = program_clock_reference_base_ & 0x1ff;
            program_clock_reference_base_ = (program_clock_reference_base_ >> 15) & 0x1FFFFFFFFLL;
            pcr_ = program_clock_reference_extension_;
            pcr_ = (pcr_ << 33) & 0x3fe00000000LL;
            pcr_ |= program_clock_reference_base_;
        }
        if (opcr_flag_)
        {
            if (buffer->size() < pos + 6) {
                return ;
            }
            original_program_clock_reference_base_ = readUint48BE(payload + pos);
            pos += 6;
            original_program_clock_reference_extension_ = original_program_clock_reference_base_ & 0x1ff;
            original_program_clock_reference_base_ = (original_program_clock_reference_base_ >> 15) & 0x1FFFFFFFFLL;
            original_pcr_ = original_program_clock_reference_extension_;
            original_pcr_ = (original_pcr_ << 33) & 0x3fe00000000LL;
            original_pcr_ |= original_program_clock_reference_base_;
        }

        if (splicing_point_flag_)
        {
            if (buffer->size() < pos + 1) {
                return ;
            }
            splice_countdown_ = payload[pos++];
        }
        if (transport_private_data_flag_)
        {
            if (buffer->size() < pos + 1) {
                return ;
            }
            transport_private_data_length_ = payload[pos++];
            if (transport_private_data_length_ > 0)
            {
                if (buffer->size() < pos + transport_private_data_length_) {
                    return ;
                }
                private_data_byte_.reset(new int8_t[transport_private_data_length_], std::default_delete<int8_t[]>());
                memcpy(private_data_byte_.get(), payload + pos, transport_private_data_length_);
                pos += transport_private_data_length_;
            }
        }
        if (adaptation_field_extension_flag_)
        {
            if (buffer->size() < pos + 1) {
                return ;
            }
            adaptation_field_extension_length_ = payload[pos++];
            int size = 0;
            int value = payload[pos++];
            size += 1;
            ltw_flag_ = (value >> 7) & 0x01;
            piecewise_rate_flag_ = (value >> 6) & 0x01;
            seamless_splice_flag_ = (value >> 5) & 0x01;
            if (ltw_flag_)
            {
                if (buffer->size() < pos + 2) {
                    return ;
                }
                int value = readUint16BE(payload + pos);
                pos += 2;
                size += 2;
                ltw_valid_falg_ = (value >> 15) & 0x01;
                ltw_offset_ = (value)&0x7ffff;
            }
            if (piecewise_rate_flag_)
            {
                if (buffer->size() < pos + 2) {
                    return ;
                }
                int value = readUint16BE(payload + pos) & 0x3fff;
                pos += 2;
                piecewise_rate_ = value << 8 | payload[pos++];
                size += 3;
            }

            if (seamless_splice_flag_)
            {
                if (buffer->size() < pos + 5) {
                    return ;
                }
                int8_t value = payload[pos++];
                splice_type_ = (value >> 4) & 0x0f;
                dts_next_au0 = (value >> 1) & 0x07;
                marker_bit0 = value & 0x01;

                int16_t value2 = readUint16BE(payload + pos);
                dts_next_au1 = (value2 >> 1) & 0x7f;
                marker_bit1 = value2 & 0x01;

                value2 = readUint16BE(payload + pos + 2);
                dts_next_au2 = (value2 >> 1) & 0x7f;
                marker_bit2 = value2 & 0x01;
                size += 5;
                pos += 4;
            }
            int reservedSize = adaptation_field_extension_length_ - size;
            if (reservedSize > 0)
            {
                // reserved_ = new int8_t[reservedSize];
                // buffer->read((char *)reserved_, reservedSize);
                pos += reservedSize;
            }
        }

        int32_t stuffing_size = adaptation_field_length_ - (pos - pos_header);
        pos += stuffing_size;
    }
    buffer->substr(pos);
}

void TsDemuxer::onDecode(const char* data, int len, int index, uint64_t pts, uint64_t dts)
{
    if (!data || len == 0) {
        return ;
    }

    // if (index == VideoTrackType) {
    //     FILE* fp = fopen("testts.h264", "ab+");
    //     fwrite(data, len, 1, fp);
    //     fclose(fp);
    // }
    // if (_firstAac && _audioCodec == "aac" && index == AudioTrackType)
    // {
    //     if (len <= 7) {
    //         return;
    //     }
    //     if (_mapTrackInfo.find(AudioTrackType) != _mapTrackInfo.end()) {
    //         auto aacTrack = dynamic_pointer_cast<AacTrack>(_mapTrackInfo[AudioTrackType]);
    //         aacTrack->setAacInfoByAdts(data, 7);
    //         _firstAac = false;
    //         if (_onReady) {
    //             _onReady();
    //         }
    //     }
    // } else if (!_audioCodec.empty() && index == AudioTrackType) {
    //     if (!_hasReady) {
    //         _hasReady = true;
    //         _onReady();
    //     }
    // }

    FrameBuffer::Ptr frame;

    if (index == AudioTrackType) {
        frame = FrameBuffer::createFrame(_audioCodec, 0, AudioTrackType, false);
        if (_audioCodec == "aac") {
            // frame = make_shared<AacFrame>();
            frame->_startSize = 7;
        } else {
            // frame = make_shared<FrameBuffer>();
        }
    } else if (index == VideoTrackType/* && (_videoCodec == "h264" || _videoCodec == "h265")*/) {
        if (len <= 4) {
            return ;
        }
        frame = FrameBuffer::createFrame(_videoCodec, 0, VideoTrackType, false);
        // if (_videoCodec == "h264") {
        //     frame = make_shared<H264Frame>();
        // } else {
        //     frame = make_shared<H265Frame>();
        // }
    }

    if (frame) {
        dts = dts == 0 ? pts : dts;
        logDebug << "pts: " << pts;
        logDebug << "dts: " << dts;
        frame->_buffer.assign(data, len);
        frame->_pts = pts / 90; // pts * 1000 / 90000,计算为毫秒
        frame->_dts = dts / 90;
        frame->_index = index;
        frame->_trackType = index;
        frame->_codec = index == VideoTrackType ? _videoCodec : _audioCodec;

        if (index == VideoTrackType) {
            if (_videoCodec == "h265") {
                frame->_startSize = 4;
                if (readUint32BE(frame->data()) != 1) {
                    frame->_startSize = 3;
                }
                auto h265frame = dynamic_pointer_cast<H265Frame>(frame);
                h265frame->split([this, h265frame](const FrameBuffer::Ptr &subFrame){
                    // if (_firstVps || _firstSps || _firstPps) {
                    //     auto h265Track = dynamic_pointer_cast<H265Track>(_mapTrackInfo[VideoTrackType]);
                    //     auto nalType = subFrame->getNalType();
                    //     switch (nalType)
                    //     {
                    //     case H265_VPS:
                    //         h265Track->setVps(subFrame);
                    //         _firstVps = false;
                    //         break;
                    //     case H265_SPS:
                    //         h265Track->setSps(subFrame);
                    //         _firstSps = false;
                    //         break;
                    //     case H265_PPS:
                    //         h265Track->setPps(subFrame);
                    //         _firstPps = false;
                    //         break;
                    //     default:
                    //         break;
                    //     }
                    // }else {
                    //     logInfo << "gb28181 _onReady";
                    //     if (!_hasReady && _onReady) {
                    //         // if (!_firstAac) {
                    //             _onReady();
                    //         // }
                    //         _hasReady = true;
                    //     }
                    // }
                    if (_onFrame) {
                        _onFrame(subFrame);
                    }
                });
                return ;
            } else if (_videoCodec == "h264") {
                frame->_startSize = 4;
                if (readUint32BE(frame->data()) != 1) {
                    frame->_startSize = 3;
                }
                auto h264frame = dynamic_pointer_cast<H264Frame>(frame);
                h264frame->split([this, h264frame](const FrameBuffer::Ptr &subFrame){
                    // if (_firstSps || _firstPps) {
                    //     auto h264Track = dynamic_pointer_cast<H264Track>(_mapTrackInfo[VideoTrackType]);
                    //     auto nalType = subFrame->getNalType();
                    //     logInfo << "get a h264 nal type: " << (int)nalType;
                    //     switch (nalType)
                    //     {
                    //     case H264_SPS:
                    //         h264Track->setSps(subFrame);
                    //         _firstSps = false;
                    //         break;
                    //     case H264_PPS:
                    //         h264Track->setPps(subFrame);
                    //         _firstPps = false;
                    //         break;
                    //     default:
                    //         break;
                    //     }
                    // } else {
                    //     // logInfo << "gb28181 _onReady";
                    //     if (!_hasReady && _onReady) {
                    //         // if (!_firstAac) {
                    //             _onReady();
                    //         // }
                    //         _hasReady = true;
                    //     }
                    // }
                    if (_onFrame) {
                        _onFrame(subFrame);
                    }
                });
                return ;
            }
        } else if (frame->getTrackType() == STREAM_TYPE_AUDIO_AAC) {
            // auto aacframe = dynamic_pointer_cast<AacFrame>(frame);
            // aacframe->split([this](const FrameBuffer::Ptr &subFrame){
            //     if (_onFrame) {
            //         _onFrame(subFrame);
            //     }
            // });
            // return ;
        }

        if (_onFrame) {
            _onFrame(frame);
        }
    }
}

void TsDemuxer::setOnDecode(const function<void(const FrameBuffer::Ptr& frame)> cb)
{
    _onFrame = cb;
}

void TsDemuxer::createTrackInfo(const string& codec, int type)
{
    if (type == TsPidTypeVideo) {
        _videoCodec = type;
    } else if (type == TsPidTypeAudio) {
        _audioCodec = type;
    }

    addTrackInfo(TrackInfo::createTrackInfo(codec));
    // if (codec == "aac") {
    //     // auto trackInfo = make_shared<AacTrack>();
    //     // trackInfo->index_ = AudioTrackType;
    //     // trackInfo->codec_ = "aac";
    //     // trackInfo->trackType_ = "audio";
    //     // trackInfo->samplerate_ = 90000;
    //     // trackInfo->payloadType_ = 97;
    //     auto trackInfo = AacTrack::createTrack(AudioTrackType, 97, 90000);
    //     addTrackInfo(trackInfo);

    //     _audioCodec = "aac";
    // } else if (codec == "g711a") {
    //     // auto trackInfo = make_shared<G711aTrack>();
    //     // trackInfo->index_ = AudioTrackType;
    //     // trackInfo->codec_ = "g711a";
    //     // trackInfo->trackType_ = "audio";
    //     auto trackInfo = G711aTrack::createTrack(AudioTrackType, 8, 8000);
    //     addTrackInfo(trackInfo);

    //     _videoCodec = "g711a";
    // } else if (codec == "g711u") {
    //     // auto trackInfo = make_shared<G711uTrack>();
    //     // trackInfo->index_ = AudioTrackType;
    //     // trackInfo->codec_ = "g711u";
    //     // trackInfo->trackType_ = "audio";
    //     auto trackInfo = G711uTrack::createTrack(AudioTrackType, 0, 8000);
    //     addTrackInfo(trackInfo);

    //     _videoCodec = "g711u";
    // } else if (codec == "mp3") {
    //     auto trackInfo = Mp3Track::createTrack(AudioTrackType, 14, 44100);
    //     addTrackInfo(trackInfo);

    //     _videoCodec = "mp3";
    // } else if (codec == "h264") {
    //     auto trackInfo = H264Track::createTrack(VideoTrackType, 96, 90000);
    //     addTrackInfo(trackInfo);

    //     _videoCodec = "h264";
    // } else if (codec == "h265") {
    //     auto trackInfo = make_shared<H265Track>();
    //     trackInfo->index_ = VideoTrackType;
    //     trackInfo->codec_ = "h265";
    //     trackInfo->trackType_ = "video";
    //     trackInfo->samplerate_ = 90000;
    //     trackInfo->payloadType_ = 96;
    //     addTrackInfo(trackInfo);

    //     _videoCodec = "h265";
    // }
}

void TsDemuxer::addTrackInfo(const shared_ptr<TrackInfo>& trackInfo)
{
    auto it = _mapTrackInfo.find(trackInfo->index_);
    if (it == _mapTrackInfo.end()) {
        _mapTrackInfo[trackInfo->index_] = trackInfo;
        
        if (_onTrackInfo) {
            _onTrackInfo(trackInfo);
        }

        // if ((_hasVideo ? _videoCodec != "" : true) && 
        //     (_hasAudio ? _audioCodec != "" : true) && 
        //     _onReady && _audioCodec != "aac") {
        //     _onReady();
        // }
    } else if (it->second->codec_ != trackInfo->codec_) {
        _mapTrackInfo[trackInfo->index_] = trackInfo;
        // _resetTrackInfo(trackInfo);
    }
}

void TsDemuxer::setOnTrackInfo(const function<void(const shared_ptr<TrackInfo>& trackInfo)>& cb)
{
    _onTrackInfo = cb;
}

void TsDemuxer::setOnReady(const function<void()>& cb)
{
    _onReady = cb;
}

bool searchTs(char* data, int size, int& pos)
{
    int index = 0;
    while (true) {
        if (data[index] == 0x47) {
            if (index + tsPacketSize < size) {
                if (data[index + tsPacketSize] == 0x47) {
                    pos = index;
                    return true;
                }
            } else {
                pos = index;
                return false;
            }
        }

        if (++index >= size) {
            return false;
        }
    }

    return false;
}

void TsDemuxer::onTsPacket(char* data, int size, uint32_t timestamp)
{
    if (_remainBuffer.size() > 0) {
        _remainBuffer.append(data, size);
        data = _remainBuffer.data();
        size = _remainBuffer.size();
        _remainBuffer.clear();
    }

    logDebug << "input size: " << size;
    logDebug << "timestamp: " << timestamp;

    while (size >= tsPacketSize) {
        if(data[0] != 0x47) {
            logInfo << "ts packet error, the first packet != 0x47, start search the ts packet";
            int pos = -1;
            bool ret = searchTs(data, size, pos);
            if (ret && pos > 0) {
                data += pos;
                size -= pos;
                logInfo << "search ths ts in pos: " << pos;
            } else if (!ret && pos > 0) {
                data += pos;
                size -= pos;
                logInfo << "search a 0x47 in pos: " << pos;
                break;
            } else {
                logInfo << "can not find ts";
                return;
            }
        }
        TsPacket packet;
        StreamBuffer::Ptr buffer = StreamBuffer::create();
        buffer->move(data, tsPacketSize, false);

        packet.demux(this, buffer);

        data += tsPacketSize;
        size -= tsPacketSize;
    }

    logDebug << "after demux size: " << size;

    if (size != 0) {
        _remainBuffer.assign(data, size);
    }
}

void TsDemuxer::clear()
{
    _remainBuffer.clear();
    _videoStream.clear();
    // pidInfos_.clear();
    // msgs_.clear();
}