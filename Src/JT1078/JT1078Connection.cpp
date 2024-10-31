#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "JT1078Connection.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"
#include "Hook/MediaHook.h"

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
    logInfo << "JT1078Connection";
}

JT1078Connection::~JT1078Connection()
{
    logInfo << "~JT1078Connection";
    auto jtSrc = _source.lock();
    if (jtSrc) {
        jtSrc->release();
        jtSrc->delConnection(this);
    }

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
}

void JT1078Connection::close()
{
    logInfo << "JT1078Connection::close()";
    TcpConnection::close();
}

void JT1078Connection::onManager()
{
    logInfo << "manager";
}

void JT1078Connection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    // logInfo << "get a buf: " << buffer->size();
    _parser.parse(buffer->data(), buffer->size());
}

void JT1078Connection::onError()
{
    close();
    logWarn << "get a error: ";
}

ssize_t JT1078Connection::send(Buffer::Ptr pkt)
{
    logInfo << "pkt size: " << pkt->size();
    return TcpConnection::send(pkt);
}

void JT1078Connection::onRtpPacket(const JT1078RtpPacket::Ptr& buffer)
{
    // logInfo << "simcode: " << buffer->getSimCode();
    if (!_source.lock()) {
        UrlParser parser;
        if (_path.empty()) {
            string key = buffer->getSimCode() + "_" + to_string(buffer->getLogicNo())
                        + "_" + to_string(_socket->getLocalPort());
            auto info = getJt1078Info(key);
            if (!info.streamName.empty()) {
                // _key = key;
                parser.path_ = "/" + info.appName + "/" + info.streamName;
                delJt1078Info(key);
            } else {
                parser.path_ = "/live/" + buffer->getSimCode() + "_" + to_string(buffer->getLogicNo());
            }
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
            logWarn << "another stream is exist with the same uri";
            return ;
        }
        logInfo << "create a JT1078MediaSource";
        auto jtSrc = dynamic_pointer_cast<JT1078MediaSource>(source);
        if (!jtSrc) {
            logWarn << "source is not gb source";
            return ;
        }
        jtSrc->setOrigin();
        jtSrc->setOriginSocket(_socket);
        weak_ptr<JT1078Connection> wSelf = dynamic_pointer_cast<JT1078Connection>(shared_from_this());
        jtSrc->addOnDetach(this, [wSelf](){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }
            self->close();
        });
        _source = jtSrc;

        PublishInfo info;
        info.protocol = parser.protocol_;
        info.type = parser.type_;
        info.uri = parser.path_;
        info.vhost = parser.vhost_;

        // weak_ptr<JT1078Connection> wSelf = dynamic_pointer_cast<JT1078Connection>(shared_from_this());
        MediaHook::instance()->onPublish(info, [wSelf](const PublishResponse &rsp){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            if (!rsp.authResult) {
                self->onError();
            }
        });

        if (_isTalk) {
            onJT1078Talk(buffer);
        }
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
    if (!_playReader) {
        logInfo << "set _playReader";
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
        logInfo << "setReadCB =================";
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

        MediaHook::instance()->onPlayer(info);
    }
}

void JT1078Connection::sendRtpPacket(const JT1078MediaSource::RingDataType &pkt)
{
    logInfo << "JT1078Connection::sendRtpPacket";
    for (auto it = pkt->begin(); it != pkt->end(); ++it) {
        auto packet = it->get();

        _socket->send(packet->buffer());
    };
    _socket->send((Buffer::Ptr)nullptr, 1);
}

bool JT1078Connection::addJt1078Info(const string& key, const JT1078Info& info)
{
    logInfo << "add info, key: " << key;
    lock_guard<mutex> lck(_lck);
    auto res = _mapJt1078Info.emplace(key, info);

    return res.second;
}

JT1078Info JT1078Connection::getJt1078Info(const string& key)
{
    logInfo << "get info, key: " << key;
    lock_guard<mutex> lck(_lck);
    JT1078Info info;
    if (_mapJt1078Info.find(key) == _mapJt1078Info.end()) {
        return info;
    }

    return _mapJt1078Info[key];
}

void JT1078Connection::delJt1078Info(const string& key)
{
    logInfo << "del info, key: " << key;
    lock_guard<mutex> lck(_lck);
    _mapJt1078Info.erase(key);
}

bool JT1078Connection::addTalkInfo(const string& key, const JT1078TalkInfo& info)
{
    logInfo << "add info, key: " << key;
    lock_guard<mutex> lck(_talkLck);
    auto res = _mapTalkInfo.emplace(key, info);

    return res.second;
}

JT1078TalkInfo JT1078Connection::getTalkInfo(const string& key)
{
    logInfo << "get info, key: " << key;
    lock_guard<mutex> lck(_talkLck);
    JT1078TalkInfo info;
    if (_mapTalkInfo.find(key) == _mapTalkInfo.end()) {
        return info;
    }

    return _mapTalkInfo[key];
}

void JT1078Connection::delTalkInfo(const string& key)
{
    logInfo << "del info, key: " << key;
    lock_guard<mutex> lck(_talkLck);
    _mapTalkInfo.erase(key);
}

