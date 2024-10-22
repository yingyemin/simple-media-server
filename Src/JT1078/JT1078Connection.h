#ifndef JT1078Connection_H
#define JT1078Connection_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Net/TcpConnection.h"
#include "Rtp/RtpPacket.h"
#include "JT1078Parser.h"
#include "JT1078DecodeTrack.h"
#include "JT1078MediaSource.h"

using namespace std;

class JT1078Info
{
public:
    string appName = "live";
    string streamName;
};

class JT1078Connection : public TcpConnection
{
public:
    using Ptr = shared_ptr<JT1078Connection>;
    using Wptr = weak_ptr<JT1078Connection>;

    JT1078Connection(const EventLoop::Ptr& loop, const Socket::Ptr& socket);
    ~JT1078Connection();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError() override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;

    void setPath(const string& path) {_path = path;}
    void onRtpPacket(const JT1078RtpPacket::Ptr& buffer);

    static bool addJt1078Info(const string& key, const JT1078Info& info);
    static JT1078Info getJt1078Info(const string& key);
    static void delJt1078Info(const string& key);

private:
    string _path;
    string _key;
    JT1078Parser _parser;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    JT1078MediaSource::Wptr _source;
    unordered_map<int, JT1078DecodeTrack::Ptr> _mapTrack;

    static mutex _lck;
    static unordered_map<string, JT1078Info> _mapJt1078Info;
};


#endif //JT1078Connection_H
