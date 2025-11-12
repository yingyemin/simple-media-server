#ifndef SrtControlPacket_H
#define SrtControlPacket_H

#include <string>
#include <memory>
#include <chrono>
#include <list>

#include "Net/Buffer.h"

enum ControlPacketType {
    HANDSHAKE = 0x0000,
    KEEPALIVE = 0x0001,
    ACK = 0x0002,
    NAK = 0x0003,
    CONGESTIONWARNING = 0x0004,
    SHUTDOWN = 0x0005,
    ACKACK = 0x0006,
    DROPREQ = 0x0007,
    PEERERROR = 0x0008,
    USERDEFINEDTYPE = 0x7FFF
};

enum HandshakeType {
    HS_TYPE_DONE = 0xFFFFFFFD,
    HS_TYPE_AGREEMENT = 0xFFFFFFFE,
    HS_TYPE_CONCLUSION = 0xFFFFFFFF,
    HS_TYPE_WAVEHAND = 0x00000000,
    HS_TYPE_INDUCTION = 0x00000001
};

enum HsEncryptionType { 
    NO_ENCRYPTION = 0, 
    AES_128 = 1, 
    AES_196 = 2, 
    AES_256 = 3
};

enum HsExtType {
    SRT_CMD_REJECT = 0,
    SRT_CMD_HSREQ = 1,
    SRT_CMD_HSRSP = 2,
    SRT_CMD_KMREQ = 3,
    SRT_CMD_KMRSP = 4,
    SRT_CMD_SID = 5,
    SRT_CMD_CONGESTION = 6,
    SRT_CMD_FILTER = 7,
    SRT_CMD_GROUP = 8,
    SRT_CMD_NONE = -1
};

enum HsExtField { 
    HS_EXT_FILED_HSREQ = 0x00000001, 
    HS_EXT_FILED_KMREQ = 0x00000002, 
    HS_EXT_FILED_CONFIG = 0x00000004, 
    HS_EXT_FILED_SID = 0x00000008, 
    HS_EXT_FILED_CONG = 0x00000010, 
    HS_EXT_FILED_FILTER = 0x00000020, 
    HS_EXT_FILED_GROUP = 0x00000040 
};

enum HsExtMsg {
    HS_EXT_MSG_TSBPDSND = 0x00000001,
    HS_EXT_MSG_TSBPDRCV = 0x00000002,
    HS_EXT_MSG_CRYPT = 0x00000004,
    HS_EXT_MSG_TLPKTDROP = 0x00000008,
    HS_EXT_MSG_PERIODICNAK = 0x00000010,
    HS_EXT_MSG_REXMITFLG = 0x00000020,
    HS_EXT_MSG_STREAM = 0x00000040,
    HS_EXT_MSG_PACKET_FILTER = 0x00000080
};

/*
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+- SRT Header +-+-+-+-+-+-+-+-+-+-+-+-+-+
|1|         Control Type        |            Subtype            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                   Type-specific Information                   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Destination Socket ID                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- CIF -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                   Control Information Field                   +
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
            Figure 4: Control packet structure
             reference https://haivision.github.io/srt-rfc/draft-sharabayko-srt.html#name-control-packets
*/
class ControlPacket
{
public:
    using Ptr = std::shared_ptr<ControlPacket>;

    ControlPacket() {}
    virtual ~ControlPacket() {}

    // 设置函数
    void setControlType(uint16_t value) { control_type_ = value; }
    void setSubtype(uint16_t value) { subtype_ = value; }
    void setTypeSpecificInfo(uint32_t value) { type_specific_info_ = value; }
    void setTimestamp(uint32_t value) { timestamp_ = value; }
    void setDstSocketId(uint32_t value) { dst_socket_id_ = value; }
    void setControlInfo(const StreamBuffer::Ptr& value) { control_info_ = value; }

