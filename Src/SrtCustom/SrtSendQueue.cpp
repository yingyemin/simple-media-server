#include "SrtSendQueue.h"
#include "SrtControlPacket.h"
#include "Log/Logger.h"

/**
 * @brief 构造函数，初始化 SrtSendQueue 实例
 * 
 * @param max_size 数据包缓存队列的最大容量
 * @param latency 数据包允许的最大延迟
 * @param flag SRT 相关标志位，用于控制特定功能
 */
SrtSendQueue::SrtSendQueue(uint32_t max_size, uint32_t latency,uint32_t flag)
    : _srt_flag(flag)  // 初始化 SRT 标志位
    , _pkt_cap(max_size)  // 初始化数据包缓存队列的最大容量
    , _pkt_latency(latency) {
    _pkt_cache.resize(max_size);  // 调整 vector 大小
}

/**
 * @brief 计算环形缓冲区的索引
 * 
 * @param offset 偏移量
 * @return uint32_t 计算后的索引
 */
uint32_t SrtSendQueue::getIndex(uint32_t offset) const {
    return (_head + offset) % _pkt_cap;
}

/**
 * @brief 从数据包缓存队列中丢弃指定序列号之前的所有数据包
 * 
 * 该方法会遍历数据包缓存队列，找到第一个序列号等于指定序列号的数据包，
 * 然后删除该数据包之前的所有数据包。
 * 
 * @param num 指定的数据包序列号
 * @return bool 总是返回 true，表示操作成功
 */
bool SrtSendQueue::drop(uint32_t num) {
    if (_size == 0) {
        logDebug << "drop num: " << num << " _size: " << _size;
        return false;
    }

    auto startNum = _pkt_cache[getIndex(0)]->getPacketSeqNumber();
    auto endNum = _pkt_cache[getIndex(_size - 1)]->getPacketSeqNumber();
    logInfo << "drop num: " << num << " _size: " << _size << " startNum: " << startNum << " endNum: " << endNum;
    if (endNum > startNum) {
        if (num > endNum || num < startNum) {
            return false;
        }
    } else {
        if (num > endNum && num < startNum) {
            return false;
        }
    }

    for (uint32_t i = 0; i < _size; ++i) {
        auto index = getIndex(i);
        if (_pkt_cache[index] && _pkt_cache[index]->getPacketSeqNumber() == num) {
            _head = index;
            _size -= i;
            break ;
        }
    }

    logInfo << " _size: " << _size;
    
    return true;
}

/**
 * @brief 向数据包缓存队列中添加一个新的数据包
 * 
 * 该方法会将新的数据包添加到队列尾部，然后检查队列大小和时间延迟，
 * 如果队列大小超过最大容量或时间延迟超过允许的最大延迟，且满足 TLPKT 丢弃条件，
 * 则从队列头部删除数据包。
 * 
 * @param pkt 要添加的数据包智能指针
 * @return bool 总是返回 true，表示操作成功
 */
bool SrtSendQueue::inputPacket(DataPacket::Ptr pkt) {
    if (_size < _pkt_cap) {
        // 缓冲区未满，直接添加
        auto index = getIndex(_size);
        _pkt_cache[index] = pkt;
        ++_size;
    } else {
        // 缓冲区已满，覆盖头部元素
        _pkt_cache[_head] = pkt;
        _head = (_head + 1) % _pkt_cap;
    }

    // 检查队列中数据包的时间延迟是否超过允许的最大延迟，且满足 TLPKT 丢弃条件
    while (_size > 0 && timeLatency() > _pkt_latency && TLPKTDrop()) {
        _head = (_head + 1) % _pkt_cap;
        --_size;
    }
    return true;
}

/**
 * @brief 判断是否满足 TLPKT 丢弃条件
 * 
 * 当 SRT 标志位同时包含 HS_EXT_MSG_TLPKTDROP 和 HS_EXT_MSG_TSBPDSND 时，认为满足 TLPKT 丢弃条件。
 * 
 * @return bool 满足条件返回 true，否则返回 false
 */
bool SrtSendQueue::TLPKTDrop(){
    return (_srt_flag & HS_EXT_MSG_TLPKTDROP) && (_srt_flag & HS_EXT_MSG_TSBPDSND);
}

/**
 * @brief 根据序列号范围查找数据包
 * 
 * 该方法会在数据包缓存队列中查找序列号在指定范围内的数据包，并将这些数据包添加到一个列表中返回。
 * 
 * @param start 起始序列号
 * @param end 结束序列号
 * @return std::list<DataPacket::Ptr> 包含符合条件数据包的列表
 */
std::list<DataPacket::Ptr> SrtSendQueue::findPacketBySeq(uint32_t start, uint32_t end) {
    std::list<DataPacket::Ptr> re;
    for (uint32_t i = 0; i < _size; ++i) {
        auto index = getIndex(i);
        auto pkt = _pkt_cache[index];
        if (pkt && pkt->getPacketSeqNumber() >= start) {
            re.push_back(pkt);
            if (pkt->getPacketSeqNumber() == end) {
                break;
            }
        }
    }
    return re;
}

/**
 * @brief 计算数据包缓存队列中最早和最晚数据包之间的时间延迟
 * 
 * 该方法会获取数据包缓存队列 _pkt_cache 中最早和最晚数据包的时间戳，
 * 计算它们之间的差值作为时间延迟。同时会处理时间戳回绕的情况。
 * 
 * @return uint32_t 计算得到的时间延迟，单位与数据包时间戳单位一致
 */
uint32_t SrtSendQueue::timeLatency() {
    if (_size == 0) {
        return 0;
    }
    auto first = _pkt_cache[getIndex(0)]->getTimestamp();
    auto last = _pkt_cache[getIndex(_size - 1)]->getTimestamp();
    uint32_t dur;

    if (last > first) {
        dur = last - first;
    } else {
        dur = first - last;
    }

    if (dur > ((uint32_t)0x01 << 31)) {
        // 输出发生时间戳回绕的日志信息
        logTrace << "cycle timeLatency " << dur;
        dur = 0xffffffff - dur;
    }

    return dur;
}
