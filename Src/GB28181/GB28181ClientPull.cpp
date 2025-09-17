#include "GB28181ClientPull.h"
#include "Logger.h"
#include "GB28181Connection.h"
#include "Common/Define.h"

using namespace std;

GB28181ClientPull::GB28181ClientPull(const string& app, const string& stream, int ssrc, int sockType)
    :GB28181Client(MediaClientType_Pull, app, stream, ssrc, sockType)
    ,_sockType(sockType)
    ,_ssrc(ssrc)
    ,_streamName(stream)
    ,_appName(app)
{
}

GB28181ClientPull::~GB28181ClientPull()
{
    
}


void GB28181ClientPull::onRead(const StreamBuffer::Ptr& buffer)
{
    if (_sockType == SOCKET_UDP) {
        if (!_context) {
            string uri = _appName + "/" + _streamName;
            _context = make_shared<GB28181Context>(getLoop(), uri, DEFAULT_VHOST, PROTOCOL_GB28181, DEFAULT_TYPE);
            if (!_context->init()) {
                _context = nullptr;
                stop();
                return ;
            }
        } else if (!_context->isAlive()) {
            stop();
            return ;
        }
        RtpPacket::Ptr rtp = make_shared<RtpPacket>(buffer, 0);
        _context->onRtpPacket(rtp);
    } else {
        _parser.parse(buffer->data(), buffer->size());
    }
}

void GB28181ClientPull::doPull()
{
    weak_ptr<GB28181ClientPull> wSelf = static_pointer_cast<GB28181ClientPull>(shared_from_this());
    _parser.setOnRtpPacket([wSelf](const StreamBuffer::Ptr& buffer){
        auto self = wSelf.lock();
        if(!self){
            return;
        }

        if (!self->_context) {
            string uri = self->_appName + "/" + self->_streamName;
            self->_context = make_shared<GB28181Context>(self->getLoop(), uri, DEFAULT_VHOST, PROTOCOL_GB28181, DEFAULT_TYPE);
            if (!self->_context->init()) {
                self->_context = nullptr;
                self->stop();
                return ;
            }
        } else if (!self->_context->isAlive()) {
            self->stop();
            return ;
        }

        // auto buffer = StreamBuffer::create();
        // buffer->assign(data + 2, len - 2);
        RtpPacket::Ptr rtp = make_shared<RtpPacket>(buffer, 0);
        self->_context->onRtpPacket(rtp);
    });
}