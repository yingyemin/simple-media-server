#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <sys/socket.h>

#include "SrtContext.h"
#include "Logger.h"
#include "Util/String.h"
#include "Util/TimeClock.h"
#include "Common/Define.h"
#include "Common/Config.h"
#include "Common/HookManager.h"
#include "Common/UrlParser.h"
#include "SrtCustom/SrtManager.h"

using namespace std;

#define MAX_TS   0xffffffff

static std::atomic<uint32_t> s_srt_socket_id_generate { 125 };

static unordered_map<string, string> parseSid(const char *sid, int len)
{
    string strSid = UrlParser::urlDecode(string(sid, len));
    logTrace << "srt streamid: " << strSid;
    
    if (strSid.find("#!::") != string::npos) {
        strSid = strSid.substr(4);
        auto vecParam = split(strSid, ",", "=");
        vecParam["path"] = vecParam["r"];
        vecParam["request"] = vecParam["m"] == "play" ? "pull" : "push";

        return vecParam;
    }

    auto vecParam = split(strSid, "|", ":");

    return vecParam;
}

SrtContext::SrtContext(const EventLoop::Ptr& loop, const string& uri, const string& vhost, const string& protocol, const string& type)
    :_uri(uri)
    ,_vhost(vhost)
    ,_protocol(protocol)
    ,_type(type)
    ,_loop(loop)
{
    _startStamp = TimeClock::nowMicro();
    _localSocketId = s_srt_socket_id_generate.fetch_add(1);
    _pktRecvRateContext = std::make_shared<PacketRecvRateContext>(_startStamp);
    //_recv_rate_context = std::make_shared<RecvRateContext>(_start_timestamp);
    _estimatedLinkCapacityContext = std::make_shared<EstimatedLinkCapacityContext>(_startStamp);
}

SrtContext::~SrtContext()
{
    logInfo << "~SrtContext";
    // auto srtSrc = _source.lock();
    // if (srtSrc) {
    //     // gbSrc->delOnDetach(this);
    //     srtSrc->release();
    // }
}

bool SrtContext::init()
{
    weak_ptr<SrtContext> wSelf = shared_from_this();

    if (_mapParam.size() == 0) {
        _mapParam = parseSid(_uri.data(), _uri.size());

        if (_mapParam.find("path") == _mapParam.end()) {
            logError << "get path failed";
            onError("get path failed");
            return false;
        }

        logInfo << "path is: " << _mapParam["path"];

        _urlParser.path_ = _mapParam["path"];
        _urlParser.protocol_ = PROTOCOL_SRT;
        _urlParser.vhost_ = DEFAULT_VHOST;
        _urlParser.type_ = DEFAULT_TYPE;

        if (_mapParam.find("request") != _mapParam.end()) {
            _request = _mapParam["request"];
        }

        if (_mapParam.find("type") != _mapParam.end()) {
            _urlParser.type_ = _mapParam["type"];
        }

        if (_request == "pull") {
            handlePull();
        } else if (_request == "push") {
            initPush();
            // string name = "./" + _urlParser.path_ + ".ts";
            // _fp = fopen(name.c_str(), "ab+");
        } else {
            onError("invalid request: " + _request);
        }
    }

    return true;
}

