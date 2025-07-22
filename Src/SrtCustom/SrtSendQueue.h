#ifndef SRT_SEND_QUEUE_H
#define SRT_SEND_QUEUE_H

#include "SrtDataPacket.h"
#include <algorithm>
#include <vector>
#include <memory>
#include <utility>
#include <list>

class SrtSendQueue {
public:
    using Ptr = std::shared_ptr<SrtSendQueue>;
    using LostPair = std::pair<uint32_t, uint32_t>;

    SrtSendQueue(uint32_t max_size, uint32_t latency,uint32_t flag = 0xbf);
    ~SrtSendQueue() = default;

    bool drop(uint32_t num);
    bool inputPacket(DataPacket::Ptr pkt);
    std::list<DataPacket::Ptr> findPacketBySeq(uint32_t start, uint32_t end);

private:
    uint32_t timeLatency();
    bool TLPKTDrop();
    // 计算环形缓冲区的索引
    uint32_t getIndex(uint32_t offset) const;
private:
    uint32_t _srt_flag;
    uint32_t _pkt_cap;
    uint32_t _pkt_latency;
    std::vector<DataPacket::Ptr> _pkt_cache;
    uint32_t _head = 0;  // 环形缓冲区的头索引
    uint32_t _size = 0;  // 当前缓冲区中元素的数量
};

#endif // SRT_SEND_QUEUE_H