#include "SrtControlPacket.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/MD5.h"
#include "Net/Socket.h"
#include "Util/TimeClock.h"

#include <cstring>
#include <atomic>

using namespace std;

#define HEADER_SIZE 16

bool ControlPacket::parse(uint8_t *buf, size_t len) {
    if (len < 12) { // 头部至少 12 字节
        return false;
    }

    // 解析 Control Type 和 Subtype
    control_type_ = ((buf[0] & 0x7F) << 8) | buf[1];
    subtype_ = (buf[2] << 8) | buf[3];

    // 解析 Type-specific Information
    type_specific_info_ = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];

    // 解析 Timestamp
    timestamp_ = (buf[8] << 24) | (buf[9] << 16) | (buf[10] << 8) | buf[11];

    // 解析 Destination Socket ID
    if (len >= 16) {
        dst_socket_id_ = (buf[12] << 24) | (buf[13] << 16) | (buf[14] << 8) | buf[15];
    }

    // 解析 Control Information Field
    if (len > 16) {
        control_info_ = std::make_shared<StreamBuffer>((char*)buf + 16, len - 16);
    }

    return true;
}

StreamBuffer::Ptr ControlPacket::encode() {
    size_t controlInfoSize = control_info_ ? control_info_->size() : 0;
    size_t totalSize = 16 + controlInfoSize;
    auto buffer = std::make_shared<StreamBuffer>(totalSize + 1);
    uint8_t *buf = (uint8_t*)buffer->data();

    // 编码 Control Type 和 Subtype
    buf[0] = 0x80 | ((control_type_ >> 8) & 0x7F);
    buf[1] = control_type_ & 0xFF;
    buf[2] = subtype_ >> 8;
    buf[3] = subtype_ & 0xFF;

    // 编码 Type-specific Information
    buf[4] = type_specific_info_ >> 24;
    buf[5] = (type_specific_info_ >> 16) & 0xFF;
    buf[6] = (type_specific_info_ >> 8) & 0xFF;
    buf[7] = type_specific_info_ & 0xFF;

    // 编码 Timestamp
    buf[8] = timestamp_ >> 24;
    buf[9] = (timestamp_ >> 16) & 0xFF;
    buf[10] = (timestamp_ >> 8) & 0xFF;
    buf[11] = timestamp_ & 0xFF;

    // 编码 Destination Socket ID
    buf[12] = dst_socket_id_ >> 24;
    buf[13] = (dst_socket_id_ >> 16) & 0xFF;
    buf[14] = (dst_socket_id_ >> 8) & 0xFF;
    buf[15] = dst_socket_id_ & 0xFF;

    // 编码 Control Information Field
    if (control_info_) {
        memcpy(buf + 16, control_info_->data(), controlInfoSize);
    }

    return buffer;
}

bool ControlPacket::isControlPacket(uint8_t *buf, size_t len) {
    if (len < 16) {
        logInfo << "data size" << len << " less " << 16;
        return false;
    }
    if (buf[0] & 0x80) {
        return true;
    }
    return false;
}

uint16_t ControlPacket::getControlType(uint8_t *buf, size_t len) {
    uint8_t *ptr = buf;
    uint16_t control_type = (ptr[0] & 0x7f) << 8 | ptr[1];
    return control_type;
}

uint32_t ControlPacket::getDstSocketID(uint8_t *buf, size_t len) {
    return readUint32BE((char*)buf + 12);
}

bool HsExtMessage::parse(uint8_t *buf, size_t len) { 
    if (len < 12) {
        return false;
    }

    // 解析 SRT 版本
    srtVersion = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    // 解析 SRT 标志
    srtFlag = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
    // 解析接收方 TSBPD 延迟
    recvTsbpdDelay = (buf[8] << 8) | buf[9];
    // 解析发送方 TSBPD 延迟
    sendTsbpdDelay = (buf[10] << 8) | buf[11];

    // 设置扩展类型和长度
    type = 0x0001; // 假设 HsExtMessage 的扩展类型为 0x0001
    length = 12;
    contents = std::string(reinterpret_cast<const char*>(buf), length);

    return true;
}