void SrtContext::onSrtPacket(const Socket::Ptr& socket, const Buffer::Ptr& pkt, struct sockaddr* addr, int len, bool sort)
{
    if (!_loop) {
        // logInfo << "init loop";
        _loop = EventLoop::getCurrentLoop();
        // init();
    }

    // logInfo << "loop thread: " << _loop->getEpollFd();

    if (!_loop->isCurrent()) {
        weak_ptr<SrtContext> wSelf = shared_from_this();
        _loop->async([wSelf, pkt, addr, len, sort, socket](){
            auto self = wSelf.lock();
            if (self) {
                self->onSrtPacket(socket, pkt, addr, len, sort);
            }
        }, true);
        return ;
    }

    if (addr) {
        if (!_addr) {
            _addr = make_shared<sockaddr>();
            memcpy(_addr.get(), addr, sizeof(sockaddr));
            _socket = socket;
        } else if (memcmp(_addr.get(), addr, sizeof(struct sockaddr)) != 0) {
            // TODO 记录一下这个流，提供切换流的api
            logInfo << "收到其他地址的流";
            return ;
        }
    }

    if (_handshakeFinish && _hsConclusionTask) {
        _hsConclusionTask->quit = true;
        _hsConclusionTask = nullptr;
        init();
    }

    auto self = static_pointer_cast<SrtContext>(shared_from_this());
    static unordered_map<uint16_t, void (SrtContext::*)(const Buffer::Ptr& pkt)> srtHandle {
        {HANDSHAKE, &SrtContext::handleHandshake},
        {KEEPALIVE, &SrtContext::handleKeepAlive},
        {ACK, &SrtContext::handleAck},
        {NAK, &SrtContext::handleNak},
        {CONGESTIONWARNING, &SrtContext::handleCongestionWarning},
        {SHUTDOWN, &SrtContext::handleShutdown},
        {ACKACK, &SrtContext::handleAckAck},
        {DROPREQ, &SrtContext::handleDropReq},
        {PEERERROR, &SrtContext::handlePeerError},
        {USERDEFINEDTYPE, &SrtContext::handleUserDefinedType}
    };

    _now = TimeClock::nowMicro();

    if (DataPacket::isDataPacket((uint8_t*)pkt->data(), pkt->size())) {
        _pktRecvRateContext->inputPacket(_now, pkt->size() + 28/*UDP_HDR_SIZE*/);

        handleDataPacket(pkt);
        checkAndSendAckNak();
    } else if (ControlPacket::isControlPacket((uint8_t*)pkt->data(), pkt->size())) {
        auto type = ControlPacket::getControlType((uint8_t*)pkt->data(), pkt->size());
        logTrace << "get a control packet type: " << type;
        auto it = srtHandle.find(type);
        if (it != srtHandle.end()) {
            (this->*(it->second))(pkt);
            if (!_isPlaying && _handshakeFinish) {
                checkAndSendAckNak();
            }
        } else {
            logWarn << "unknown srt method: " << type;
        }
    } else {
        logWarn << "unknown srt packet";
    }

    _timeClock.update();
}

void SrtContext::heartbeat()
{
    // static int timeout = Config::instance()->getConfig()["Rtp"]["Server"]["timeout"];
    static int timeout = Config::instance()->getAndListen([](const json& config){
        timeout = config["SrtCustom"]["Server"]["timeout"];
        logInfo << "srt custom timeout: " << timeout;
    }, "SrtCustom", "Server", "timeout");

    if (timeout == 0) {
        timeout = 5000;
    }

    if (_timeClock.startToNow() > timeout) {
        logInfo << "alive is false";
        _alive = false;
    }
}

void SrtContext::setPayloadType(const string& payloadType)
{
    _payloadType = payloadType;
}

void SrtContext::createVideoTrack(const string& videoCodec)
{
    // _videoTrack = make_shared<RtpDecodeTrack>(VideoTrackType, "raw");
    // _videoTrack->createVideoTrack(videoCodec);
}

void SrtContext::createAudioTrack(const string& audioCodec, int channel, int sampleBit, int sampleRate)
{
    // _audioTrack = make_shared<RtpDecodeTrack>(AudioTrackType, "raw");
    // _audioTrack->createAudioTrack(audioCodec, channel, sampleBit, sampleRate);
}

void SrtContext::handleHandshake(const Buffer::Ptr& pkt)
{
    logInfo << "handleHandshake, pkt size: " << pkt->size();
    HandshakePacket hsPacket;
    if (!hsPacket.parse((uint8_t*)pkt->data(), pkt->size())) {
        logWarn << "parse handshake packet failed";
        return ;
    }

    if (hsPacket.getHandshakeType() == HS_TYPE_INDUCTION) {
        handleHandshakeInduction(hsPacket);
    } else if (hsPacket.getHandshakeType() == HS_TYPE_CONCLUSION) {
        handleHandshakeConclusion(hsPacket);
    } else {
        logWarn << "unknown srt type: " << hsPacket.getHandshakeType();
    }

    _nakClock.update();
    _ackClock.update();
}

