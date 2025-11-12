#include "SrtRecvQueue.h"
#include "Log/Logger.h"

#define MAX_SEQ  0x7fffffff
#define MAX_TS   0xffffffff

// 判断序列号是否接近回环边界
// seq: 要检查的序列号
// cap: 缓冲区容量
// 返回值: 如果序列号接近回环边界返回 true，否则返回 false
static inline bool isSeqEdge(uint32_t seq, uint32_t cap) {
    if (seq > (MAX_SEQ - cap)) {
        return true;
    }
    return false;
}

// SrtRecvQueue 构造函数
// max_size: 队列最大容量
// init_seq: 初始期望序列号
// latency: 延迟阈值
SrtRecvQueue::SrtRecvQueue(uint32_t max_size, uint32_t init_seq, uint32_t latency)
    : _pkt_cap(max_size)
    , _pkt_latency(latency)
    , _pkt_expected_seq(init_seq) {}

/**
 * @brief 尝试将数据包插入到队列中
 * 
 * 该函数会根据数据包的序列号与期望序列号的关系，判断是否将数据包插入队列。
 * 若序列号差距过大，可能会丢弃数据包。
 * 
 * @param pkt 要插入的数据包的智能指针
 */
void SrtRecvQueue::tryInsertPkt(DataPacket::Ptr pkt) {
    // 使用自定义比较函数后，简化插入逻辑
    if (SeqCompare{}(pkt->getPacketSeqNumber(), _pkt_expected_seq)) {
        // 数据包序列号小于期望序列号且差距过大，丢弃
        return;
    }
    _pkt_map.emplace(pkt->getPacketSeqNumber(), pkt);
}

// 输入数据包并处理
// pkt: 输入的数据包指针
// out: 按顺序输出的数据包列表
// 返回值: 总是返回 true
bool SrtRecvQueue::inputPacket(DataPacket::Ptr pkt, std::list<DataPacket::Ptr> &out) {
    tryInsertPkt(pkt);
    auto it = _pkt_map.find(_pkt_expected_seq);
    // 按顺序取出已到达的数据包
    while (it != _pkt_map.end()) {
        out.push_back(it->second);
        _pkt_map.erase(it);
        _pkt_expected_seq++;
        it = _pkt_map.find(_pkt_expected_seq);
    }

    if (_pkt_map.empty()) {
        return true;
    }

    auto endSeq = _pkt_map.end()->first;

    // 当队列大小超过容量时，尝试取出数据包
    while (!_pkt_map.empty()) {
        auto startIt = _pkt_map.begin();
        auto startSeq = startIt->first;
        
        if (endSeq - startSeq <= _pkt_cap) {
            break;
        }
        
        out.push_back(startIt->second);
        _pkt_map.erase(startIt);
    }

    // 当延迟超过阈值时，尝试取出数据包
    while (timeLatency() > _pkt_latency) {
        auto it = _pkt_map.begin();
        if (it != _pkt_map.end()) {
            out.push_back(it->second);
            _pkt_map.erase(it);
        }
    }

    return true;
}

// 丢弃指定范围内的数据包
// first: 起始序列号
// last: 结束序列号
// out: 被丢弃的数据包列表
// 返回值: 总是返回 true
bool SrtRecvQueue::drop(uint32_t first, uint32_t last, std::list<DataPacket::Ptr> &out) {
    for (uint32_t i = first; i <= last; ++i) {
        auto it = _pkt_map.find(i);
        if (it != _pkt_map.end()) {
            out.push_back(it->second);
            _pkt_map.erase(it);
        }
    }
    _pkt_expected_seq = last + 1;
    return true;
}

// 计算队列中数据包的延迟
// 返回值: 延迟时间（单位：毫秒）
uint32_t SrtRecvQueue::timeLatency() {
    if (_pkt_map.empty()) {
        return 0;
    }

    auto first = _pkt_map.begin()->second->getTimestamp();
    auto last = _pkt_map.rbegin()->second->getTimestamp();
    uint32_t dur = last > first ? last - first : first - last;

    if (dur > 0x80000000) {
        dur = MAX_TS - dur;
        logWarn << "cycle dur " << dur;
    }

    return dur;
}

// 获取丢失的序列号范围
// 返回值: 丢失序列号范围列表
std::list<SrtRecvQueue::LostPair> SrtRecvQueue::getLostSeq() {
    std::list<SrtRecvQueue::LostPair> re;
    if (_pkt_map.empty()) {
        return re;
    }

    if (getExpectedSize() == getSize()) {
        return re;
    }

    for (auto it = _pkt_map.begin(); it != _pkt_map.end(); ++it) {
        if (it->first != _pkt_expected_seq) {
            re.push_back({_pkt_expected_seq, it->first - 1});
            _pkt_expected_seq = it->first + 1;
        }
    }

    // uint32_t last = _pkt_map.rbegin()->first;
    // SrtRecvQueue::LostPair lost;
    // lost.first = 0;
    // lost.second = 0;

    // bool finish = true;
    // for (uint32_t i = _pkt_expected_seq; i <= last; ++i) {
    //     if (_pkt_map.find(i) == _pkt_map.end()) {
    //         if (finish) {
    //             finish = false;
    //             lost.first = i;
    //             lost.second = i + 1;
    //         } else {
    //             lost.second = i + 1;
    //         }
    //     } else {
    //         if (!finish) {
    //             finish = true;
    //             re.push_back(lost);
    //         }
    //     }
    // }

    return re;
}

// 获取队列中当前数据包的数量
// 返回值: 数据包数量
size_t SrtRecvQueue::getSize() {
    return _pkt_map.size();
}

// 获取队列中期望的数据包数量
// 返回值: 期望的数据包数量
size_t SrtRecvQueue::getExpectedSize() {
    if (_pkt_map.empty()) {
        return 0;
    }

    uint32_t max = _pkt_map.rbegin()->first;
    return max - _pkt_expected_seq + 1;
}

// 获取队列中可用的缓冲区大小
// 返回值: 可用缓冲区大小
size_t SrtRecvQueue::getAvailableBufferSize() {
    auto size = getExpectedSize();
    if (_pkt_cap > size) {
        return _pkt_cap - size;
    }

    if (_pkt_cap > _pkt_map.size()) {
        return _pkt_cap - _pkt_map.size();
    }
    logWarn << " cap " << _pkt_cap << " expected size " << size << " map size " << _pkt_map.size();
    return _pkt_cap;
}

// 获取当前期望的序列号
// 返回值: 期望的序列号
uint32_t SrtRecvQueue::getExpectedSeq() {
    return _pkt_expected_seq;
}

// 生成队列的状态信息字符串
// 返回值: 队列状态信息字符串
std::string SrtRecvQueue::dump() {
    std::stringstream ss;
    if (_pkt_map.empty()) {
        ss << " expected seq :" << _pkt_expected_seq;
    } else {
        ss << " expected seq :" << _pkt_expected_seq << " size:" << _pkt_map.size()
           << " first:" << _pkt_map.begin()->second->getPacketSeqNumber()
           << " last:" << _pkt_map.rbegin()->second->getPacketSeqNumber()
           << " latency:" << timeLatency() / 1e3;
    }
    return std::move(ss.str());
}