#ifndef Ehome5Connection_H
#define Ehome5Connection_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Net/TcpConnection.h"
#include "Rtp/RtpPacket.h"
#include "Ehome5Parser.h"
#include "Rtp/RtpContext.h"


using namespace std;

class Ehome5Connection : public TcpConnection
{
public:
    using Ptr = shared_ptr<Ehome5Connection>;
    using Wptr = weak_ptr<Ehome5Connection>;

    Ehome5Connection(const EventLoop::Ptr& loop, const Socket::Ptr& socket);
    ~Ehome5Connection();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError(const string& msg) override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;

    void onEhomePacket(const char* data, int len);

private:
    bool getSSRC(const char *data,int data_len);
    void getCodec(const char *data,int data_len);

private:
    // int _ssrc = -1;
    uint64_t _packetSeq = 0;
    uint64_t _channel = 0;
    uint64_t _sampleBit = 0;
    uint64_t _sampleRate = 0;

    std::string _ssrc;
    std::string _videoCodec;
    std::string _audioCodec;
    std::string _payloadType = "ps";

    Ehome5Parser _parser;
    RtpContext::Ptr _context;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
};


#endif //Ehome5Connection_H