void SrtContext::handleHandshakeInduction(const HandshakePacket& hsPacket)
{
    if (_hsInductionRes) {
        sendControlPacket(_hsInductionRes);
        return ;
    }

    _inductionStamp = _now;
    _startStamp = _now;
    _startSeq = hsPacket.getInitialPacketSequenceNumber();
    _lastPktSeq = _startSeq - 1;
    _maxWindowSize = hsPacket.getMaximumFlowWindowSize();
    _mtu = hsPacket.getMaximumTransmissionUnitSize();
    _peerSocketId = hsPacket.getSrtSocketId();

    _estimatedLinkCapacityContext->setLastSeq(_lastPktSeq);

    HandshakePacket::Ptr res = std::make_shared<HandshakePacket>();
    res->setDstSocketId(_peerSocketId);
    res->setTimestamp(_startStamp);
    res->setMaximumTransmissionUnitSize(_mtu);
    res->setMaximumFlowWindowSize(_maxWindowSize);
    res->setInitialPacketSequenceNumber(_startSeq);
    res->setVersion(5);
    res->setEncryptionField(NO_ENCRYPTION);
    res->setExtensionField(0x4A17);
    res->setHandshakeType(HS_TYPE_INDUCTION);
    res->setSrtSocketId(_localSocketId);
    res->setSynCookie(HandshakePacket::generateSynCookie((sockaddr_storage *)_addr.get(), _startStamp));
    // res->setPeerIPAddress(hsPacket.getPeerIPAddress());
    res->assignPeerIP((sockaddr_storage *)_addr.get());
    _hsInductionRes = res->encode();

    _syncCookie = res->getSynCookie();

    logTrace << "add context, syncCookie: " << _syncCookie;
    SrtManager::instance()->addContext(_syncCookie, shared_from_this());
    sendControlPacket(_hsInductionRes);

    if (!_hsInductionTask) {
        weak_ptr<SrtContext> weakSelf = shared_from_this();
        _loop->addTimerTask(200, [weakSelf]() {
            auto self = weakSelf.lock();
            if (!self) {
                return 0;
            }

            self->sendControlPacket(self->_hsInductionRes);
            return 200;
        }, [weakSelf](bool success, const shared_ptr<TimerTask>& task) {
            auto self = weakSelf.lock();
            if (!self) {
                return ;
            }
            self->_hsInductionTask = task;
        });
    }
}

void SrtContext::handleHandshakeConclusion(const HandshakePacket& hsPacket)
{
    if (!_hsInductionRes) {
        logInfo << "handleHandshakeConclusion, _hsInductionRes is null";
        return ;
    }

    if (_hsInductionTask) {
        _hsInductionTask->quit = true;
        _hsInductionTask = nullptr;
    }

    if (_hsConclusionRes) {
        sendControlPacket(_hsConclusionRes);
        return ;
    }

    // static uint32_t latencyMul = Config::instance()->getAndListen([](const json& config){
    //     latencyMul = Config::instance()->get("SrtCustom", "Server", "Server1", "latencyMul");
    //     logInfo << "srt custom latencyMul: " << latencyMul;
    // }, "SrtCustom", "Server", "Server1", "latencyMul");

    shared_ptr<HsExtMessage> msg;
    shared_ptr<HsExtStreamId> sid;

    uint32_t srt_flag = 0xbf;
    // // rtt 的 3到4倍，作为srt delay
    // uint32_t delay = (_now - _inductionStamp) * latencyMul / 1000;
    // delay = delay <= 120 ? 120 : delay;
    // 设置最小的delay
    uint32_t delay = 120;

    for (auto& ext : hsPacket.extensions_) {
        if (ext->type == SRT_CMD_HSREQ) {
            msg = std::dynamic_pointer_cast<HsExtMessage>(ext);
        } else if (ext->type == SRT_CMD_SID) {
            sid = std::dynamic_pointer_cast<HsExtStreamId>(ext);
        }
    }

    if (sid) {
        _uri = sid->streamId;
    }

    if (msg) {
        if (msg->srtFlag != srt_flag) {
            logWarn << "  flag " << msg->srtFlag;
        }
        srt_flag = msg->srtFlag;
        delay = delay <= msg->recvTsbpdDelay ? msg->recvTsbpdDelay : delay;
    }

    HandshakePacket::Ptr res = std::make_shared<HandshakePacket>();
    res->setDstSocketId(_peerSocketId);
    res->setTimestamp(_now - _startStamp);
    res->setMaximumTransmissionUnitSize(_mtu);
    res->setMaximumFlowWindowSize(_maxWindowSize);
    res->setInitialPacketSequenceNumber(_startSeq);
    res->setVersion(5);
    res->setEncryptionField(NO_ENCRYPTION);
    res->setExtensionField(HS_EXT_FILED_HSREQ);
    res->setHandshakeType(HS_TYPE_CONCLUSION);
    res->setSrtSocketId(_localSocketId);
    res->setSynCookie(_syncCookie);
    res->assignPeerIP((sockaddr_storage *)_addr.get());
    std::shared_ptr<HsExtMessage> ext = std::make_shared<HsExtMessage>();
    ext->type = SRT_CMD_HSRSP;
    ext->srtVersion = 5 * 0x100 + 1 * 0x10000;
    ext->srtFlag = srt_flag;
    ext->recvTsbpdDelay = ext->sendTsbpdDelay = delay;
    ext->length = 3;
    res->extensions_.push_back(std::move(ext));
    _hsConclusionRes = res->encode();
    // unregisterSelfHandshake();
    SrtManager::instance()->delContext(_syncCookie);
    // registerSelf();
    SrtManager::instance()->addContext(_localSocketId, shared_from_this());
    sendControlPacket(res->encode());
    logInfo << " buf size = " << res->getMaximumFlowWindowSize() << " init seq =" << _startSeq
            << " latency=" << delay;

    static int srtBufferSize = Config::instance()->getAndListen([](const json& config){
        srtBufferSize = Config::instance()->get("SrtCustom", "Server", "Server1", "bufferSize", "8192");
        logInfo << "srt custom bufferSize: " << srtBufferSize;
    }, "SrtCustom", "Server", "Server1", "bufferSize", "8192");

    _recvQueue = std::make_shared<SrtRecvQueue>(srtBufferSize, _startSeq, delay * 1e3/*,srt_flag*/);
    _sendQueue = std::make_shared<SrtSendQueue>(srtBufferSize, delay * 1e3,srt_flag);
    _sendPktSeq = _startSeq;
    // _buf_delay = delay;

    weak_ptr<SrtContext> weakSelf = shared_from_this();
    _loop->addTimerTask(200, [weakSelf]() {
        auto self = weakSelf.lock();
        if (!self) {
            return 0;
        }

        self->sendControlPacket(self->_hsConclusionRes);
        return 200;
    }, [weakSelf](bool success, const shared_ptr<TimerTask>& task) {
        auto self = weakSelf.lock();
        if (!self) {
            return ;
        }
        self->_hsConclusionTask = task;
    });

    _lastAckPktSeq = _startSeq;
    _handshakeFinish = true;
}

