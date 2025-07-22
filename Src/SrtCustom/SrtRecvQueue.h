#ifndef SRT_RECV_QUEUE_H
#define SRT_RECV_QUEUE_H
#include "SrtDataPacket.h"
#include <algorithm>
#include <list>
#include <map>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>

// 自定义比较函数，处理序列号回环
struct SeqCompare {
    bool operator()(const uint32_t& a, const uint32_t& b) const {
        // 假设 MAX_SEQ 是 0xFFFFFFFF
        constexpr uint32_t HALF_MAX_SEQ = 0x80000000;
        return (a < b) && ((b - a) < HALF_MAX_SEQ) || (a > b) && ((a - b) > HALF_MAX_SEQ);
    }
};

// for recv
class SrtRecvQueue
{
public:
    using Ptr = std::shared_ptr<SrtRecvQueue>;
    using LostPair = std::pair<uint32_t, uint32_t>;

    SrtRecvQueue(uint32_t max_size, uint32_t init_seq, uint32_t latency);
    ~SrtRecvQueue() = default;
    bool inputPacket(DataPacket::Ptr pkt, std::list<DataPacket::Ptr> &out);

    uint32_t timeLatency();
    std::list<LostPair> getLostSeq();

    size_t getSize();
    size_t getExpectedSize();
    size_t getAvailableBufferSize();
    uint32_t getExpectedSeq();

    std::string dump();
    bool drop(uint32_t first, uint32_t last, std::list<DataPacket::Ptr> &out);

private:
    void tryInsertPkt(DataPacket::Ptr pkt);

private:
    uint32_t _pkt_cap;
    uint32_t _pkt_latency;
    uint32_t _pkt_expected_seq;
    // 使用自定义比较函数
    std::map<uint32_t, DataPacket::Ptr, SeqCompare> _pkt_map;
};

#endif // SRT_RECV_QUEUE_H