#ifndef RtpConnectionSend_H
#define RtpConnectionSend_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Net/TcpConnection.h"
#include "Rtp/RtpPacket.h"
#include "RtpMediaSource.h"
#include "Common/UrlParser.h"


using namespace std;

class RtpConnectionSend : public TcpConnection
{
public:
    using Ptr = shared_ptr<RtpConnectionSend>;
    using Wptr = weak_ptr<RtpConnectionSend>;

    RtpConnectionSend(const EventLoop::Ptr& loop, const Socket::Ptr& socket);
    RtpConnectionSend(const EventLoop::Ptr& loop, const Socket::Ptr& socket, int transType);
    ~RtpConnectionSend();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError() override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;
    void initReader();
    void setOndetach(const function<void()>& cb) {_ondetach = cb;}

    void setMediaInfo(const string& app, const string& stream, int ssrc);
    void sendRtpPacket(const RtpMediaSource::RingDataType &pack);

private:
    int _transType = 1; //1:tcp server;2:udp server;3:tcp client;4:udp client
    int _ssrc = -1;
    string _app;
    string _stream;
    UrlParser _urlParser;
    shared_ptr<sockaddr> _addr;
    shared_ptr<TimerTask> _task;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    RtpMediaSource::Wptr _source;
    RtpMediaSource::RingType::DataQueReaderT::Ptr _playReader;
    function<void()> _ondetach;
};


#endif //RtpConnection_H