void SrtContext::handleKeepAlive(const Buffer::Ptr& pkt)
{
    logInfo << "handleKeepAlive, pkt size: " << pkt->size();
    KeepAlivePacket::Ptr keepAlivePkt = std::make_shared<KeepAlivePacket>();
    keepAlivePkt->setDstSocketId(_peerSocketId);
    keepAlivePkt->setTimestamp(_now - _startStamp);
    keepAlivePkt->setControlType(ControlPacketType::KEEPALIVE);
    keepAlivePkt->setSubtype(0);
    sendControlPacket(keepAlivePkt->encode());
}

void SrtContext::handleAck(const Buffer::Ptr& pkt)
{
    logInfo << "handleAck, pkt size: " << pkt->size();
    AckPacket::Ptr ackPkt = std::make_shared<AckPacket>();
    if (!ackPkt->parse((uint8_t*)pkt->data(), pkt->size())) {
        logError << "handleAck, parse ack pkt failed";
        return ;
    }

    logInfo << "handleAck, ack number: " << ackPkt->getAckNumber();
    if (ackPkt->getAckNumber() != 0) {
        AckAckPacket::Ptr ackAckPkt = std::make_shared<AckAckPacket>();
        ackAckPkt->setAckNumber(ackPkt->getAckNumber());
        // ackAckPkt->setSubtype(0);
        // ackAckPkt->setControlType(ACKACK);
        ackAckPkt->setDstSocketId(_peerSocketId);
        ackAckPkt->setTimestamp(_now - _startStamp);
        sendControlPacket(ackAckPkt->encode());

        auto recvRate = ackPkt->getReceivingRate();
        auto maxBandwidth = recvRate * (1 + 5.0 / 100);
        maxBandwidth = maxBandwidth > _maxBandwidth ? _maxBandwidth : maxBandwidth;
        _pktSendPeriod = _avgPayloadSize * 1e6 / maxBandwidth;

        _rtt = ackPkt->getRtt();
        _rttVariance = ackPkt->getRttVariance();
        // srt延迟为rtt的3到4倍，缓存时间的阈值为srt延迟的1.25倍
        auto delay = _rtt * 4 * 1.25;
        delay = delay <= 120 ? 120 : delay;

        // _recvQueue->setDelay(delay);
        // _sendQueue->setDelay(delay);

        // 2倍ack发送的周期
        static int syn = 2 * 10 * 1e3;
        if (_rexmitCount == 0) {
            _rto = _rtt + 4 * _rttVariance + syn;
        } else {
            _rto = _rexmitCount * (_rtt + 4 * _rttVariance + syn) + syn;
        }
    }

    _sendQueue->drop(ackPkt->getLastAckPacketSeqNumber());
}

void SrtContext::sendMsgDropReq(uint32_t first, uint32_t last) {
    MsgDropReqPacket::Ptr pkt = std::make_shared<MsgDropReqPacket>();
    pkt->setDstSocketId(_peerSocketId);
    pkt->setTimestamp(_now - _startStamp);
    pkt->setFirstPacketSeqNumber(first);
    pkt->setLastPacketSeqNumber(last);
    sendControlPacket(pkt->encode());
}

