#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "Ehome2VodConnection.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"
#include "Common/Config.h"

using namespace std;

Ehome2VodConnection::Ehome2VodConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket, 
    const string& streamId, int timeout)
    :TcpConnection(loop, socket)
    ,_loop(loop)
    ,_socket(socket)
    ,_streamId(streamId)
    ,_timeout(timeout)
{
    logInfo << "Ehome2VodConnection";
}

Ehome2VodConnection::~Ehome2VodConnection()
{
    logInfo << "~Ehome2VodConnection";
    auto psSrc = _source.lock();
    if (psSrc) {
        psSrc->release();
    }
}

void Ehome2VodConnection::init()
{
}

void Ehome2VodConnection::close()
{
    logInfo << "Ehome2VodConnection::close()";
    TcpConnection::close();
    if (_onClose) {
        _onClose();
    }
}

void Ehome2VodConnection::onManager()
{
    logTrace << "streamId: " << _streamId << ", on manager";
    static int timeout = Config::instance()->getAndListen([](const json& config){
        timeout = Config::instance()->get("Ehome2", "Server", "timeout", "", "5000");
    }, "Ehome2", "Server", "timeout", "", "5000");

    int curTimeout = _timeout == 0 ? timeout : _timeout;

    if (_timeClock.startToNow() > curTimeout) {
        logWarn << _streamId <<  ": timeout";
        weak_ptr<Ehome2VodConnection> wSelf = dynamic_pointer_cast<Ehome2VodConnection>(shared_from_this());
        // 直接close会将tcpserver的map迭代器破坏，用异步接口关闭
        _loop->async([wSelf](){
            auto self = wSelf.lock();
            if (self) {
                self->close();
            }
        }, false);
    }
}

void Ehome2VodConnection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    _timeClock.update();

    auto psSource = _source.lock();
    if (!psSource) {
        _localUrlParser.path_ = _streamId;
        _localUrlParser.vhost_ = DEFAULT_VHOST;
        _localUrlParser.protocol_ = PROTOCOL_EHOME2;
        _localUrlParser.type_ = DEFAULT_TYPE;
        auto source = MediaSource::getOrCreate(_localUrlParser.path_, _localUrlParser.vhost_
                        , _localUrlParser.protocol_, _localUrlParser.type_
                        , [this]() {
                            return make_shared<PsMediaSource>(_localUrlParser, _loop);
                        });

        if (!source) {
            logWarn << "another stream is exist with the same uri";
            return ;
        }
        logInfo << "create a PsMediaSource";
        psSource = dynamic_pointer_cast<PsMediaSource>(source);
        psSource->setOrigin();
        psSource->setOriginSocket(_socket);
        psSource->setAction(true);

        auto psDemuxer = make_shared<PsDemuxer>();

        psSource->addTrack(psDemuxer);
        _source = psSource;
    }

    auto frameBuffer = make_shared<FrameBuffer>();
    frameBuffer->_buffer.assign(buffer->data(), buffer->size());
    psSource->inputPs(frameBuffer);
}

void Ehome2VodConnection::onError(const string& msg)
{
    close();
    logWarn << "get a error: " << msg;
}

ssize_t Ehome2VodConnection::send(Buffer::Ptr pkt)
{
    logInfo << "pkt size: " << pkt->size();
    return TcpConnection::send(pkt);
}
