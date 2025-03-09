#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "SrtClient.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"
#include "EventPoller/EventLoopPool.h"

using namespace std;

#ifdef ENABLE_SRT

SrtClient::SrtClient(MediaClientType type, const string& appName, const string& streamName)
{
    logInfo << "SrtClient";
    _request = (type == MediaClientType_Push ? "push" : "pull");
    _urlParser.path_ = "/" + appName + "/" + streamName;
    _urlParser.protocol_ = PROTOCOL_SRT;
    _urlParser.vhost_ = DEFAULT_VHOST;
    _urlParser.type_ = DEFAULT_TYPE;
}

SrtClient::~SrtClient()
{
    logInfo << "~SrtClient";
    auto srtSrc = _source.lock();
    if (_request == "pull" && srtSrc) {
        srtSrc->delConnection(this);
        srtSrc->release();
    } else if (srtSrc) {
        srtSrc->delConnection(this);
    }
}

void SrtClient::getProtocolAndType(string& protocol, MediaClientType& type)
{
    protocol = _urlParser.protocol_;
    type = _request == "pull" ? MediaClientType_Pull : MediaClientType_Push;
}

bool SrtClient::start(const string& localIp, int localPort, const string& url, int timeout)
{
    _url = url;
    _peerUrlParser.parse(url);
    _loop = SrtEventLoopPool::instance()->getLoopByCircle();

    _socket = make_shared<SrtSocket>(_loop, false);
    _socket->createSocket(0);
    int ret = _socket->setsockopt(_socket->getFd(), SOL_SOCKET, SRTO_STREAMID, _peerUrlParser.param_.data(), _peerUrlParser.param_.size());
    if (ret == -1) {
        cout << "setsockopt SO_RCVBUF failed";
        return false;
    }

    if (_socket->connect(_peerUrlParser.host_, _peerUrlParser.port_, timeout) != 0) {
        close();
        logInfo << "TcpClient::connect, ip: " << _peerUrlParser.host_ << ", peerPort: " 
                << _peerUrlParser.port_ << ", failed";

        return false;
    }

    weak_ptr<SrtClient> wSelf = shared_from_this();
    _socket->setErrorCb([wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        self->onError("get a err from socket");
    });

    _socket->setAcceptCb([wSelf](){
        logInfo << "get a new connection from socket";
        
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        auto acceptFd = self->_socket->accept();
        auto acceptSocket = make_shared<SrtSocket>(self->_socket->getLoop(), acceptFd);
        acceptSocket->addToEpoll();

        acceptSocket->setReadCb([wSelf](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
            auto self = wSelf.lock();
            if (!self) {
                return 0;
            }
            logInfo << "get a buffer, size: " << buffer->size();
            return 0;
        });
        self->_acceptSocket = acceptSocket;
    });

    if (_request == "push") {
        handlePush();
    } else if (_request == "pull") {
        initPull();
    } else {
        onError("invalid request: " + _request);
        return false;
    }

    return true;
}

void SrtClient::stop()
{
    close();
}

void SrtClient::pause()
{

}

void SrtClient::setOnClose(const function<void()>& cb)
{
    _onClose = cb;
}

void SrtClient::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    // logInfo << "get a buf: " << buffer->size();
    assert(buffer->data()[0] == 0x47);
    auto copyBuffer = StreamBuffer::create();
    copyBuffer->assign(buffer->data(), buffer->size()); 

    // if (_request == "pull") {
    //     handlePull(copyBuffer);
    // } else 
    if (_request == "pull") {
        handlePull(copyBuffer);
    } else {
        logWarn << "invalid request: " << _request;
        onError("invalid request: " + _request);
    }
}

void SrtClient::close()
{
    if (_socket) {
        _socket->close();
        _socket = nullptr;
    }

    if (_onClose) {
        _onClose();
    }
}

void SrtClient::onError(const string& err)
{
    logInfo << "get a err: " << err;
    close();
}

void SrtClient::handlePush()
{
    weak_ptr<SrtClient> wSelf = shared_from_this();
    MediaSource::getOrCreateAsync(_urlParser.path_, _urlParser.vhost_, _urlParser.protocol_, _urlParser.type_, 
    [wSelf](const MediaSource::Ptr &src){
        logInfo << "get a src";
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

		auto tsSrc = dynamic_pointer_cast<TsMediaSource>(src);
		if (!tsSrc) {
			self->onError("hls source is not exist");
            return ;
		}

        // self->_source = rtmpSrc;

		self->_loop->async([wSelf, tsSrc](){
			auto self = wSelf.lock();
			if (!self) {
				return ;
			}

			self->onPushTs(tsSrc);
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

void SrtClient::onPushTs(const TsMediaSource::Ptr &tsSrc)
{
    weak_ptr<SrtClient> wSelf = shared_from_this();

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
                self->_socket->send(pkt);
				// uint8_t frame_type = (pkt->payload.get()[0] >> 4) & 0x0f;
				// uint8_t codec_id = pkt->payload.get()[0] & 0x0f;

				// FILE* fp = fopen("testflv.rtmp", "ab+");
				// fwrite(pkt->payload.get(), pkt->length, 1, fp);
				// fclose(fp);

				// logInfo << "frame_type : " << (int)frame_type << ", codec_id: " << (int)codec_id
				// 		<< "pkt->payload.get()[0]: " << (int)(pkt->payload.get()[0]);
				// self->sendMediaData(pkt->type_id, pkt->abs_timestamp, pkt->payload, pkt->length);
			}
		});
	}
}

void SrtClient::initPull()
{
    if (!_inited) {
        auto source = MediaSource::getOrCreate(_urlParser.path_, _urlParser.vhost_
                        , _urlParser.protocol_, _urlParser.type_
                        , [this](){
                            auto loop = EventLoopPool::instance()->getLoopByCircle();
                            return make_shared<TsMediaSource>(_urlParser, loop);
                        });

        if (!source) {
            logWarn << "another stream is exist with the same uri";
            return ;
        }
        logInfo << "create a TsMediaSource";
        auto tsSource = dynamic_pointer_cast<TsMediaSource>(source);
        tsSource->setOrigin();
        tsSource->setAction(false);

        auto tsDemuxer = make_shared<TsDemuxer>();

        tsSource->addTrack(tsDemuxer);
        _source = tsSource;

        weak_ptr<SrtClient> wSelf = shared_from_this();
        _socket->setReadCb([wSelf](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
            auto self = wSelf.lock();
			if (!self/* || pack->empty()*/) {
				return -1;
			}
            self->onRead(buffer, addr, len);
            return 0;
        });

        _inited = true;
    }
}

void SrtClient::handlePull(const StreamBuffer::Ptr& buffer)
{
    auto tsSource = _source.lock();
    if (!tsSource) {
        logWarn << "source is empty";
        close();
        return ;
    }

    tsSource->getLoop()->async([buffer, tsSource](){
        tsSource->inputTs(buffer);
    }, true);
    
}

#endif
