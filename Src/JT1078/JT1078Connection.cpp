﻿#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "JT1078Connection.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"
#include "Common/HookManager.h"
#include "Common/Config.h"

using namespace std;

mutex JT1078Connection::_lck;
unordered_map<string, JT1078Info> JT1078Connection::_mapJt1078Info;

mutex JT1078Connection::_talkLck;
unordered_map<string, JT1078TalkInfo> JT1078Connection::_mapTalkInfo;

JT1078Connection::JT1078Connection(const EventLoop::Ptr& loop, const Socket::Ptr& socket)
    :TcpConnection(loop, socket)
    ,_loop(loop)
    ,_socket(socket)
{
    logTrace << "JT1078Connection: " << this;
}

JT1078Connection::~JT1078Connection()
{
    logTrace << "~JT1078Connection: " << this << ", path: " << _path;
    auto jtSrc = _source.lock();
    if (jtSrc) {
        jtSrc->release();
        jtSrc->delConnection(this);
    }

    auto talkSrc = _talkSource.lock();
    if (talkSrc) {
        jtSrc->delConnection(this);
    }

    // if (_onClose) {
    //     _onClose();
    // }

    // if (!_key.empty()) {
    //     delJt1078Info(_key);
    // }
}

void JT1078Connection::init()
{
    weak_ptr<JT1078Connection> wSelf = static_pointer_cast<JT1078Connection>(shared_from_this());
    _parser.setOnRtpPacket([wSelf](const JT1078RtpPacket::Ptr& buffer){
        auto self = wSelf.lock();
        if(!self){
            return;
        }
        
        self->onRtpPacket(buffer);
    });

    _timeClock.start();
}

void JT1078Connection::close()
{
    logTrace << "path: " << _path << "JT1078Connection::close()";
    TcpConnection::close();
    if (_onClose) {
        _onClose();
    }
}

void JT1078Connection::onManager()
{
    logTrace << "path: " << _path << ", on manager";
    static int timeout = Config::instance()->getAndListen([](const json& config){
        timeout = Config::instance()->get("JT1078", "Server", "timeout", "", "5000");
    }, "JT1078", "Server", "timeout", "", "5000");

    int curTimeout = _timeout == 0 ? timeout : _timeout;

    if (_timeClock.startToNow() > curTimeout) {
        logWarn << _path <<  ": timeout";
        weak_ptr<JT1078Connection> wSelf = dynamic_pointer_cast<JT1078Connection>(shared_from_this());
        // 直接close会将tcpserver的map迭代器破坏，用异步接口关闭
        _loop->async([wSelf](){
            auto self = wSelf.lock();
            if (self) {
                self->close();
            }
        }, false);
    }
}

void JT1078Connection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    // logInfo << "get a buf: " << buffer->size();
    _parser.parse(buffer->data(), buffer->size());
}

void JT1078Connection::onError(const string& msg)
{
    close();
    logWarn << "get a error: " << msg;
}

ssize_t JT1078Connection::send(Buffer::Ptr pkt)
{
    logInfo << "pkt size: " << pkt->size();
    return TcpConnection::send(pkt);
}