StreamBuffer::Ptr HsExtMessage::encode() {
    size_t totalSize = 16;
    auto buffer = std::make_shared<StreamBuffer>(totalSize + 1);
    char *buf = buffer->data();

    length = (totalSize - 4) / 4;
    // type = SRT_CMD_SID;

    writeUint16BE(buf, type);
    buf += 2;
    writeUint16BE(buf, length);
    buf += 2;

    // 填充剩余部分
    writeUint32BE(buf, srtVersion);
    buf += 4;
    // 编码 SRT 版本
    writeUint32BE(buf, srtFlag);
    buf += 4;

    writeUint16BE(buf, recvTsbpdDelay);
    buf += 2;

    writeUint16BE(buf, sendTsbpdDelay);
    buf += 2;

    return buffer;
}

bool HsExtStreamId::parse(uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len / 4; ++i) {
        streamId.push_back(*(buf + 3));
        streamId.push_back(*(buf + 2));
        streamId.push_back(*(buf + 1));
        streamId.push_back(*(buf));
        buf += 4;
    }
    char zero = 0x00;
    if (streamId.back() == zero) {
        streamId.erase(streamId.find_first_of(zero), streamId.size());
    }

    return true;
}

StreamBuffer::Ptr HsExtStreamId::encode() {
    size_t totalSize = ((streamId.length() + 4) + 3) / 4 * 4;

    auto buffer = std::make_shared<StreamBuffer>(totalSize + 1);
    char *ptr = buffer->data();

    length = (totalSize - 4) / 4;
    type = SRT_CMD_SID;

    writeUint16BE(ptr, type);
    ptr += 2;
    writeUint16BE(ptr, length);
    ptr += 2;

    const char *src = streamId.c_str();
    for (size_t i = 0; i < streamId.length() / 4; ++i) {
        *ptr = *(src + 3 + i * 4);
        ptr++;

        *ptr = *(src + 2 + i * 4);
        ptr++;

        *ptr = *(src + 1 + i * 4);
        ptr++;

        *ptr = *(src + 0 + i * 4);
        ptr++;
    }

    ptr += 3;
    size_t offset = streamId.length() / 4 * 4;
    for (size_t i = 0; i < streamId.length() % 4; ++i) {
        *ptr = *(src + offset + i);
        ptr -= 1;
    }

    return buffer;
}

bool HandshakePacket::parse(uint8_t *buf, size_t len) {
    if (len < HEADER_SIZE + 48) {
        logWarn << "handshake packet len: " << len << " less " << HEADER_SIZE + 48;
        return false;
    }

    // 先调用基类的 parse 函数解析 ControlPacket 部分
    if (!ControlPacket::parse(buf, len)) {
        return false;
    }

    // 跳过 ControlPacket 头部的 16 字节
    buf += 16;
    len -= 16;

    // 检查剩余长度是否足够解析基本握手包信息
    if (len < 48) {
        return false;
    }

    // 解析版本号
    version_ = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    // 解析加密字段
    encryption_field_ = (buf[4] << 8) | buf[5];
    // 解析扩展字段
    extension_field_ = (buf[6] << 8) | buf[7];
    // 解析初始包序列号
    initial_packet_sequence_number_ = (buf[8] << 24) | (buf[9] << 16) | (buf[10] << 8) | buf[11];
    // 解析最大传输单元大小
    maximum_transmission_unit_size_ = (buf[12] << 24) | (buf[13] << 16) | (buf[14] << 8) | buf[15];
    // 解析最大流窗口大小
    maximum_flow_window_size_ = (buf[16] << 24) | (buf[17] << 16) | (buf[18] << 8) | buf[19];
    // 解析握手类型
    handshake_type_ = (buf[20] << 24) | (buf[21] << 16) | (buf[22] << 8) | buf[23];
    // 解析 SRT 套接字 ID
    srt_socket_id_ = (buf[24] << 24) | (buf[25] << 16) | (buf[26] << 8) | buf[27];
    // 解析 SYN Cookie
    syn_cookie_ = (buf[28] << 24) | (buf[29] << 16) | (buf[30] << 8) | buf[31];

    // 移动指针到 Peer IP Address 部分
    buf += 32;
    len -= 32;

    // 解析 Peer IP Address，假设为 IPv4 地址
    if (len >= 16) {
        memcpy(peer_ip_address_, buf, sizeof(peer_ip_address_));
        buf += sizeof(peer_ip_address_);
        len -= sizeof(peer_ip_address_);
    }

    // 解析扩展部分
    extensions_.clear();
    while (len >= 4) {
        uint16_t type = (buf[0] << 8) | buf[1];
        // 解析扩展长度
        uint16_t length = (buf[2] << 8) | buf[3];
        buf += 4;
        len -= 4;

        shared_ptr<HandshakeExtension> ext;
        switch (type) {
            case SRT_CMD_HSREQ:
            case SRT_CMD_HSRSP: ext = std::make_shared<HsExtMessage>(); break;
            case SRT_CMD_SID: ext = std::make_shared<HsExtStreamId>(); break;
            default: logWarn << "not support ext " << type; break;
        }
        if (!ext) {
            continue;
        }
        ext->type = type;
        ext->length = length;

        // 解析扩展内容
        if (len >= ext->length * 4) {
            ext->contents = std::string(reinterpret_cast<char*>(buf), ext->length*4);
            buf += ext->length * 4;
            len -= ext->length * 4;
            ext->parse((uint8_t*)ext->contents.c_str(), ext->contents.size());
            extensions_.push_back(ext);
        } else {
            break;
        }
    }

    return true;
}

