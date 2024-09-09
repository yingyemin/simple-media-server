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
        if (_setSeq.find(rtp->getSeq()) == _setSeq.end()) {
            // _mapRtp[rtp->getSeq()] = rtp;
            _mapRtp.emplace(rtp->getSeq(), rtp);
            _setSeq.emplace(rtp->getSeq());
            logInfo << "add seq: " << rtp->getSeq();
        }
    } else {
        onRtpPacket(rtp);
        _lastRtpSeq = rtp->getSeq();
    }

    // for (auto iter : _mapRtp) {
    //     logInfo << "iter seq is: " << iter.first << endl;
    // }

    // for (auto iter : _setSeq) {
    //     logInfo << "set iter seq is: " << iter << endl;
    // }

    while (true) {
        if (_mapRtp.empty()) {
            break;
        }
        auto iter = _mapRtp.begin();
        auto rtpSend = iter->second;
        if ((_mapRtp.size() >= _maxQueSize) || 
            (_mapRtp.size() > 0 && rtpSend->getSeq() == (uint16_t)(_lastRtpSeq + 1))) {
            // auto it = _mapRtp.begin();
            _lastRtpSeq = rtpSend->getSeq();
            auto seq = iter->first;
            onRtpPacket(rtpSend);
            _setSeq.erase(iter->first);
            _mapRtp.erase(iter);
        } else {
            break;
        }
    }
}

void RtpSort::onRtpPacket(const RtpPacket::Ptr& rtp)
{
    logInfo << "on rtp packet seq : " << rtp->getSeq();
    if (_onRtpPacket) {
        _onRtpPacket(rtp);
    }
}

void RtpSort::setOnRtpPacket(const function<void(const RtpPacket::Ptr& rtp)>& cb)
{
    _onRtpPacket = cb;
}

vector<uint16_t> RtpSort::
getLossSeq()
{
    vector<uint16_t> vecSeq;
    if (_firstRtp) {
        return vecSeq;
    }

    int curSeq = _lastRtpSeq;
    for (auto& iter : _mapRtp) {
        if (curSeq == -1) {
            curSeq = iter.first;
            continue;
        }
        logInfo << "iter.first: " << iter.first << ", curSeq: " << curSeq;
        uint16_t diff = iter.first - (uint16_t)curSeq;
        for (int i = 1; i < diff; ++i) {
            vecSeq.push_back(curSeq + i);
        }

        curSeq = iter.first;
    }

    return vecSeq;
}