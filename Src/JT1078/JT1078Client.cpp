#include "JT1078Client.h"
#include "Logger.h"
#include "Common/Config.h"
#include "Util/String.h"
#include "Common/Define.h"
#include "Hook/MediaHook.h"

using namespace std;

JT1078Client::JT1078Client(MediaClientType type, const string& appName, const string& streamName)
    :TcpClient(EventLoop::getCurrentLoop())
    ,_type(type)
{
    _localUrlParser.path_ = "/" + appName + "/" + streamName;
    _localUrlParser.protocol_ = PROTOCOL_JT1078;
    _localUrlParser.type_ = DEFAULT_TYPE;
    _localUrlParser.vhost_ = DEFAULT_VHOST;
}

JT1078Client::~JT1078Client()
{
    auto jtSrc = _source.lock();
    if (jtSrc) {
        jtSrc->delConnection(this);
    }
}

void JT1078Client::start(const string& localIp, int localPort, const string& url, int timeout)
{
    if (!timeout) {
        timeout = 5;
    }
    weak_ptr<JT1078Client> wSelf = dynamic_pointer_cast<JT1078Client>(shared_from_this());
    logInfo << "localIp: " << localIp << ", localPort: " << localPort;
    if (TcpClient::create(localIp, localPort) < 0) {
        close();
        logInfo << "TcpClient::create failed: " << strerror(errno);
        return ;
    }

    _peerUrlParser.parse(url);
    logInfo << "_peerUrlParser.host_: " << _peerUrlParser.host_ << ", _peerUrlParser.port_: " << _peerUrlParser.port_
            << ", timeout: " << timeout;
    
    if (_peerUrlParser.port_ == 0 || _peerUrlParser.host_.empty()) {
        logInfo << "peer ip: " << _peerUrlParser.host_ << ", peerPort: " 
                << _peerUrlParser.port_ << ", failed: is empty";
        return ;
    }
    if (TcpClient::connect(_peerUrlParser.host_, _peerUrlParser.port_, timeout) < 0) {
        close();
        logInfo << "TcpClient::connect, ip: " << _peerUrlParser.host_ << ", peerPort: " 
                << _peerUrlParser.port_ << ", failed: " << strerror(errno);
        return ;
    }
}

void JT1078Client::stop()
{
    close();
}

void JT1078Client::pause()
{

}

void JT1078Client::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    // logInfo << "get a buffer: " << buffer->size();
    // _parser.parse(buffer->data(), buffer->size());
}

void JT1078Client::close()
{
    if (_onClose) {
        _onClose();
    }

    TcpClient::close();
}

void JT1078Client::onConnect()
{
    _socket = TcpClient::getSocket();
    _loop = _socket->getLoop();

    weak_ptr<JT1078Client> wSelf = dynamic_pointer_cast<JT1078Client>(shared_from_this());
    MediaSource::getOrCreateAsync(_localUrlParser.path_, _localUrlParser.vhost_, _localUrlParser.protocol_, _localUrlParser.type_, 
    [wSelf](const MediaSource::Ptr &src){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        self->_loop->async([wSelf, src](){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            self->startSendTalkData(dynamic_pointer_cast<JT1078MediaSource>(src));
        }, true);
    }, 
    [wSelf]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }

        auto source = make_shared<JT1078MediaSource>(self->_localUrlParser, nullptr, true);
        source->setSimCode(self->_simCode);
        source->setChannel(self->_channel);

        return source;
    }, this);
}

void JT1078Client::onError(const string& err)
{
    logInfo << "get a error: " << err;

    close();
}

void JT1078Client::setOnClose(const function<void()>& cb)
{
    _onClose = cb;
}

void JT1078Client::startSendTalkData(const JT1078MediaSource::Ptr &jtSrc)
{
    if (!jtSrc) {
        return ;
    }

    _source = jtSrc;

    if (!_playReader) {
        logInfo << "set _playReader";
        weak_ptr<JT1078Client> wSelf = dynamic_pointer_cast<JT1078Client>(shared_from_this());
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
        info.type = _localUrlParser.type_;
        info.uri = _localUrlParser.path_;
        info.vhost = _localUrlParser.vhost_;

        MediaHook::instance()->onPlayer(info);
    }
}

void JT1078Client::sendRtpPacket(const JT1078MediaSource::RingDataType &pkt)
{
    logInfo << "JT1078Client::sendRtpPacket";
    for (auto it = pkt->begin(); it != pkt->end(); ++it) {
        auto packet = it->get();
        logInfo << "packet size: " << packet->size();
        _socket->send(packet->buffer(), 0);
    };
    _socket->send((Buffer::Ptr)nullptr, 1);
}