StreamBuffer::Ptr HandshakePacket::encode() {
    // 计算扩展内容长度
    size_t extensionContentLen = 0;
    for (const auto& ext : extensions_) {
        extensionContentLen += ext->length * 4 + 4;
    }
    // 计算总长度
    size_t totalSize = 16 + 32 + 16 + extensionContentLen;
    auto buffer = std::make_shared<StreamBuffer>(totalSize + 1);
    uint8_t *buf = (uint8_t*)buffer->data();

    setControlType(HANDSHAKE);
    setSubtype(0);

    // 先调用基类的 encode 函数编码 ControlPacket 部分
    auto baseBuffer = ControlPacket::encode();
    memcpy(buf, baseBuffer->data(), 16);
    buf += 16;

    // 编码版本号
    buf[0] = version_ >> 24;
    buf[1] = (version_ >> 16) & 0xFF;
    buf[2] = (version_ >> 8) & 0xFF;
    buf[3] = version_ & 0xFF;
    // 编码加密字段
    buf[4] = encryption_field_ >> 8;
    buf[5] = encryption_field_ & 0xFF;
    // 编码扩展字段
    buf[6] = extension_field_ >> 8;
    buf[7] = extension_field_ & 0xFF;
    // 编码初始包序列号
    buf[8] = initial_packet_sequence_number_ >> 24;
    buf[9] = (initial_packet_sequence_number_ >> 16) & 0xFF;
    buf[10] = (initial_packet_sequence_number_ >> 8) & 0xFF;
    buf[11] = initial_packet_sequence_number_ & 0xFF;
    // 编码最大传输单元大小
    buf[12] = maximum_transmission_unit_size_ >> 24;
    buf[13] = (maximum_transmission_unit_size_ >> 16) & 0xFF;
    buf[14] = (maximum_transmission_unit_size_ >> 8) & 0xFF;
    buf[15] = maximum_transmission_unit_size_ & 0xFF;
    // 编码最大流窗口大小
    buf[16] = maximum_flow_window_size_ >> 24;
    buf[17] = (maximum_flow_window_size_ >> 16) & 0xFF;
    buf[18] = (maximum_flow_window_size_ >> 8) & 0xFF;
    buf[19] = maximum_flow_window_size_ & 0xFF;
    // 编码握手类型
    buf[20] = handshake_type_ >> 24;
    buf[21] = (handshake_type_ >> 16) & 0xFF;
    buf[22] = (handshake_type_ >> 8) & 0xFF;
    buf[23] = handshake_type_ & 0xFF;
    // 编码 SRT 套接字 ID
    buf[24] = srt_socket_id_ >> 24;
    buf[25] = (srt_socket_id_ >> 16) & 0xFF;
    buf[26] = (srt_socket_id_ >> 8) & 0xFF;
    buf[27] = srt_socket_id_ & 0xFF;
    // 编码 SYN Cookie
    buf[28] = syn_cookie_ >> 24;
    buf[29] = (syn_cookie_ >> 16) & 0xFF;
    buf[30] = (syn_cookie_ >> 8) & 0xFF;
    buf[31] = syn_cookie_ & 0xFF;
    buf += 32;

    // 编码 Peer IP Address，假设为 IPv4 地址
    memcpy(buf, (char*)peer_ip_address_, 16);
    buf += 16;

    // 编码扩展部分
    // 编码扩展类型
    // buf[0] = extension_type_ >> 8;
    // buf[1] = extension_type_ & 0xFF;
    // // 编码扩展长度
    // buf[2] = extension_length_ >> 8;
    // buf[3] = extension_length_ & 0xFF;
    // buf += 4;

    // 编码扩展内容
    for (const auto& ext : extensions_) {
        // 编码扩展类型
        // buf[0] = ext->type >> 8;
        // buf[1] = ext->type & 0xFF;
        // // 编码扩展长度
        // buf[2] = ext.length >> 8;
        // buf[3] = ext.length & 0xFF;
        // buf += 4;

        // 编码扩展内容
        // if (!ext.contents.empty()) {
            auto extBody = ext->encode();
            memcpy(buf, extBody->data(), extBody->size());
            buf += extBody->size();
        // }
    }

    return buffer;
}