void SrtContext::handleNak(const Buffer::Ptr& buffer)
{
    logInfo << "handleNak, pkt size: " << buffer->size();
    NakPacket nakPkt;
    nakPkt.parse((uint8_t*)buffer->data(), buffer->size());
    bool empty = false;
    bool flush = false;

    for (auto& it : nakPkt.lossList_) {
        empty = true;
        auto re_list = _sendQueue->findPacketBySeq(it.first, it.second);
        for (auto& pkt : re_list) {
            pkt->setR(1);
            auto data = pkt->encode(nullptr, 0);
            _socket->send(data, 1, 0, data->size(), _addr.get(), sizeof(sockaddr));
            empty = false;
        }
        if (empty) {
            sendMsgDropReq(it.first, it.second);
        }
    }
}

void SrtContext::handleCongestionWarning(const Buffer::Ptr& pkt)
{
    logInfo << "handleCongestionWarning, pkt size: " << pkt->size();
}

void SrtContext::handleShutdown(const Buffer::Ptr& pkt)
{
    logInfo << "handleShutdown, pkt size: " << pkt->size();
    _alive = false;
}

void SrtContext::handleAckAck(const Buffer::Ptr& pkt)
{
    logInfo << "handleAckAck, pkt size: " << pkt->size();
    AckAckPacket::Ptr ackAckPkt = std::make_shared<AckAckPacket>();
    ackAckPkt->parse((uint8_t*)pkt->data(), pkt->size());

    if(_ackSendTimestamp.find(ackAckPkt->getAckNumber())!=_ackSendTimestamp.end()){
        uint32_t rtt = _now - _ackSendTimestamp[ackAckPkt->getAckNumber()];
        _rttVariance = (3 * _rttVariance + abs((long)rtt - (long)_rtt)) / 4;
        _rtt = (7 * rtt + _rtt) / 8;
        // TraceL<<" rtt:"<<_rtt<<" rtt variance:"<<_rtt_variance;
        _ackSendTimestamp.erase(ackAckPkt->getAckNumber());

        if(_lastRecvAckackSeqNum < ackAckPkt->getAckNumber()){
            _lastRecvAckackSeqNum = ackAckPkt->getAckNumber();
        }else{
            if((_lastRecvAckackSeqNum - ackAckPkt->getAckNumber()) > (MAX_TS >> 1)){
                _lastRecvAckackSeqNum = ackAckPkt->getAckNumber();
            }
        }

        if(_ackSendTimestamp.size()>1000){
            // clear data
            for(auto it = _ackSendTimestamp.begin(); it != _ackSendTimestamp.end();){
                if(_now-it->second>5e6){
                    // 超过五秒没有ackack 丢弃  [AUTO-TRANSLATED:9626442f]
                    // Discard if no ACK for more than five seconds
                    it = _ackSendTimestamp.erase(it);
                }else{
                    it++;
                }
            }
        }

    }
}

void SrtContext::handleDropReq(const Buffer::Ptr& pkt)
{
    logInfo << "handleDropReq, pkt size: " << pkt->size();
    MsgDropReqPacket dropReqPkt;
    dropReqPkt.parse((uint8_t*)pkt->data(), pkt->size());
    std::list<DataPacket::Ptr> list;
    // TraceL<<"drop "<<pkt.first_pkt_seq_num<<" last "<<pkt.last_pkt_seq_num;
    _recvQueue->drop(dropReqPkt.getFirstPacketSeqNumber(), dropReqPkt.getLastPacketSeqNumber(), list);
    //checkAndSendAckNak();
    if (list.empty()) {
        return;
    }
    // uint32_t max_seq = 0;
    for (auto& data : list) {
        // max_seq = data->packet_seq_number;
        if (_lastPktSeq + 1 != data->getPacketSeqNumber()) {
            logTrace << "pkt lost " << _lastPktSeq + 1 << "->" << data->getPacketSeqNumber();
        }
        _lastPktSeq = data->getPacketSeqNumber();
        inputSrtData(std::move(data));
    }
}

void SrtContext::handlePeerError(const Buffer::Ptr& pkt)
{
    logInfo << "handlePeerError, pkt size: " << pkt->size();
}

void SrtContext::handleUserDefinedType(const Buffer::Ptr& pkt)
{
    logInfo << "handleUserDefinedType, pkt size: " << pkt->size();
}