void JT1078Connection::onRtpPacket(const JT1078RtpPacket::Ptr& buffer)
{
    // logInfo << "simcode: " << buffer->getSimCode();
    _timeClock.update();
    if (_first) {
        _first = false;
        
        PublishInfo info;
        info.protocol = PROTOCOL_JT1078;
        info.type = _isTalk ? "talk" : DEFAULT_TYPE;

        string key = buffer->getSimCode() + "_" + to_string(buffer->getLogicNo())
                    + "_" + to_string(_socket->getLocalPort());
        auto jt1078Info = getJt1078Info(key);
        if (!jt1078Info.streamName.empty()) {
            // _key = key;
            info.uri = "/" + jt1078Info.appName + "/" + jt1078Info.streamName;
            _timeout = jt1078Info.timeout;
        } else {
            info.uri = "/" + _app + "/" + key;
        }

        info.vhost = DEFAULT_VHOST;

        weak_ptr<JT1078Connection> wSelf = dynamic_pointer_cast<JT1078Connection>(shared_from_this());
        auto hook = HookManager::instance()->getHook(MEDIA_HOOK);
        if (hook) {
            hook->onPublish(info, [wSelf, buffer, hook](const PublishResponse &rsp){
                auto self = wSelf.lock();
                if (!self) {
                    return ;
                }

                if (!rsp.authResult) {
                    self->onError("publish auth failed");
                    return ;
                }

                static int streamHeartbeatTime = Config::instance()->getAndListen([](const json &config){
                    streamHeartbeatTime = Config::instance()->get("Util", "streamHeartbeatTime");
                }, "Util", "streamHeartbeatTime");

                if (self->_loop) {
                    self->_loop->addTimerTask(streamHeartbeatTime, [wSelf, hook](){
                        auto self = wSelf.lock();
                        if (!self) {
                            return 0;
                        }

                        auto jt1078Source = self->_source.lock(); 
                        if (!jt1078Source) {
                            return 0;
                        }

                        StreamHeartbeatInfo info;
                        info.type = jt1078Source->getType();
                        info.protocol = jt1078Source->getProtocol();
                        info.uri = jt1078Source->getPath();
                        info.vhost = jt1078Source->getVhost();
                        info.playerCount = jt1078Source->totalPlayerCount();
                        info.createTime = jt1078Source->getCreateTime();
                        info.bytes = jt1078Source->getBytes();
                        info.currentTime = TimeClock::now();
                        info.bitrate = jt1078Source->getBitrate();
                        hook->onStreamHeartbeat(info);

                        return streamHeartbeatTime;
                    }, nullptr);
                }

                // 本来想通过回调来进行指定流名称，但是会导致上线慢，暂时屏蔽
                // if (self->_path.empty() && !rsp.appName.empty() && !rsp.streamName.empty()) {
                //     self->_path = "/" + rsp.appName + "/" + rsp.streamName;
                // }

                // self->createSource(buffer);
            });
        }

        // 不在鉴权回调里创建source了，提升上线速度
        createSource(buffer);
    }

    if (!_source.lock()) {
        return ;
    }

    int index = 0;
    if (buffer->getTrackType() == "video") {
        index = VideoTrackType;
    } else if (buffer->getTrackType() == "audio") {
        index = AudioTrackType;
    } else {
        return ;
    }
    // logInfo << "codec: " << buffer->getCodecType() << ", index: " << index;
    auto iter = _mapTrack.find(index);
    if (iter == _mapTrack.end()) {
        auto track = make_shared<JT1078DecodeTrack>(index);
        _mapTrack[index] = track;
        auto jtSrc = _source.lock();
        if (jtSrc) {
            jtSrc->addTrack(track);
        }
    }

    _mapTrack[index]->onRtpPacket(buffer);
}

void JT1078Connection::createSource(const JT1078RtpPacket::Ptr& buffer)
{
    UrlParser parser;
    if (_path.empty()) {
        _simCode = buffer->getSimCode();
        _channel = buffer->getLogicNo();
        string key = buffer->getSimCode() + "_" + to_string(buffer->getLogicNo())
                    + "_" + to_string(_socket->getLocalPort());
        auto info = getJt1078Info(key);
        if (!info.streamName.empty()) {
            // _key = key;
            parser.path_ = "/" + info.appName + "/" + info.streamName;
            delJt1078Info(key);
        } else {
            parser.path_ = "/" + _app + "/" + buffer->getSimCode() + "_" + to_string(buffer->getLogicNo());
        }
        _path = parser.path_;
    } else {
        parser.path_ = _path;
    }
    parser.vhost_ = DEFAULT_VHOST;
    parser.protocol_ = PROTOCOL_JT1078;
    parser.type_ = _isTalk ? "talk" : DEFAULT_TYPE;
    auto source = MediaSource::getOrCreate(parser.path_, parser.vhost_
                    , parser.protocol_, parser.type_
                    , [this, parser](){
                        return make_shared<JT1078MediaSource>(parser, _loop);
                    });

    if (!source) {
        logWarn << "another stream is exist with the same uri: " << _path;
        close();
        return ;
    }
    logInfo << "create a JT1078MediaSource: " << _path;
    auto jtSrc = dynamic_pointer_cast<JT1078MediaSource>(source);
    if (!jtSrc) {
        logWarn << "source is not gb source: " << _path;
        return ;
    }
    jtSrc->setOrigin();
    jtSrc->setOriginSocket(_socket);
    jtSrc->setAction(true);
    weak_ptr<JT1078Connection> wSelf = dynamic_pointer_cast<JT1078Connection>(shared_from_this());
    jtSrc->addOnDetach(this, [wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        self->close();
    });
    _source = jtSrc;

    if (_isTalk) {
        onJT1078Talk(buffer);
    }
}

void JT1078Connection::onJT1078Talk(const JT1078RtpPacket::Ptr& buffer)
{
    string key = buffer->getSimCode() + "_" + to_string(buffer->getLogicNo())
                        + "_" + to_string(_socket->getLocalPort());
    auto info = getTalkInfo(key);
    if (info.urlParser.path_.empty()) {
        delTalkInfo(key);
        return ;
    }
    delTalkInfo(key);

    // 写死信息，方便测试，测试完注意改回上面的代码
    // JT1078TalkInfo info;
    // info.channel = 1;
    // info.simCode = "13333333333";
    // info.urlParser.vhost_ = DEFAULT_VHOST;
    // info.urlParser.path_ = "/live/test";
    // info.urlParser.type_ = DEFAULT_TYPE;
    // info.urlParser.protocol_ = PROTOCOL_JT1078;

    weak_ptr<JT1078Connection> wSelf = dynamic_pointer_cast<JT1078Connection>(shared_from_this());
    MediaSource::getOrCreateAsync(info.urlParser.path_, info.urlParser.vhost_, info.urlParser.protocol_, info.urlParser.type_, 
    [wSelf, info](const MediaSource::Ptr &src){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        self->_loop->async([wSelf, src, info](){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            self->startSendTalkData(dynamic_pointer_cast<JT1078MediaSource>(src), info);
        }, true);
    }, 
    [wSelf, info]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }

        auto source = make_shared<JT1078MediaSource>(info.urlParser, nullptr, true);
        source->setSimCode(info.simCode);
        source->setChannel(info.channel);

        return source;
    }, this);
}

