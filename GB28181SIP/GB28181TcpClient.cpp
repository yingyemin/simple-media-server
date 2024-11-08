/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2020 Lixin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "GB28181TcpClient.h"
#include "Log/Logger.h"
#include "EventPoller/EventLoopPool.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>  
#include <map>

using namespace std;

GB28181TcpClient::GB28181TcpClient()
    :TcpClient(EventLoopPool::instance()->getLoopByCircle())
{
    GB28181Client::_loop = getLoop();
}

GB28181TcpClient::~GB28181TcpClient()
{}

void GB28181TcpClient::start()
{
    weak_ptr<GB28181TcpClient> wSelf = static_pointer_cast<GB28181TcpClient>(shared_from_this());
    _parser.setOnGB28181Packet([wSelf](const char* data, int len){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }
        self->onGB28181Packet(data, len);
    });

    //get local config
    string localIp = "0.0.0.0";

    // create(_req->peer_ip, _req->peer_port);
    if (TcpClient::create(localIp, 0) < 0) {
        close();
        logInfo << "TcpClient::create failed: " << strerror(errno);
        return ;
    }

    int timeout = 15;
    if (TcpClient::connect(_req->peer_ip, _req->peer_port, timeout) < 0) {
        close();
        logInfo << "TcpClient::connect, ip: " << _req->peer_ip << ", peerPort: " 
                << _req->peer_port << ", failed: " << strerror(errno);
        return ;
    }

    // _req->host_port = getSocket()->getLocalPort();

    // if (_req->host_port != 0) {
    //     startConnect();
    // }
}

void GB28181TcpClient::sendMessage(const string& message)
{
    logInfo << "send a message: " << message << endl;

    StringBuffer::Ptr buffer(new StringBuffer(message));
    send(buffer);
}

void GB28181TcpClient::addTimerTask()
{
    weak_ptr<GB28181TcpClient> wSelf = dynamic_pointer_cast<GB28181TcpClient>(shared_from_this());

    GB28181Client::_loop->addTimerTask(5000, [wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return 0;
        }
        if (self->_aliveStatus != 0) {
            self->_aliveStatus = 0;
            self->gbRegister(self->_req);
            return 0;
        }
        self->_aliveStatus = 1;
        self->keepalive();
        return 5000;
    }, nullptr);
}

void GB28181TcpClient::onError(const string &ex) {
    //连接失败
    //TODO 重试以及日志上报
}

void GB28181TcpClient::onConnect() {
    // if (err) {
    //     //连接失败
    //     //TODO 重试以及日志上报
    //     return;
    // }
    _req->host_port = getSocket()->getLocalPort();
    //推流器不需要多大的接收缓存，节省内存占用
    //_sock->setReadBuffer(std::make_shared<BufferRaw>(1 * 1024));
    gbRegister(_req);
}

void GB28181TcpClient::onRead(const StreamBuffer::Ptr &buf, struct sockaddr* addr, int len){
    try {
        // input(buf->data(), buf->size());
        _parser.parse(buf->data(), buf->size());
    } catch (exception &e) {
        //连接失败
        //TODO 重试以及日志上报
    }
}

void GB28181TcpClient::onGB28181Packet(const char *data, uint64_t len) {
    logTrace << "message  is: " << string(data, len);
    shared_ptr<SipRequest> req;
    _sipStack.parse_request(req, data, len);
    if (!req) {
        logInfo << "parse request error: " << string(data, len);
        return ;
    }

    // _req->content = string(data,len);
    onWholeSipPacket(req);
}

