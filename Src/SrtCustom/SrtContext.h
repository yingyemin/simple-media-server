#ifndef SrtContext_H
#define SrtContext_H

#include "Buffer.h"
#include "SrtDataPacket.h"
#include "SrtControlPacket.h"
// #include "SrtSort.h"
// #include "SrtMediaSource.h"
#include "Util/TimeClock.h"
#include "EventPoller/EventLoop.h"
#include "Net/Socket.h"
#include "Mpeg/TsMediaSource.h"
#include "SrtSendQueue.h"
#include "SrtRecvQueue.h"
#include "SrtUtil.h"

#include <memory>
#include <unordered_map>
#include <list>

using namespace std;

class SrtContext : public enable_shared_from_this<SrtContext>
{
public:
    using Ptr = shared_ptr<SrtContext>;
    using Wptr = weak_ptr<SrtContext>;

    SrtContext(const EventLoop::Ptr& loop, const string& uri, const string& vhost, const string& protocol, const string& type);
    ~SrtContext();
public:
    bool init();
    void onSrtPacket(const Socket::Ptr& socket, const Buffer::Ptr& pkt, struct sockaddr* addr = nullptr, int len = 0, bool sort = false);
    void heartbeat();
    bool isAlive() {return _alive;}

    void setPayloadType(const string& payloadType);
    void createVideoTrack(const string& videoCodec);
    void createAudioTrack(const string& audioCodec, int channel, int sampleBit, int sampleRate);

private:
    void handleHandshake(const Buffer::Ptr& pkt);
    void handleKeepAlive(const Buffer::Ptr& pkt);
    void handleAck(const Buffer::Ptr& pkt);
    void handleNak(const Buffer::Ptr& pkt);
    void handleCongestionWarning(const Buffer::Ptr& pkt);
    void handleShutdown(const Buffer::Ptr& pkt);
    void handleAckAck(const Buffer::Ptr& pkt);
    void handleDropReq(const Buffer::Ptr& pkt);
    void handlePeerError(const Buffer::Ptr& pkt);
    void handleUserDefinedType(const Buffer::Ptr& pkt);

    void handleDataPacket(const Buffer::Ptr& pkt);
    void checkAndSendAckNak();
    void handleHandshakeConclusion(const HandshakePacket& hsPacket);
    void handleHandshakeInduction(const HandshakePacket& hsPacket);
    void sendControlPacket(const Buffer::Ptr& pkt);
    void sendDataPacket(DataPacket::Ptr pkt, char *buf, int len, bool flush);
    void onError(const string& msg);
    void handlePull();
    void onPlayTs(const TsMediaSource::Ptr &tsSrc);
    void sendTSData(const Buffer::Ptr &buffer, bool flush);
    void initPush();
    void inputSrtData(const DataPacket::Ptr &pkt);
    void sendMsgDropReq(uint32_t first, uint32_t last);
    void sendAckPacket();
    void sendLightAckPacket();
    void sendNakPacket(std::list<SrtRecvQueue::LostPair> &lost_list);
    void sendShutDown();

private:
    bool _alive = true;
    bool _isPlaying = false;
    bool _handshakeFinish = false;
    bool _inited = false;

    uint32_t _startSeq = 0;
    uint32_t _lastPktSeq = 0;
    uint32_t _lastAckPktSeq = 0;
    uint32_t _maxWindowSize = 8192;
    uint32_t _mtu = 1500;
    uint32_t _localSocketId = 0;
    uint32_t _peerSocketId = 0;
    uint32_t _syncCookie = 0;
    uint32_t _sendPktSeq = 0;
    uint32_t _sendMsgNumber = 1;
    uint32_t _rtt = 100 * 1000;
    uint32_t _rttVariance = 50 * 1000;
    uint32_t _lastRecvAckackSeqNum = 0;
    uint32_t _lightAckPktCount = 0;
    uint32_t _ackNumberCount = 0;
    uint32_t _rexmitCount = 0;
    uint32_t _rto = 0;

    uint64_t _inductionStamp;
    uint64_t _startStamp;
    uint64_t _now;
    uint64_t _sendBytes_1s = 0;
    uint64_t _avgPayloadSize = 0;
    uint64_t _pktSendPeriod = 0;

    float _sendRate = 0;
    float _maxBandwidth = 0;

    string _uri;
    string _vhost;
    string _protocol;
    string _type;
    string _payloadType;
    string _request;

    TimeClock _timeClock;
    TimeClock _nakClock;
    TimeClock _ackClock;
    TimeClock _sendRateClock;
    UrlParser _urlParser;
    shared_ptr<TimerTask> _hsInductionTask;
    shared_ptr<TimerTask> _hsConclusionTask;
    shared_ptr<sockaddr> _addr;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    TsMediaSource::Wptr _source;
    TsMediaSource::RingType::DataQueReaderT::Ptr _playTsReader;
    Buffer::Ptr _hsInductionRes;
    Buffer::Ptr _hsConclusionRes;
    SrtSendQueue::Ptr _sendQueue;
    SrtRecvQueue::Ptr _recvQueue;
    std::shared_ptr<PacketRecvRateContext> _pktRecvRateContext;
    std::shared_ptr<EstimatedLinkCapacityContext> _estimatedLinkCapacityContext;

    unordered_map<string, string> _mapParam;
    std::map<uint32_t, uint64_t> _ackSendTimestamp;
    // SrtSort::Ptr _sort;
    // SrtDecodeTrack::Ptr _videoTrack;
    // SrtDecodeTrack::Ptr _audioTrack;
    // SrtMediaSource::Wptr _source;
};



#endif //SrtContext_H