void JT1078Connection::startSendTalkData(const JT1078MediaSource::Ptr &jtSrc, const JT1078TalkInfo& talkInfo)
{
    if (!jtSrc) {
        return ;
    }
    _talkSource = jtSrc;

    if (!_playReader) {
        logTrace << "path: " << _path << ", set _playReader";
        weak_ptr<JT1078Connection> wSelf = dynamic_pointer_cast<JT1078Connection>(shared_from_this());
        _playReader = jtSrc->getRing()->attach(_socket->getLoop(), true);
        _playReader->setGetInfoCB([wSelf]() {
            auto self = wSelf.lock();
            ClientInfo ret;
            if (!self) {
                return ret;
            }
            ret.ip_ = self->_socket->getLocalIp();
            ret.port_ = self->_socket->getLocalPort();
            ret.protocol_ = PROTOCOL_JT1078;
            return ret;
        });
        _playReader->setDetachCB([wSelf]() {
            auto self = wSelf.lock();
            if (!self) {
                return;
            }
            // // strong_self->shutdown(SockException(Err_shutdown, "rtsp ring buffer detached"));
            self->close();
        });
        logTrace << "path: " << _path << ", setReadCB =================";
        _playReader->setReadCB([wSelf](const JT1078MediaSource::RingDataType &pack) {
            auto self = wSelf.lock();
            if (!self/* || pack->empty()*/) {
                return;
            }
            // logInfo << "send rtp packet";
            // auto pktList = *(pack.get());
            // for (auto& rtp : pktList) {
            //     logInfo << "send rtmp msg,time: " << pkt->abs_timestamp << ", type: " << (int)(pkt->type_id)
            //                 << ", length: " << pkt->length;
                self->sendRtpPacket(pack);
            // }
        });

        PlayerInfo info;
        info.ip = _socket->getPeerIp();
        info.port = _socket->getPeerPort();
        info.protocol = PROTOCOL_JT1078;
        info.status = "on";
        info.type = talkInfo.urlParser.type_;
        info.uri = talkInfo.urlParser.path_;
        info.vhost = talkInfo.urlParser.vhost_;

        auto hook = HookManager::instance()->getHook(MEDIA_HOOK);
        if (hook) {
            hook->onPlayer(info);
        }
    }
}

void JT1078Connection::sendRtpPacket(const JT1078MediaSource::RingDataType &pkt)
{
    logTrace << "path: " << _path << ", send packet to device";
    for (auto it = pkt->begin(); it != pkt->end(); ++it) {
        auto packet = it->get();

        _socket->send(packet->buffer(), 0);
    };
    _socket->send((Buffer::Ptr)nullptr, 1);
}

bool JT1078Connection::addJt1078Info(const string& key, const JT1078Info& info)
{
    logDebug << "add info, key: " << key;
    lock_guard<mutex> lck(_lck);
    auto res = _mapJt1078Info.emplace(key, info);

    return res.second;
}

JT1078Info JT1078Connection::getJt1078Info(const string& key)
{
    logDebug << "get info, key: " << key;
    lock_guard<mutex> lck(_lck);
    JT1078Info info;
    if (_mapJt1078Info.find(key) == _mapJt1078Info.end()) {
        return info;
    }

    return _mapJt1078Info[key];
}

void JT1078Connection::delJt1078Info(const string& key)
{
    logDebug << "del info, key: " << key;
    lock_guard<mutex> lck(_lck);
    _mapJt1078Info.erase(key);
}

bool JT1078Connection::addTalkInfo(const string& key, const JT1078TalkInfo& info)
{
    logDebug << "add info, key: " << key;
    lock_guard<mutex> lck(_talkLck);
    auto res = _mapTalkInfo.emplace(key, info);

    return res.second;
}

JT1078TalkInfo JT1078Connection::getTalkInfo(const string& key)
{
    logDebug << "get info, key: " << key;
    lock_guard<mutex> lck(_talkLck);
    JT1078TalkInfo info;
    if (_mapTalkInfo.find(key) == _mapTalkInfo.end()) {
        return info;
    }

    return _mapTalkInfo[key];
}

void JT1078Connection::delTalkInfo(const string& key)
{
    logDebug << "del info, key: " << key;
    lock_guard<mutex> lck(_talkLck);
    _mapTalkInfo.erase(key);
}

