#ifndef SrtDataPacket_H
#define SrtDataPacket_H

#include <string>
#include <memory>

#include "Net/Buffer.h"

/*
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+- SRT Header +-+-+-+-+-+-+-+-+-+-+-+-+-+
|0|                    Packet Sequence Number                   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|P P|O|K K|R|                   Message Number                  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Destination Socket ID                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                              Data                             +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            Figure 3: Data packet structure
            reference https://haivision.github.io/srt-rfc/draft-sharabayko-srt.html#name-packet-structure
*/

class DataPacket
{
public:
    using Ptr = std::shared_ptr<DataPacket>;

    DataPacket() {}
    ~DataPacket() {}

    // 设置函数
    void setF(uint8_t value) { f_ = value; }
    void setPacketSeqNumber(uint32_t value) { packet_seq_number_ = value; }
    void setPP(uint8_t value) { pp_ = value; }
    void setO(uint8_t value) { o_ = value; }
    void setKK(uint8_t value) { kk_ = value; }
    void setR(uint8_t value) { r_ = value; }
    void setMsgNumber(uint32_t value) { msg_number_ = value; }
    void setTimestamp(uint32_t value) { timestamp_ = value; }
    void setDstSocketId(uint32_t value) { dst_socket_id_ = value; }
    void setData(const StreamBuffer::Ptr& value) { data_ = value; }

    // 获取函数
    uint8_t getF() const { return f_; }
    uint32_t getPacketSeqNumber() const { return packet_seq_number_; }
    uint8_t getPP() const { return pp_; }
    uint8_t getO() const { return o_; }
    uint8_t getKK() const { return kk_; }
    uint8_t getR() const { return r_; }
    uint32_t getMsgNumber() const { return msg_number_; }
    uint32_t getTimestamp() const { return timestamp_; }
    uint32_t getDstSocketId() const { return dst_socket_id_; }
    StreamBuffer::Ptr getData() const { return data_; }
    size_t getLength() const { return length_; }

    bool parse(uint8_t *buf, size_t len);
    StreamBuffer::Ptr encode(const uint8_t *data, size_t len);

public:
    static bool isDataPacket(uint8_t *buf, size_t len);
    static uint32_t getDstSocketID(uint8_t *buf, size_t len);

protected:
    uint8_t f_;
    uint32_t packet_seq_number_;
    uint8_t pp_;
    uint8_t o_;
    uint8_t kk_;
    uint8_t r_;
    uint32_t msg_number_;
    uint32_t timestamp_;
    uint32_t dst_socket_id_;
    size_t length_ = 0;
    StreamBuffer::Ptr data_;
};

#endif //SrtDataPacket_H
