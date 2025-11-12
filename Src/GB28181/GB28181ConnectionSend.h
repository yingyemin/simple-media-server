#ifndef GB28181ConnectionSend_H
#define GB28181ConnectionSend_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Net/TcpConnection.h"
#include "Rtp/RtpPacket.h"
#include "GB28181MediaSource.h"
#include "Common/UrlParser.h"


// using namespace std;

class GB28181ConnectionSend : public TcpConnection
{
public:
    using Ptr = std::shared_ptr<GB28181ConnectionSend>;
    using Wptr = std::weak_ptr<GB28181ConnectionSend>;

    GB28181ConnectionSend(const EventLoop::Ptr& loop, const Socket::Ptr& socket);
    GB28181ConnectionSend(const EventLoop::Ptr& loop, const Socket::Ptr& socket, int transType);
    ~GB28181ConnectionSend();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError(const std::string& msg) override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;
    void initReader();
    void setOndetach(const std::function<void()>& cb) {_ondetach = cb;}

    void setMediaInfo(const std::string& app, const std::string& stream, int ssrc);
    void sendRtpPacket(const GB28181MediaSource::RingDataType &pack);

private:
    int _transType = 1; //1:tcp server;2:udp server;3:tcp client;4:udp client
    int _ssrc = -1;
    std::string _app;
    std::string _stream;
    UrlParser _urlParser;
    std::shared_ptr<sockaddr> _addr;
    std::shared_ptr<TimerTask> _task;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    GB28181MediaSource::Wptr _source;
    GB28181MediaSource::RingType::DataQueReaderT::Ptr _playReader;
    std::function<void()> _ondetach;
};


#endif //GB28181Connection_H