    // 获取函数
    uint16_t getControlType() const { return control_type_; }
    uint16_t getSubtype() const { return subtype_; }
    uint32_t getTypeSpecificInfo() const { return type_specific_info_; }
    uint32_t getTimestamp() const { return timestamp_; }
    uint32_t getDstSocketId() const { return dst_socket_id_; }
    StreamBuffer::Ptr getControlInfo() const { return control_info_; }

    virtual bool parse(uint8_t *buf, size_t len);
    virtual StreamBuffer::Ptr encode();

public:
    static bool isControlPacket(uint8_t *buf, size_t len);
    static uint16_t getControlType(uint8_t *buf, size_t len);
    static uint32_t getDstSocketID(uint8_t *buf, size_t len);

private:
    uint16_t control_type_;
    uint16_t subtype_;
    uint32_t type_specific_info_ = 0;
    uint32_t timestamp_;
    uint32_t dst_socket_id_;
    StreamBuffer::Ptr control_info_;
};

// 定义扩展数据结构
class HandshakeExtension 
{
public:
    virtual ~HandshakeExtension() {}
    virtual bool parse(uint8_t *buf, size_t len) = 0;
    virtual StreamBuffer::Ptr encode() = 0;

public:
    uint16_t type;
    uint16_t length;
    std::string contents;
};

/*
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          SRT Version                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           SRT Flags                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|      Receiver TSBPD Delay     |       Sender TSBPD Delay      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    Figure 6: Handshake Extension Message structure
    https://haivision.github.io/srt-rfc/draft-sharabayko-srt.html#name-handshake-extension-message

*/
class HsExtMessage : public HandshakeExtension
{
public:
    bool parse(uint8_t *buf, size_t len) override;
    StreamBuffer::Ptr encode() override;

public:
    uint32_t srtVersion;
    uint32_t srtFlag;
    uint16_t recvTsbpdDelay;
    uint16_t sendTsbpdDelay;
};

/*
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
|                           Stream ID                           |
                               ...
|                                                               |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    Figure 7: Stream ID Extension Message
    https://haivision.github.io/srt-rfc/draft-sharabayko-srt.html#name-stream-id-extension-message
*/
class HsExtStreamId : public HandshakeExtension
{
public:
    bool parse(uint8_t *buf, size_t len) override;
    StreamBuffer::Ptr encode() override;

public:
    std::string streamId;
};

/**
  0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                            Version                            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|        Encryption Field       |        Extension Field        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                 Initial Packet Sequence Number                |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                 Maximum Transmission Unit Size                |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Maximum Flow Window Size                   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Handshake Type                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         SRT Socket ID                         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           SYN Cookie                          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                                                               +
|                                                               |
+                        Peer IP Address                        +
|                                                               |
+                                                               +
|                                                               |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
|         Extension Type        |        Extension Length       |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                                                               |
+                       Extension Contents                      +
|                                                               |
+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
    Figure 5: Handshake packet structure
    https://haivision.github.io/srt-rfc/draft-sharabayko-srt.html#name-handshake
 */
class HandshakePacket : public ControlPacket
{
public:
    using Ptr = std::shared_ptr<HandshakePacket>;

    HandshakePacket() {}
    ~HandshakePacket() override {}

    // 设置函数
    void setVersion(uint32_t version) { version_ = version; }
    void setEncryptionField(uint16_t encryptionField) { encryption_field_ = encryptionField; }
    void setExtensionField(uint16_t extensionField) { extension_field_ = extensionField; }
    void setInitialPacketSequenceNumber(uint32_t sequenceNumber) { initial_packet_sequence_number_ = sequenceNumber; }
    void setMaximumTransmissionUnitSize(uint32_t mtuSize) { maximum_transmission_unit_size_ = mtuSize; }
    void setMaximumFlowWindowSize(uint32_t flowWindowSize) { maximum_flow_window_size_ = flowWindowSize; }
    void setHandshakeType(uint32_t handshakeType) { handshake_type_ = handshakeType; }
    void setSrtSocketId(uint32_t socketId) { srt_socket_id_ = socketId; }
    void setSynCookie(uint32_t synCookie) { syn_cookie_ = synCookie; }
    void setPeerIPAddress(const std::string& ipAddress) { memcpy(peer_ip_address_, ipAddress.c_str(), ipAddress.size()); }
    
