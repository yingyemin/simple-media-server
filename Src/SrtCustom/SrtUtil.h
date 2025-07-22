#ifndef SRT_UTIL_H
#define SRT_UTIL_H
#include <map>
#include <chrono>

#include "SrtDataPacket.h"

#define SEQ_NONE 0xffffffff

class PacketRecvRateContext {
public:
    PacketRecvRateContext(uint64_t start);
    ~PacketRecvRateContext() = default;
    void inputPacket(uint64_t &ts,size_t len = 0);
    uint32_t getPacketRecvRate(uint32_t& bytesps);
    std::string dump();
    static const int SIZE = 16;
private:
    uint64_t _last_arrive_time;
    int64_t _ts_arr[SIZE];
    size_t _size_arr[SIZE];
    size_t _cur_idx;
    //std::map<int64_t, int64_t> _pkt_map;
};

class EstimatedLinkCapacityContext {
public:
    EstimatedLinkCapacityContext(uint64_t start);
    ~EstimatedLinkCapacityContext() = default;
    void setLastSeq(uint32_t seq){
        _last_seq = seq;
    }
    void inputPacket(uint64_t &ts,DataPacket::Ptr& pkt);
    uint32_t getEstimatedLinkCapacity();
    static const int SIZE = 64;
private:
    void probe1Arrival(uint64_t &ts,const DataPacket::Ptr& pkt, bool unordered);
    void probe2Arrival(uint64_t &ts,const DataPacket::Ptr& pkt);
private:
    uint64_t _start;
    uint64_t _ts_probe_time;
    int64_t _dur_probe_arr[SIZE];
    size_t _cur_idx;
    uint32_t _last_seq = 0;
    uint32_t _probe1_seq = SEQ_NONE;
    //std::map<int64_t, int64_t> _pkt_map;
};

/*
class RecvRateContext {
public:
    RecvRateContext(TimePoint start)
        : _start(start) {};
    ~RecvRateContext() = default;
    void inputPacket(TimePoint &ts, size_t size);
    uint32_t getRecvRate();

private:
    TimePoint _start;
    std::map<int64_t, size_t> _pkt_map;
};
*/

#endif // SRT_UTIL_H