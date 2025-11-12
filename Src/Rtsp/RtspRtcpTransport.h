#ifndef RtspRtcpTransport_H
#define RtspRtcpTransport_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Socket.h"
#include "RtspTrack.h"
#include "Rtp/RtpSort.h"

// using namespace std;

class RtspRtcpTransport : public std::enable_shared_from_this<RtspRtcpTransport>
{
public:
    using Ptr = std::shared_ptr<RtspRtcpTransport>;
    using Wptr = std::weak_ptr<RtspRtcpTransport>;

    RtspRtcpTransport(int transType, int dataType, const RtspTrack::Ptr& track, const Socket::Ptr& socket);

public:
    void start();
    void onSendRtp(const RtpPacket::Ptr& rtp);
    void onRtcpPacket(const StreamBuffer::Ptr& buffer);
    void sendRtcpPacket();
    void bindPeerAddr(struct sockaddr* addr);
    Socket::Ptr getSocket() {return _socket;}    
    void setOnTcpSend(const std::function<void(const Buffer::Ptr& pkt, int flag, size_t offset, size_t len)>& func) {_tcpSend = func;}

private:
    int _transType; //tcp or udp
    int _trackType; //video or audio
    int _index; //track index
    int _dataType;
    uint64_t _sendSize = 0;
    uint64_t _lastStamp = 0;
    uint64_t _sendCount = 0;
    RtpSort::Ptr _sort;
    Socket::Ptr _socket;
    RtspTrack::Ptr _track;
    std::function<void(const Buffer::Ptr& pkt, int flag, size_t offset, size_t len)> _tcpSend;
};


#endif //RtspRtcpTransport_H
