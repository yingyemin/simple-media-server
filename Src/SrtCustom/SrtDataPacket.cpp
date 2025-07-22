#include "SrtDataPacket.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

bool DataPacket::parse(uint8_t *buf, size_t len) {
    if (len < 16) { // 头部至少 16 字节
        return false;
    }

    length_ = len;

    // 解析 Packet Sequence Number
    f_ = (buf[0] >> 7) & 0x01;
    packet_seq_number_ = ((buf[0] & 0x7F) << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];

    // 解析 Message Number 相关字段
    pp_ = (buf[4] >> 6) & 0x03;
    o_ = (buf[4] >> 5) & 0x01;
    kk_ = (buf[4] >> 3) & 0x03;
    r_ = (buf[4] >> 2) & 0x01;
    msg_number_ = ((buf[4] & 0x03) << 30) | (buf[5] << 22) | (buf[6] << 14) | (buf[7] << 6) | (buf[8] >> 2);

    // 解析 Timestamp
    timestamp_ = (buf[8] & 0x03) << 30 | (buf[9] << 22) | (buf[10] << 14) | (buf[11] << 6) | (buf[12] >> 2);

    // 解析 Destination Socket ID
    dst_socket_id_ = (buf[12] & 0x03) << 30 | (buf[13] << 22) | (buf[14] << 14) | (buf[15] << 6) | (buf[16] >> 2);

    // 解析数据部分
    if (len > 16) {
        data_ = std::make_shared<StreamBuffer>((char*)buf + 16, len - 16);
    }

    return true;
}

StreamBuffer::Ptr DataPacket::encode(const uint8_t *data, size_t len) {
    // size_t dataSize = data_ ? data_->size() : 0;
    size_t totalSize = 16 + len;
    auto buffer = std::make_shared<StreamBuffer>(totalSize + 1);
    uint8_t *buf = (uint8_t*)buffer->data();

    // 编码 Packet Sequence Number
    buf[0] = (f_ << 7) | ((packet_seq_number_ >> 24) & 0x7F);
    buf[1] = (packet_seq_number_ >> 16) & 0xFF;
    buf[2] = (packet_seq_number_ >> 8) & 0xFF;
    buf[3] = packet_seq_number_ & 0xFF;

    // 编码 Message Number 相关字段
    buf[4] = (pp_ << 6) | (o_ << 5) | (kk_ << 3) | (r_ << 2) | ((msg_number_ >> 24) & 0x03);
    buf[5] = (msg_number_ >> 16) & 0xFF;
    buf[6] = (msg_number_ >> 8) & 0xFF;
    buf[7] = msg_number_ & 0xFF;

    // 编码 Timestamp
    writeUint32BE((char*)buf + 8, timestamp_);

    // 编码 Destination Socket ID
    writeUint32BE((char*)buf + 12, dst_socket_id_);

    // 编码数据部分
    if (len > 0) {
        memcpy(buf + 16, data, len);
    }

    length_ = totalSize;

    return buffer;
}


bool DataPacket::isDataPacket(uint8_t *buf, size_t len) {
    if (len < 16) {
        logInfo << "data size" << len << " less " << 16;
        return false;
    }
    if (buf[0] & 0x80) {
        return false;
    }
    return true;
}

uint32_t DataPacket::getDstSocketID(uint8_t *buf, size_t len) {
    uint8_t *ptr = buf;
    ptr += 12;
    return readUint32BE((char*)ptr);
}