#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "RtpSort.h"
#include "Logger.h"
#include "Util/String.h"

using namespace std;

RtpSort::RtpSort(int maxQueSize)
    :_maxQueSize(maxQueSize)
{}

void RtpSort::inputRtp(const RtpPacket::Ptr& rtp)
{
    if (_firstRtp) {
        _firstRtp = false;
        onRtpPacket(rtp);
        _lastRtpSeq = rtp->getSeq();
        return ;
    }

    uint16_t curSeq = _lastRtpSeq + 1;
    if (rtp->getSeq() != curSeq) {
        logInfo << "loss rtp, last seq + 1: " << curSeq << ", cur seq: " << rtp->getSeq();
        _mapRtp[rtp->getSeq()] = rtp;
    } else {
        onRtpPacket(rtp);
        _lastRtpSeq = rtp->getSeq();
    }

    while (_mapRtp.size() >= _maxQueSize || 
        (_mapRtp.size() > 0 && _mapRtp.begin()->second->getSeq() == (uint16_t)(_lastRtpSeq + 1))) {
        auto it = _mapRtp.begin();
        _lastRtpSeq = it->second->getSeq();
        onRtpPacket(it->second);
        _mapRtp.erase(it);
    }
}

void RtpSort::onRtpPacket(const RtpPacket::Ptr& rtp)
{
    if (_onRtpPacket) {
        _onRtpPacket(rtp);
    }
}

void RtpSort::setOnRtpPacket(const function<void(const RtpPacket::Ptr& rtp)>& cb)
{
    _onRtpPacket = cb;
}

vector<uint16_t> RtpSort::getLossSeq()
{
    int curSeq = -1;
    vector<uint16_t> vecSeq;
    for (auto& iter : _mapRtp) {
        if (curSeq == -1) {
            curSeq = iter.first;
            continue;
        }
        uint16_t diff = iter.first - (uint16_t)curSeq;
        for (int i = 1; i < diff; ++i) {
            vecSeq.push_back(curSeq + i);
        }
    }

    return vecSeq;
}