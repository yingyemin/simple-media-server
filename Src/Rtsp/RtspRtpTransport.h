#ifndef RtspRtpTransport_H
#define RtspRtpTransport_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>
#include <deque>

#include "Net/Socket.h"
#include "RtspTrack.h"
#include "Rtp/RtpSort.h"

using namespace std;

enum TransportType {
    Transport_Invalide = -1,
    Transport_TCP = 0,
    Transport_UDP = 1,
    Transport_MULTICAST = 2,
};

enum TransportDataType {
    TransportData_Invalide = -1,
    TransportData_Media = 0, //rtp
    TransportData_Data = 1   //rtcp
};

class RtspRtpTransport : public enable_shared_from_this<RtspRtpTransport>
{
public:
    using Ptr = shared_ptr<RtspRtpTransport>;
    using Wptr = weak_ptr<RtspRtpTransport>;

    RtspRtpTransport(int transType, int dataType, const RtspTrack::Ptr& track, const Socket::Ptr& socket);

public:
    void start();
    void onRtpPacket(const StreamBuffer::Ptr& buffer);
    void sendRtpPacket(const shared_ptr<deque<RtpPacket::Ptr>> &pkt);
    void bindPeerAddr(struct sockaddr* addr);
    Socket::Ptr getSocket() {return _socket;}

private:
    int _transType; //tcp or udp
    int _trackType; //video or audio
    int _index; //track index
    int _dataType;
    RtpSort::Ptr _sort;
    Socket::Ptr _socket;
    RtspTrack::Ptr _track;
};


#endif //RtspRtpTransport_H
