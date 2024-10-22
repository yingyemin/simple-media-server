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
        parser.type_ = DEFAULT_TYPE;
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

