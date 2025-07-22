﻿#ifndef Ehome2VodConnection_H
#define Ehome2VodConnection_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Net/TcpConnection.h"
#include "Mpeg/PsMediaSource.h"
#include "Common/UrlParser.h"

using namespace std;

class Ehome2VodConnection : public TcpConnection
{
public:
    using Ptr = shared_ptr<Ehome2VodConnection>;
    using Wptr = weak_ptr<Ehome2VodConnection>;

    Ehome2VodConnection(const EventLoop::Ptr& loop, const Socket::Ptr& socket, 
        const string& streamId, int timeout);
    ~Ehome2VodConnection();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError(const string& msg) override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;

    void setOnClose(const function<void()>& cb) {_onClose = cb;}

private:
    int _ssrc = -1;
    int _timeout = 5000;
    string _streamId;
    UrlParser _localUrlParser;
    TimeClock _timeClock;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    PsMediaSource::Wptr _source;
    function<void()> _onClose;
};


#endif //Ehome2VodConnection_H