void HandshakePacket::assignPeerIP(struct sockaddr_storage *addr)
{
    // uint16_t peer_ip_addr[16];
    memset(peer_ip_address_, 0, sizeof(peer_ip_address_));
    if (addr->ss_family == AF_INET) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)addr;
        // 抓包 奇怪好像是小头端？？？  [AUTO-TRANSLATED:40eb164c]
        // Packet capture, weird, seems to be from the client side？？？
        writeUint32LE((char*)peer_ip_address_, ipv4->sin_addr.s_addr);
    } else if (addr->ss_family == AF_INET6) {
        if (IN6_IS_ADDR_V4MAPPED(&((struct sockaddr_in6 *)addr)->sin6_addr)) {
            struct in_addr addr4;
            memcpy(&addr4, 12 + (char *)&(((struct sockaddr_in6 *)addr)->sin6_addr), 4);
            writeUint32LE((char*)peer_ip_address_, addr4.s_addr);
        } else {
            const sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)addr;
            memcpy(peer_ip_address_, ipv6->sin6_addr.s6_addr, sizeof(peer_ip_address_));
        }
    }

    // peer_ip_address_ = (char*)peer_ip_addr;
}

bool HandshakePacket::isHandshakePacket(uint8_t *buf, size_t len)
{
    if (!ControlPacket::isControlPacket(buf, len)) {
        return false;
    }
    if (len < HEADER_SIZE + 48) {
        return false;
    }
    return ControlPacket::getControlType(buf, len) == HANDSHAKE;
}

uint32_t HandshakePacket::getHandshakeType(uint8_t *buf, size_t len)
{
    uint8_t *ptr = buf + HEADER_SIZE + 5 * 4;
    return readUint32BE((char*)ptr);
}

uint32_t HandshakePacket::getSynCookie(uint8_t *buf, size_t len)
{
    uint8_t *ptr = buf + HEADER_SIZE + 7 * 4;
    return readUint32BE((char*)ptr);
}

uint32_t HandshakePacket::generateSynCookie(struct sockaddr_storage *addr, 
        uint64_t ts, uint32_t current_cookie, int correction) {
    static std::atomic<uint32_t> distractor { 0 };
    uint32_t rollover = distractor.load() + 10;
    std::string ip;
    int port;
    tie(ip, port) = Socket::getIpAndPort(addr);

    while (true) {
        // SYN cookie
        int64_t timestamp = (TimeClock::nowMicro() - ts + 60 * 1e6) + distractor.load()
            + correction; // secret changes every one minute
        std::stringstream cookiestr;
        cookiestr << ip << ":" << port
                  << ":" << timestamp;
        union {
            unsigned char cookie[16];
            uint32_t cookie_val;
        };
        MD5 md5(cookiestr.str());
        memcpy(cookie, md5.rawdigest().c_str(), 16);

        if (cookie_val != current_cookie) {
            return cookie_val;
        }

        ++distractor;

        // This is just to make the loop formally breakable,
        // but this is virtually impossible to happen.
        if (distractor == rollover) {
            return cookie_val;
        }
    }
}