    // 获取函数
    uint32_t getVersion() const { return version_; }
    uint16_t getEncryptionField() const { return encryption_field_; }
    uint16_t getExtensionField() const { return extension_field_; }
    uint32_t getInitialPacketSequenceNumber() const { return initial_packet_sequence_number_; }
    uint32_t getMaximumTransmissionUnitSize() const { return maximum_transmission_unit_size_; }
    uint32_t getMaximumFlowWindowSize() const { return maximum_flow_window_size_; }
    uint32_t getHandshakeType() const { return handshake_type_; }
    uint32_t getSrtSocketId() const { return srt_socket_id_; }
    uint32_t getSynCookie() const { return syn_cookie_; }
    std::string getPeerIPAddress() const { return (char*)peer_ip_address_; }

    bool parse(uint8_t *buf, size_t len) override;
    StreamBuffer::Ptr encode() override;

    void assignPeerIP(struct sockaddr_storage *addr);

public:
    static bool isHandshakePacket(uint8_t *buf, size_t len);
    static uint32_t getHandshakeType(uint8_t *buf, size_t len);
    static uint32_t getSynCookie(uint8_t *buf, size_t len);
    static uint32_t generateSynCookie(struct sockaddr_storage *addr, 
        uint64_t ts, uint32_t current_cookie = 0, int correction = 0);

public:
    std::vector<std::shared_ptr<HandshakeExtension>> extensions_;

private:
    uint32_t version_;
    uint16_t encryption_field_;
    uint16_t extension_field_;
    uint32_t initial_packet_sequence_number_;
    uint32_t maximum_transmission_unit_size_;
    uint32_t maximum_flow_window_size_;
    uint32_t handshake_type_;
    uint32_t srt_socket_id_;
    uint32_t syn_cookie_;
    uint8_t peer_ip_address_[16];
};

/**
  0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+- SRT Header +-+-+-+-+-+-+-+-+-+-+-+-+-+
|1|         Control Type        |            Reserved           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                   Type-specific Information                   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Destination Socket ID                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    Figure 12: Keep-Alive control packet
    https://haivision.github.io/srt-rfc/draft-sharabayko-srt.html#name-keep-alive
 */
class KeepAlivePacket : public ControlPacket
{
public:
    using Ptr = std::shared_ptr<KeepAlivePacket>;

    KeepAlivePacket() {
        // 保留字段初始化为 0
        setSubtype(0); 
    }
    ~KeepAlivePacket() override {}

    bool parse(uint8_t *buf, size_t len) override {
        // 直接调用基类的解析函数
        return ControlPacket::parse(buf, len);
    }

    StreamBuffer::Ptr encode() override {
        // 直接调用基类的编码函数
        return ControlPacket::encode();
    }
};

/**
  0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+- SRT Header +-+-+-+-+-+-+-+-+-+-+-+-+-+
|1|        Control Type         |           Reserved            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                   Type-specific Information                   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Destination Socket ID                     |
+-+-+-+-+-+-+-+-+-+-+-+- CIF (Loss List) -+-+-+-+-+-+-+-+-+-+-+-+
|0|                 Lost packet sequence number                 |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|1|         Range of lost packets from sequence number          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|0|                    Up to sequence number                    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|0|                 Lost packet sequence number                 |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    Figure 14: NAK control packet
    https://haivision.github.io/srt-rfc/draft-sharabayko-srt.html#name-nak-control-packet
 */
class NakPacket : public ControlPacket
{
public:
    using Ptr = std::shared_ptr<NakPacket>;

    struct LossInfo {
        bool isRange;
        uint32_t seqNumber;
        uint32_t endSeqNumber; // 当 isRange 为 true 时有效
    };

