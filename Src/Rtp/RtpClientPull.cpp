#include "RtpClientPull.h"
#include "Logger.h"
#include "RtpConnection.h"
#include "Common/Define.h"

using namespace std;

RtpClientPull::RtpClientPull(const string& app, const string& stream, int ssrc, int sockType)
    :RtpClient(MediaClientType_Pull, app, stream, ssrc, sockType)
    ,_sockType(sockType)
    ,_ssrc(ssrc)
    ,_streamName(stream)
    ,_appName(app)
{
}

RtpClientPull::~RtpClientPull()
{
    
}


void RtpClientPull::onRead(const StreamBuffer::Ptr& buffer)
{
    if (_sockType == SOCKET_UDP) {
        if (!_context) {
            string uri = _appName + "/" + _streamName;
            _context = make_shared<RtpContext>(getLoop(), uri, DEFAULT_VHOST, PROTOCOL_RTP, DEFAULT_TYPE);
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

void RtpClientPull::doPull()
{
    weak_ptr<RtpClientPull> wSelf = static_pointer_cast<RtpClientPull>(shared_from_this());
    _parser.setOnRtpPacket([wSelf](const StreamBuffer::Ptr& buffer){
        auto self = wSelf.lock();
        if(!self){
            return;
        }
        // auto buffer = StreamBuffer::create();
        // buffer->assign(data + 2, len - 2);
        RtpPacket::Ptr rtp = make_shared<RtpPacket>(buffer, 0);
        self->_context->onRtpPacket(rtp);
    });
}