bool NakPacket::parse(uint8_t *buf, size_t len) {
    // 先调用基类的 parse 函数解析 ControlPacket 部分
    if (!ControlPacket::parse(buf, len)) {
        return false;
    }

    // 跳过 ControlPacket 头部的 16 字节
    buf += 16;
    len -= 16;

    lossList_.clear();
    while (len >= 4) {
        LossInfo info;
        info.isRange = (buf[0] & 0x80) != 0;
        info.seqNumber = ((buf[0] & 0x7F) << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
        if (info.isRange && len >= 8) {
            info.endSeqNumber = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
            buf += 8;
            len -= 8;
        } else {
            buf += 4;
            len -= 4;
        }
        lossList_.push_back(std::make_pair(info.seqNumber, info.endSeqNumber));
    }
    return true;
}

StreamBuffer::Ptr NakPacket::encode() {
    // 计算 Loss List 的长度
    size_t lossListSize = getCIFSize(lossList_);

    // 计算总长度
    size_t totalSize = 16 + lossListSize;
    auto buffer = std::make_shared<StreamBuffer>(totalSize + 1);
    uint8_t *buf = (uint8_t*)buffer->data();

    // 先调用基类的 encode 函数编码 ControlPacket 部分
    auto baseBuffer = ControlPacket::encode();
    memcpy(buf, baseBuffer->data(), 16);
    buf += 16;

    // 编码 Loss List
    for (const auto& info : lossList_) {
        if (info.first + 1 == info.second) {
            buf[0] = 0x80 | ((info.first >> 24) & 0x7F);
            buf[1] = (info.first >> 16) & 0xFF;
            buf[2] = (info.first >> 8) & 0xFF;
            buf[3] = info.first & 0xFF;
            buf[4] = (info.second >> 24) & 0xFF;
            buf[5] = (info.second >> 16) & 0xFF;
            buf[6] = (info.second >> 8) & 0xFF;
            buf[7] = info.second & 0xFF;
            buf += 8;
        } else {
            buf[0] = info.first >> 24 & 0x7F;
            buf[1] = (info.first >> 16) & 0xFF;
            buf[2] = (info.first >> 8) & 0xFF;
            buf[3] = info.first & 0xFF;
            buf += 4;
        }
    }

    return buffer;
}

size_t NakPacket::getCIFSize(std::list<std::pair<uint32_t, uint32_t>> &lost) {
    size_t size = 0;
    for (auto it : lost) {
        if (it.first + 1 == it.second) {
            size += 4;
        } else {
            size += 8;
        }
    }
    return size;
}

bool MsgDropReqPacket::parse(uint8_t *buf, size_t len) {
    // 先调用基类的 parse 函数解析 ControlPacket 部分
    if (!ControlPacket::parse(buf, len)) {
        return false;
    }

    // 检查长度是否足够解析后续字段
    if (len < 24) {
        return false;
    }

    // 跳过 ControlPacket 头部的 16 字节
    buf += 16;

    // 解析消息编号
    messageNumber_ = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
    // 解析首个数据包序列号
    firstPacketSeqNumber_ = (buf[4] << 24) | (buf[5] << 16) | (buf[6] << 8) | buf[7];
    // 解析最后一个数据包序列号
    lastPacketSeqNumber_ = (buf[8] << 24) | (buf[9] << 16) | (buf[10] << 8) | buf[11];

    return true;
}

StreamBuffer::Ptr MsgDropReqPacket::encode() {
    // 计算总长度
    size_t totalSize = 16 + 12;
    auto buffer = std::make_shared<StreamBuffer>(totalSize + 1);
    uint8_t *buf = (uint8_t*)buffer->data();

    // 先调用基类的 encode 函数编码 ControlPacket 部分
    auto baseBuffer = ControlPacket::encode();
    memcpy(buf, baseBuffer->data(), 16);
    buf += 16;

    // 编码消息编号
    buf[0] = messageNumber_ >> 24;
    buf[1] = (messageNumber_ >> 16) & 0xFF;
    buf[2] = (messageNumber_ >> 8) & 0xFF;
    buf[3] = messageNumber_ & 0xFF;
    // 编码首个数据包序列号
    buf[4] = firstPacketSeqNumber_ >> 24;
    buf[5] = (firstPacketSeqNumber_ >> 16) & 0xFF;
    buf[6] = (firstPacketSeqNumber_ >> 8) & 0xFF;
    buf[7] = firstPacketSeqNumber_ & 0xFF;
    // 编码最后一个数据包序列号
    buf[8] = lastPacketSeqNumber_ >> 24;
    buf[9] = (lastPacketSeqNumber_ >> 16) & 0xFF;
    buf[10] = (lastPacketSeqNumber_ >> 8) & 0xFF;
    buf[11] = lastPacketSeqNumber_ & 0xFF;

    return buffer;
}
bool AckPacket::parse(uint8_t *buf, size_t len) {
    if (len < 16 + 4) {
        logWarn << "AckPacket::parse len < 16 + 4";
        return false;
    }

    // 先调用基类的 parse 函数解析 ControlPacket 部分
    if (!ControlPacket::parse(buf, len)) {
        logWarn << "AckPacket::parse ControlPacket::parse failed";
        return false;
    }

    buf += 16;

    uint32_t ackNo = getTypeSpecificInfo();
    ack_number_ = readUint32BE((char*)&ackNo);

    last_ack_packet_seq_number_ = readUint32BE((char*)buf);
    buf += 4;
    if (len == 20) {
        // light ack
        return true;
    }

    rtt_ = readUint32BE((char*)buf);
    buf += 4;
    rtt_variance_ = readUint32BE((char*)buf);
    buf += 4;
    available_buffer_size_ = readUint32BE((char*)buf);
    buf += 4;
    if (len == 32) {
        // small ack
        return true;
    }
    packets_receiving_rate_ = readUint32BE((char*)buf);
    buf += 4;
    estimated_link_capacity_ = readUint32BE((char*)buf);
    buf += 4;
    receiving_rate_ = readUint32BE((char*)buf);
    buf += 4;

    return true;
}

StreamBuffer::Ptr AckPacket::encode() {
    // 计算总长度
    size_t totalSize = 16 + 28;
    auto buffer = std::make_shared<StreamBuffer>(totalSize + 1);
    uint8_t *buf = (uint8_t*)buffer->data();

    setControlType(ACK);
    setSubtype(0);
    uint32_t ackNo;
    writeUint32BE((char*)&ackNo, ack_number_);
    setTypeSpecificInfo(ackNo);

    // 先调用基类的 encode 函数编码 ControlPacket 部分
    auto baseBuffer = ControlPacket::encode();
    memcpy(buf, baseBuffer->data(), 16);
    buf += 16;

    // 编码消息编号
    writeUint32BE((char*)buf, last_ack_packet_seq_number_);
    buf += 4;
    // 编码 ACK rtt
    writeUint32BE((char*)buf, rtt_);
    buf += 4;
    // 编码 ACK rtt 方差
    writeUint32BE((char*)buf, rtt_variance_);
    buf += 4;
    // 编码 ACK 可用缓冲大小
    writeUint32BE((char*)buf, available_buffer_size_);
    buf += 4;
    // 编码 ACK 接收包速率
    writeUint32BE((char*)buf, packets_receiving_rate_);
    buf += 4;
    // 编码 ACK 估计链路容量
    writeUint32BE((char*)buf, estimated_link_capacity_);
    buf += 4;
    // 编码 ACK 接收包速率方差
    writeUint32BE((char*)buf, receiving_rate_);
    buf += 4;

    return buffer;
}

bool AckAckPacket::parse(uint8_t *buf, size_t len) 
{
    // 直接调用基类的解析函数
    bool ret = ControlPacket::parse(buf, len);
    ack_number_ = ntohl(getTypeSpecificInfo());

    return ret;
}

StreamBuffer::Ptr AckAckPacket::encode() 
{
    // 直接调用基类的编码函数
    size_t totalSize = 16 + 4;
    auto buffer = std::make_shared<StreamBuffer>(totalSize + 1);
    uint8_t *buf = (uint8_t*)buffer->data();

    setControlType(ACKACK);
    setSubtype(0);
    setTypeSpecificInfo(htonl(ack_number_));

    // 先调用基类的 encode 函数编码 ControlPacket 部分
    auto baseBuffer = ControlPacket::encode();
    memcpy(buf, baseBuffer->data(), 16);
    buf += 16;

    // 编码unused，wireshark抓包，发现libsrt多了四个字节的unused字段为0x00000000
    writeUint32BE((char*)buf, 0);
    buf += 4;
    
    return buffer;
}