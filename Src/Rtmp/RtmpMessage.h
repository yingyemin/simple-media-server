#ifndef RtmpMessage_H
#define RtmpMessage_H

#include <cstdint>
#include <memory>

#include "Rtmp.h"

using namespace std;

#pragma pack(1)

	// chunk header: basic_header + rtmp_message_header 
struct RtmpMessageHeader
{
    uint8_t timestamp[3];
    uint8_t length[3];
    uint8_t type_id;
    uint8_t stream_id[4];
};

class RtmpMessage
{
public:
    using Ptr = shared_ptr<RtmpMessage>;
    void clear()
    {
        index = 0;
        timestamp = 0;
        extend_timestamp = 0;
        if (length > 0 || payload) {
            length = 0;
            payload = nullptr;
        }
    }

    bool isCompleted() const
    {
        if (length > 0 && index == length && payload) {
            return true;
        }

        return false;
    }

    bool isKeyFrame() const
    {
        bool isEnhance = (payload.get()[0] >> 4) & 0b1000;
        uint8_t frame_type;
        uint8_t packet_type;

        if (isEnhance) {
            frame_type = (payload.get()[0] >> 4) & 0b0111;
            packet_type = payload.get()[0] & 0x0f;
            return type_id == RTMP_VIDEO && frame_type == 1 && (packet_type == 1 || packet_type == 3);
        } else {
            frame_type = (payload.get()[0] >> 4) & 0x0f;
            packet_type = payload.get()[1];
            return type_id == RTMP_VIDEO && frame_type == 1 && packet_type == 1;
        }
    }

public:
    int trackIndex_;

    uint8_t  type_id = 0;
    uint8_t  codecId = 0;
    uint8_t  csid = 0;

    uint32_t index = 0;
    uint32_t timestamp = 0;
    uint32_t length = 0;
    uint32_t stream_id = 0;
    uint32_t extend_timestamp = 0;

    uint64_t abs_timestamp = 0;
    uint64_t laststep = 0;

    std::shared_ptr<char> payload = nullptr;
};

#pragma pack()
#endif