    NakPacket() {
        // 保留字段初始化为 0
        setSubtype(0); 
    }
    ~NakPacket() override {}

    // 添加丢失信息
    void addLossInfo(const std::pair<uint32_t, uint32_t>& info) {
        lossList_.push_back(info);
    }

    // 获取丢失信息列表
    const std::list<std::pair<uint32_t, uint32_t>>& getLossList() const {
        return lossList_;
    }

    bool parse(uint8_t *buf, size_t len) override;
    StreamBuffer::Ptr encode() override;

    static size_t getCIFSize(std::list<std::pair<uint32_t, uint32_t>> &lost);

public:
    std::list<std::pair<uint32_t, uint32_t>> lossList_;
};

/**
  0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+- SRT Header +-+-+-+-+-+-+-+-+-+-+-+-+-+
|1|      Control Type = 7       |         Reserved = 0          |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                        Message Number                         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Destination Socket ID                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                 First Packet Sequence Number                  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                  Last Packet Sequence Number                  |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    Figure 18: Drop Request control packet
    https://haivision.github.io/srt-rfc/draft-sharabayko-srt.html#name-message-drop-request
 */
class MsgDropReqPacket : public ControlPacket
{
public:
    using Ptr = std::shared_ptr<MsgDropReqPacket>;

    MsgDropReqPacket() {
        // 设置 Control Type 为 7，保留字段为 0
        setControlType(7);
        setSubtype(0); 
    }
    ~MsgDropReqPacket() override {}

    // 设置消息编号
    void setMessageNumber(uint32_t number) {
        messageNumber_ = number;
    }

    // 获取消息编号
    uint32_t getMessageNumber() const {
        return messageNumber_;
    }

    // 设置首个数据包序列号
    void setFirstPacketSeqNumber(uint32_t seqNumber) {
        firstPacketSeqNumber_ = seqNumber;
    }

    // 获取首个数据包序列号
    uint32_t getFirstPacketSeqNumber() const {
        return firstPacketSeqNumber_;
    }

    // 设置最后一个数据包序列号
    void setLastPacketSeqNumber(uint32_t seqNumber) {
        lastPacketSeqNumber_ = seqNumber;
    }

    // 获取最后一个数据包序列号
    uint32_t getLastPacketSeqNumber() const {
        return lastPacketSeqNumber_;
    }

    bool parse(uint8_t *buf, size_t len) override;
    StreamBuffer::Ptr encode() override;

private:
    uint32_t messageNumber_;
    uint32_t firstPacketSeqNumber_;
    uint32_t lastPacketSeqNumber_;
};

/**
  0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+- SRT Header +-+-+-+-+-+-+-+-+-+-+-+-+-+
|1|        Control Type         |           Reserved            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                   Type-specific Information                   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Destination Socket ID                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    Figure 16: Shutdown control packet
    https://haivision.github.io/srt-rfc/draft-sharabayko-srt.html#name-shutdown
 */
class ShutdownPacket : public ControlPacket
{
public:
    using Ptr = std::shared_ptr<ShutdownPacket>;

    ShutdownPacket() {
        // 保留字段初始化为 0
        setSubtype(0); 
    }
    ~ShutdownPacket() override {}

    bool parse(uint8_t *buf, size_t len) override {
        // 直接调用基类的解析函数
        return ControlPacket::parse(buf, len);
    }

    StreamBuffer::Ptr encode() override {
        // 直接调用基类的编码函数
        return ControlPacket::encode();
    }
};

/**
  0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+- SRT Header +-+-+-+-+-+-+-+-+-+-+-+-+-+
|1|        Control Type         |           Reserved            |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                    Acknowledgement Number                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Destination Socket ID                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+- CIF -+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|            Last Acknowledged Packet Sequence Number           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                              RTT                              |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                          RTT Variance                         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Available Buffer Size                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Packets Receiving Rate                    |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Estimated Link Capacity                   |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                         Receiving Rate                        |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    Figure 13: ACK control packet
    https://haivision.github.io/srt-rfc/draft-sharabayko-srt.html#name-ack-acknowledgment
*/