void SrtContext::handleDataPacket(const Buffer::Ptr& buffer)
{
    logInfo << "handleDataPacket, pkt size: " << buffer->size();
    DataPacket::Ptr pkt = std::make_shared<DataPacket>();
    pkt->parse((uint8_t*)buffer->data(), buffer->size());

    _estimatedLinkCapacityContext->inputPacket(_now, pkt);

    std::list<DataPacket::Ptr> list;
    //TraceL<<" seq="<< pkt->packet_seq_number<<" ts="<<pkt->timestamp<<" size="<<pkt->payloadSize()<<\
    //" PP="<<(int)pkt->PP<<" O="<<(int)pkt->O<<" kK="<<(int)pkt->KK<<" R="<<(int)pkt->R;
    _recvQueue->inputPacket(pkt, list);
    if (list.empty()) {
        // when no data ok send nack to sender immediately
    } else {
        // uint32_t last_seq;
        for (auto& data : list) {
            // last_seq = data->packet_seq_number;
            if (_lastPktSeq + 1 != data->getPacketSeqNumber()) {
                logTrace << "pkt lost " << _lastPktSeq + 1 << "->" << data->getPacketSeqNumber();
            }
            _lastPktSeq = data->getPacketSeqNumber();
            inputSrtData(std::move(data));
        }

        //_recv_nack.drop(last_seq);
    }
}

void SrtContext::checkAndSendAckNak()
{
    auto nak_interval = (_rtt + _rttVariance * 4) / 2;
    if (nak_interval <= 20 * 1000) {
        nak_interval = 20 * 1000;
    }
    if (_nakClock.startToNow() > nak_interval) {
        auto lost = _recvQueue->getLostSeq();
        if (!lost.empty()) {
            sendNakPacket(lost);
        }
        _nakClock.update();
    }

    if (_ackClock.startToNow() > 10 * 1000) {
        _lightAckPktCount = 0;
        _ackClock.update();
        // send a ack per 10 ms for receiver
        if(_lastAckPktSeq != _recvQueue->getExpectedSeq()){
            //logTrace<<"send a ack packet";
            sendAckPacket();
        }else{
            //logTrace<<" ignore repeate ack packet";
        }
        
    } else {
        if (_lightAckPktCount >= 64) {
            // for high bitrate stream send light ack
            // TODO
            sendLightAckPacket();
            logTrace << "send light ack";
        }
        _lightAckPktCount = 0;
    }
    _lightAckPktCount++;
}

void SrtContext::sendAckPacket() {
    uint32_t recv_rate = 0;

    AckPacket::Ptr pkt = std::make_shared<AckPacket>();
    pkt->setDstSocketId(_peerSocketId);
    pkt->setTimestamp(_now - _startStamp);
    pkt->setAckNumber(++_ackNumberCount);
    pkt->setLastAckPacketSeqNumber(_recvQueue->getExpectedSeq());
    pkt->setRtt(_rtt);
    pkt->setRttVariance(_rttVariance);
    pkt->setAvailableBufferSize(_recvQueue->getAvailableBufferSize());
    pkt->setPacketsReceivingRate(_pktRecvRateContext->getPacketRecvRate(recv_rate));
    pkt->setEstimatedLinkCapacity(_estimatedLinkCapacityContext->getEstimatedLinkCapacity());
    pkt->setReceivingRate(recv_rate);
    if(0){
        logTrace << pkt->getPacketsReceivingRate()<<" pkt/s "<<recv_rate<<" byte/s "<<pkt->getEstimatedLinkCapacity()<<" pkt/s (cap) "<<pkt->getAvailableBufferSize()<<" available buf";
        //logTrace<<_pktRecvRateContext->dump();
        //logTrace<<"recv estimated:";
        //logTrace<< _pktRecvRateContext->dump();
        //logTrace<<"recv queue:";
        //logTrace<<_recvQueue->dump();
    }
    if(pkt->getAvailableBufferSize()<2){
        pkt->setAvailableBufferSize(2);
    }
    auto buffer = pkt->encode();
    _ackSendTimestamp[pkt->getAckNumber()] = _now;
    _lastAckPktSeq = pkt->getLastAckPacketSeqNumber();
    sendControlPacket(buffer);
    // logTrace<<"send  ack "<<pkt->dump();
    // logTrace<<_recv_buf->dump();
}

void SrtContext::sendLightAckPacket() {
    AckPacket::Ptr pkt = std::make_shared<AckPacket>();

    pkt->setDstSocketId(_peerSocketId);
    pkt->setTimestamp(_now - _startStamp);
    pkt->setAckNumber(0);
    pkt->setLastAckPacketSeqNumber(_recvQueue->getExpectedSeq());
    pkt->setRtt(0);
    pkt->setRttVariance(0);
    pkt->setAvailableBufferSize(0);
    pkt->setPacketsReceivingRate(0);
    pkt->setEstimatedLinkCapacity(0);
    pkt->setReceivingRate(0);
    auto buffer = pkt->encode();
    _lastAckPktSeq = pkt->getLastAckPacketSeqNumber();
    sendControlPacket(buffer);
    // TraceL << "send  ack " << pkt->dump();
}

