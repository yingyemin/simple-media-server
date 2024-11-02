#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "SrtConnection.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"
#include "EventPoller/EventLoopPool.h"

using namespace std;

#ifdef ENABLE_SRT

static unordered_map<string, string> parseSid(char *sid, int len)
{
    string strSid(sid, len);

    auto vecParam = split(strSid, "|", ":");

    return vecParam;
}

SrtConnection::SrtConnection(const SrtEventLoop::Ptr& loop, const SrtSocket::Ptr& socket)
    :_loop(loop)
    ,_socket(socket)
{
    logInfo << "SrtConnection";
}

SrtConnection::~SrtConnection()
{
    auto tsSrc = _source.lock();
    if (_request == "push" && tsSrc) {
        auto loop = tsSrc->getLoop();
        if (loop) {
            loop->async([tsSrc](){
                tsSrc->release();
            }, true);
        }
        // _source->delOnDetach(this);
    } else if (tsSrc) {
        logInfo << "delConnection(this)";
        tsSrc->delConnection(this);
        // _source->delOnDetach(this);
    }
    close();
    logInfo << "~SrtConnection";
}

void SrtConnection::init()
{
    weak_ptr<SrtConnection> wSelf = shared_from_this();
    _socket->setErrorCb([wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

        self->onError("get a err from socket");
    });

    char sid[1024] = {0};
	int  sid_size = sizeof(sid);
    if (_socket->getsockopt(SRTO_STREAMID, "SRTO_STREAMID", &sid, &sid_size) != 0) {
        logError << "get streamid info failed";
        close();
        return ;
    }

    if (_mapParam.size() == 0) {
        _mapParam = parseSid(sid, sid_size);

        if (_mapParam.find("path") == _mapParam.end()) {
            logError << "get path failed";
            close();
            return ;
        }

        logInfo << "path is: " << _mapParam["path"];

        _urlParser.path_ = _mapParam["path"];
        _urlParser.protocol_ = PROTOCOL_TS;
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
        } else {
            onError("invalid request: " + _request);
        }
    }
}

void SrtConnection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    // logInfo << "get a buf: " << buffer->size();
    assert(buffer->data()[0] == 0x47);
    auto copyBuffer = StreamBuffer::create();
    copyBuffer->assign(buffer->data(), buffer->size()); 

    // if (_request == "pull") {
    //     handlePull(copyBuffer);
    // } else 
    if (_request == "push") {
        handlePush(copyBuffer);
    } else {
        logWarn << "invalid request: " << _request;
        onError("invalid request: " + _request);
    }
}

void SrtConnection::close()
{
    if (_onClose) {
        _onClose();
        _onClose = nullptr;
    }

    if (_socket) {
        _socket->close();
        _socket = nullptr;
    }
}

void SrtConnection::onError(const string& err)
{
    logInfo << "get a err: " << err;
    close();
}

void SrtConnection::handlePull()
{
    weak_ptr<SrtConnection> wSelf = shared_from_this();
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

void SrtConnection::onPlayTs(const TsMediaSource::Ptr &tsSrc)
{
    weak_ptr<SrtConnection> wSelf = shared_from_this();

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
			ret.protocol_ = PROTOCOL_TS;
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

void SrtConnection::initPush()
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

void SrtConnection::handlePush(const StreamBuffer::Ptr& buffer)
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
