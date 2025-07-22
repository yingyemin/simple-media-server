#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "Ehome5Connection.h"
#include "Rtp/RtpManager.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"

using namespace std;

Ehome5Connection::Ehome5Connection(const EventLoop::Ptr& loop, const Socket::Ptr& socket)
    :TcpConnection(loop, socket)
    ,_loop(loop)
    ,_socket(socket)
{
    logInfo << "Ehome5Connection";
}

Ehome5Connection::~Ehome5Connection()
{
    logInfo << "~Ehome5Connection";
    // if (!_ssrc.empty()) {
    //     RtpManager::instance()->delContext(_ssrc);
    // }
    auto psSrc = _source.lock();
    if (psSrc) {
        psSrc->release();
        psSrc->delConnection(this);
    }
}

void Ehome5Connection::init()
{
    weak_ptr<Ehome5Connection> wSelf = static_pointer_cast<Ehome5Connection>(shared_from_this());
    _parser.setOnEhomePacket([wSelf](const char* data, int len){
        auto self = wSelf.lock();
        if(!self){
            return;
        }
        // auto buffer = StreamBuffer::create();
        // buffer->assign(data + 4, len - 4);
        // RtpPacket::Ptr rtp = make_shared<RtpPacket>(buffer, 0);
        self->onEhomePacket(data + 4, len - 4);
    });
}

void Ehome5Connection::close()
{
    logInfo << "Ehome5Connection::close()";
    TcpConnection::close();
}

void Ehome5Connection::onManager()
{
    logInfo << "manager";
}

void Ehome5Connection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    // logInfo << "get a buf: " << buffer->size();
    _parser.parse(buffer->data(), buffer->size());
}

void Ehome5Connection::onError(const string& msg)
{
    close();
    logWarn << "get a error: " << msg;
}

ssize_t Ehome5Connection::send(Buffer::Ptr pkt)
{
    logInfo << "pkt size: " << pkt->size();
    return TcpConnection::send(pkt);
}

void Ehome5Connection::onEhomePacket(const char* data, int len)
{
    if (_packetSeq == 0) {
        ++_packetSeq;
        getSSRC(data, len);
        return ;
    } else if (_packetSeq == 1) {
        ++_packetSeq;
        getCodec(data, len);
        return ;
    }

    auto buffer = StreamBuffer::create();
    buffer->assign(data + 4, len - 4);

    if (_payloadType == "ps") {
        auto frameBuffer = make_shared<FrameBuffer>();
        frameBuffer->_buffer.assign(data + 4, len - 4);

        auto psSource = _source.lock();
        if (!psSource) {
            _localUrlParser.path_ = "/live/" + _ssrc;
            _localUrlParser.vhost_ = DEFAULT_VHOST;
            _localUrlParser.protocol_ = PROTOCOL_EHOME5;
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
            logInfo << "create a TsMediaSource";
            psSource = dynamic_pointer_cast<PsMediaSource>(source);
            psSource->setOrigin();
            psSource->setOriginSocket(_socket);
            psSource->setAction(true);

            auto psDemuxer = make_shared<PsDemuxer>();

            psSource->addTrack(psDemuxer);
            _source = psSource;
        }

        psSource->inputPs(frameBuffer);

        return ;
    }
    
    RtpPacket::Ptr rtp = make_shared<RtpPacket>(buffer, 0);
    if (!_context)
    {
        string uri = "/live/" + _ssrc;
        _context = make_shared<RtpContext>(_loop, uri, DEFAULT_VHOST, PROTOCOL_EHOME5, DEFAULT_TYPE);
        _context->setPayloadType(_payloadType);

        if (_payloadType == "nalu") {
            if (!_videoCodec.empty()) {
                _context->createVideoTrack(_videoCodec);
            }
            if (!_audioCodec.empty()) {
                _context->createAudioTrack(_audioCodec, _channel, _sampleBit, _sampleRate);
            }
        }

        if (!_context->init()) {
            _context = nullptr;
            close();
            return ;
        }

        // RtpManager::instance()->addContext(_ssrc, _context);
    } else if (!_context->isAlive()) {
        close();
        return ;
    }

    _context->onRtpPacket(rtp);
}

bool Ehome5Connection::getSSRC(const char *data,int data_len)
{
    if (data_len < 12) {
        return false;
    }
    
    int i = 0;
    for (; (i < data_len) && (data[i] != 0x00); ++i) {
        _ssrc += (data[i]);
    }
    logInfo << "get ssrc: " << _ssrc << endl;
    return true;
}

void Ehome5Connection::getCodec(const char *data,int data_len)
{
    if (data_len < 20) {
        return ;
    }
    uint8_t* payloadType = (uint8_t*) (data + 12);
    uint16_t *video_ptr = (uint16_t *) (data + 14);
    uint16_t *audio_ptr = (uint16_t *) (data + 16);

    if (*payloadType == 2) {
        _payloadType = "ps";
    } else if (*payloadType == 4) {
        _payloadType = "nalu";
    }

    logInfo << "audio_ptr: " << *audio_ptr << endl;
    logInfo << "video_ptr: " << *video_ptr << endl;
    if (*audio_ptr == 0x7111) {
        _audioCodec = "g711a";
    } else if (*audio_ptr == 0x2001) {
        _audioCodec = "aac";
    } else if (*audio_ptr == 0x7110) {
        _audioCodec = "g711u";
    }
    if (*video_ptr == 0x0100) {
        _videoCodec = "h264";
    } else if (*video_ptr == 0x0005) {
        _videoCodec = "h265";
    }

    _channel = data[18];
    _sampleBit = data[19];
    _sampleRate = *((uint16_t *) (data + 20));

    logInfo << "_channel: " << _channel << endl;
    logInfo << "_sampleBit: " << _sampleBit << endl;
    logInfo << "_sampleRate: " << _sampleRate << endl;
}