void SrtContext::sendNakPacket(std::list<SrtRecvQueue::LostPair> &lost_list) {
    NakPacket::Ptr pkt = std::make_shared<NakPacket>();
    std::list<SrtRecvQueue::LostPair> tmp;
    auto size = NakPacket::getCIFSize(lost_list);
    size_t paylaod_size = (_mtu - 28 - 16) / 188 * 188;
    if (size > paylaod_size) {
        logWarn << "loss report cif size " << size;
        size_t num = paylaod_size / 8;

        size_t msgNum = (lost_list.size() + num - 1) / num;
        decltype(lost_list.begin()) cur, next;
        for (size_t i = 0; i < msgNum; ++i) {
            cur = lost_list.begin();
            std::advance(cur, i * num);

            if (i == msgNum - 1) {
                next = lost_list.end();
            } else {
                next = lost_list.begin();
                std::advance(next, (i + 1) * num);
            }
            tmp.assign(cur, next);
            pkt->setDstSocketId(_peerSocketId);
            pkt->setTimestamp(_now - _startStamp);
            pkt->lossList_ = tmp;
            sendControlPacket(pkt->encode());
        }

    } else {
        pkt->setDstSocketId(_peerSocketId);
        pkt->setTimestamp(_now - _startStamp);
        pkt->lossList_ = lost_list;
        sendControlPacket(pkt->encode());
    }

    // logTrace<<"send NAK "<<pkt->dump();
}

void SrtContext::sendShutDown() {
    ShutdownPacket::Ptr pkt = std::make_shared<ShutdownPacket>();
    pkt->setDstSocketId(_peerSocketId);
    pkt->setTimestamp(_now - _startStamp);
    sendControlPacket(pkt->encode());
}

void SrtContext::sendControlPacket(const Buffer::Ptr& pkt)
{
    _socket->send(pkt, 1, 0, pkt->size(), _addr.get(), sizeof(sockaddr));
}

void SrtContext::sendDataPacket(DataPacket::Ptr pkt, char *buf, int len, bool flush) {
    auto buffer = pkt->encode((uint8_t *)buf, len);
    _socket->send(buffer, 1, 0, buffer->size(), _addr.get(), sizeof(sockaddr));
    _sendQueue->inputPacket(pkt);

    _sendBytes_1s += buffer->size();
    if (_sendRateClock.startToNow() > 1000) {
        _sendRate = _sendBytes_1s * 1000.0 / _sendRateClock.startToNow();
        _sendBytes_1s = 0;

        // 先设置5%的开销,后面改成读配置
        int overhead = 5;
        _maxBandwidth = _sendRate * (1 + overhead / 100.0);
    }

    if (_avgPayloadSize == 0) {
        _avgPayloadSize = buffer->size();
    } else {
        _avgPayloadSize = _avgPayloadSize * 7 / 8 + buffer->size() / 8;
    }
}

void SrtContext::onError(const string& msg)
{
    logError << "onError, msg: " << msg;
}

void SrtContext::handlePull()
{
    weak_ptr<SrtContext> wSelf = shared_from_this();

    MediaSource::getOrCreateAsync(_urlParser.path_, _urlParser.vhost_, _urlParser.protocol_, _urlParser.type_, 
    [wSelf](const MediaSource::Ptr &src){
        logInfo << "get a src";
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

		auto tsSrc = dynamic_pointer_cast<TsMediaSource>(src);
		if (!tsSrc) {
			self->onError("ts source is not exist");
            return ;
		}

        self->_source = tsSrc;

		self->_loop->async([wSelf, tsSrc](){
			auto self = wSelf.lock();
			if (!self) {
				return ;
			}

			self->onPlayTs(tsSrc);
		}, true);
    }, 
    [wSelf]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }
        return make_shared<TsMediaSource>(self->_urlParser, nullptr, true);
    }, this);
}