class AckPacket : public ControlPacket
{
public:
    using Ptr = std::shared_ptr<AckPacket>;

    AckPacket() {
        // 设置 Control Type 为 2，保留字段为 0
        setControlType(ACK);
        setSubtype(0); 
    }
    ~AckPacket() override {}

    // 设置确认号
    void setAckNumber(uint32_t number) {
        ack_number_ = number;
    }

    // 获取确认号
    uint32_t getAckNumber() const {
        return ack_number_;
    }

    // 设置最后一个确认的数据包序列号
    void setLastAckPacketSeqNumber(uint32_t seqNumber) {
        last_ack_packet_seq_number_ = seqNumber;
    }

    // 获取最后一个确认的数据包序列号
    uint32_t getLastAckPacketSeqNumber() const {
        return last_ack_packet_seq_number_;
    }

    // 设置 RTT
    void setRtt(uint32_t rtt) {
        rtt_ = rtt;
    }

    // 获取 RTT
    uint32_t getRtt() const {
        return rtt_;
    }

    // 设置 RTT 方差
    void setRttVariance(uint32_t variance) {
        rtt_variance_ = variance;
    }

    // 获取 RTT 方差
    uint32_t getRttVariance() const {
        return rtt_variance_;
    }

    // 设置可用缓冲区大小
    void setAvailableBufferSize(uint32_t size) {
        available_buffer_size_ = size;
    }

    // 获取可用缓冲区大小
    uint32_t getAvailableBufferSize() const {
        return available_buffer_size_;
    }

    // 设置接收速率
    void setPacketsReceivingRate(uint32_t rate) {
        packets_receiving_rate_ = rate;
    }

    // 获取接收速率
    uint32_t getPacketsReceivingRate() const {
        return packets_receiving_rate_;
    }

    // 设置估计的链路容量
    void setEstimatedLinkCapacity(uint32_t capacity) {
        estimated_link_capacity_ = capacity;
    }

    // 获取估计的链路容量
    uint32_t getEstimatedLinkCapacity() const {
        return estimated_link_capacity_;
    }

    // 设置接收速率
    void setReceivingRate(uint32_t rate) {
        receiving_rate_ = rate;
    }

    // 获取接收速率
    uint32_t getReceivingRate() const {
        return receiving_rate_;
    }

    bool parse(uint8_t *buf, size_t len) override;
    StreamBuffer::Ptr encode() override;

private:
    uint32_t ack_number_;
    uint32_t last_ack_packet_seq_number_;
    uint32_t rtt_;
    uint32_t rtt_variance_;
    uint32_t available_buffer_size_;
    uint32_t packets_receiving_rate_;
    uint32_t estimated_link_capacity_;
    uint32_t receiving_rate_;
};

/**
  0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+- SRT Header +-+-+-+-+-+-+-+-+-+-+-+-+-+
|1|         Control Type        |            Reserved           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                   Acknowledgement Number                      |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                           Timestamp                           |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|                     Destination Socket ID                     |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    Figure 12: Keep-Alive control packet
    https://haivision.github.io/srt-rfc/draft-sharabayko-srt.html#name-keep-alive
 */
class AckAckPacket : public ControlPacket
{
public:
    using Ptr = std::shared_ptr<AckAckPacket>;

    AckAckPacket() {
        // 保留字段初始化为 0
        setControlType(ACKACK);
        setSubtype(0); 
    }
    ~AckAckPacket() override {}

    void setAckNumber(uint32_t number) {
        ack_number_ = number;
    }

    uint32_t getAckNumber() const {
        return ack_number_;
    }

    bool parse(uint8_t *buf, size_t len) override;

    StreamBuffer::Ptr encode() override;

private:
    uint32_t ack_number_;
};

#endif //SrtControlPacket_H