void SrtContext::onPlayTs(const TsMediaSource::Ptr &tsSrc)
{
    weak_ptr<SrtContext> wSelf = shared_from_this();

    if (!_playTsReader) {
		logInfo << "set _playTsReader";
		_playTsReader = tsSrc->getRing()->attach(_loop, true);
		_playTsReader->setGetInfoCB([wSelf]() {
			auto self = wSelf.lock();
			ClientInfo ret;
			if (!self) {
				return ret;
			}
			ret.ip_ = self->_socket->getLocalIp();
			ret.port_ = self->_socket->getLocalPort();
			ret.protocol_ = PROTOCOL_SRT;
			return ret;
		});
		_playTsReader->setDetachCB([wSelf]() {
			auto strong_self = wSelf.lock();
			if (!strong_self) {
				return;
			}
			// strong_self->shutdown(SockException(Err_shutdown, "rtsp ring buffer detached"));
			strong_self->onError("ring buffer detach");
		});
		logInfo << "setReadCB =================";
		_playTsReader->setReadCB([wSelf](const TsMediaSource::RingDataType &pack) {
			auto self = wSelf.lock();
			if (!self/* || pack->empty()*/ || !self->_socket) {
				return;
			}
			// logInfo << "send rtmp msg";
			auto pktList = *(pack.get());
			for (auto& pkt : pktList) {
                // self->_socket->send(pkt, 1, 0, pkt->size(), self->_addr.get(), sizeof(sockaddr));
				// uint8_t frame_type = (pkt->payload.get()[0] >> 4) & 0x0f;
				// uint8_t codec_id = pkt->payload.get()[0] & 0x0f;

				// FILE* fp = fopen("testflv.rtmp", "ab+");
				// fwrite(pkt->payload.get(), pkt->length, 1, fp);
				// fclose(fp);

				// logInfo << "frame_type : " << (int)frame_type << ", codec_id: " << (int)codec_id
				// 		<< "pkt->payload.get()[0]: " << (int)(pkt->payload.get()[0]);
				// self->sendMediaData(pkt->type_id, pkt->abs_timestamp, pkt->payload, pkt->length);
                self->sendTSData(pkt, true);
			}
		});
	}
}

void SrtContext::sendTSData(const Buffer::Ptr &buffer, bool flush)
{
    // TraceL;
    DataPacket::Ptr pkt;
    size_t payloadSize = (_mtu - 28 - 16) / 188 * 188;
    size_t size = buffer->size();
    char *ptr = buffer->data();
    char *end = buffer->data() + size;
    auto now = TimeClock::nowMicro();

    while (ptr < end && size >= payloadSize) {
        pkt = std::make_shared<DataPacket>();
        pkt->setF(0);
        pkt->setPacketSeqNumber(_sendPktSeq & 0x7fffffff);
        _sendPktSeq = (_sendPktSeq + 1) & 0x7fffffff;
        pkt->setPP(3);
        pkt->setO(0);
        pkt->setKK(0);
        pkt->setR(0);
        pkt->setMsgNumber(_sendMsgNumber++);
        pkt->setDstSocketId(_peerSocketId);
        pkt->setTimestamp(now - _startStamp);
        sendDataPacket(pkt, ptr, (int)payloadSize, flush);
        ptr += payloadSize;
        size -= payloadSize;
    }

    if (size > 0 && ptr < end) {
        pkt = std::make_shared<DataPacket>();
        pkt->setF(0);
        pkt->setPacketSeqNumber(_sendPktSeq & 0x7fffffff);
        _sendPktSeq = (_sendPktSeq + 1) & 0x7fffffff;
        pkt->setPP(3);
        pkt->setO(0);
        pkt->setKK(0);
        pkt->setR(0);
        pkt->setMsgNumber(_sendMsgNumber++);
        pkt->setDstSocketId(_peerSocketId);
        pkt->setTimestamp(now - _startStamp);
        sendDataPacket(pkt, ptr, (int)size, flush);
    }
}

void SrtContext::initPush()
{
    if (!_inited) {
        auto source = MediaSource::getOrCreate(_urlParser.path_, _urlParser.vhost_
                        , _urlParser.protocol_, _urlParser.type_
                        , [this](){
                            return make_shared<TsMediaSource>(_urlParser, _loop);
                        });

        if (!source) {
            logWarn << "another stream is exist with the same uri";
            return ;
        }
        logInfo << "create a TsMediaSource";
        auto tsSource = dynamic_pointer_cast<TsMediaSource>(source);
        tsSource->setOrigin();
        tsSource->setAction(true);

        auto tsDemuxer = make_shared<TsDemuxer>();

        tsSource->addTrack(tsDemuxer);
        _source = tsSource;

        // weak_ptr<SrtConnection> wSelf = shared_from_this();
        // _socket->setReadCb([wSelf](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
        //     auto self = wSelf.lock();
		// 	if (!self/* || pack->empty()*/) {
		// 		return -1;
		// 	}
        //     self->onRead(buffer, addr, len);
        //     return 0;
        // });

        _inited = true;
    }
}

void SrtContext::inputSrtData(const DataPacket::Ptr &pkt)
{
    auto tsSrc = _source.lock();
    if (!tsSrc) {
        logError << "ts source is not exist";
        return;
    }
    tsSrc->inputTs(pkt->getData());
}