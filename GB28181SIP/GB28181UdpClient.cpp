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

#include "GB28181UdpClient.h"
#include "Log/Logger.h"
#include "EventPoller/EventLoopPool.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>  
#include <map>
#include <arpa/inet.h>

using namespace std;

GB28181UdpClient::GB28181UdpClient()
{}

GB28181UdpClient::~GB28181UdpClient()
{}

void GB28181UdpClient::start()
{
    //get local config
    string localIp = "0.0.0.0";
    _loop = EventLoopPool::instance()->getLoopByCircle();
    _pSock = std::make_shared<Socket>(_loop);
    _pSock->createSocket(SOCKET_UDP);
    if (_pSock->bind(0, localIp.data()) < 0) {
        //分配端口失败
        throw runtime_error("open udp socket failed");
    }
    logInfo << "bind port is: " << _pSock->getLocalPort() << endl;
    _req->host_port = _pSock->getLocalPort();
    _pSock->setRecvBuf(14 * 1024 * 1024);
    struct sockaddr_in peerAddr;
    //设置rtp发送目标地址
    peerAddr.sin_family = AF_INET;
    peerAddr.sin_port = htons(_req->peer_port);
    peerAddr.sin_addr.s_addr = inet_addr(_req->peer_ip.data());
    bzero(&(peerAddr.sin_zero), sizeof peerAddr.sin_zero);
    _pSock->bindPeerAddr((struct sockaddr *)(&peerAddr));

    logTrace << "send to ip: " << _req->peer_ip << "; port: " << _req->peer_port << endl;

    weak_ptr<GB28181UdpClient> wSelf = dynamic_pointer_cast<GB28181UdpClient>(shared_from_this());
    _pSock->setReadCb([wSelf](const StreamBuffer::Ptr &buf, struct sockaddr *addr, int) {
        auto self = wSelf.lock();
        if (!self) {
            return -1;
        }
        logInfo << "get a message: " << buf->data() << endl;
        shared_ptr<SipRequest> req;
        int err = self->_sipStack.parse_request(req, buf->data(), buf->size());
        if (err != 0 || !req) {
            logError << "parse req error, err = " << err << endl;
            return -1;
        }
        self->onWholeSipPacket(req);

        return 0;
    });

    _pSock->addToEpoll();

    gbRegister(_req);
}

void GB28181UdpClient::sendMessage(const string& message)
{
    logInfo << "send a message: " << message << endl;
    StringBuffer::Ptr buffer(new StringBuffer(message));
    _pSock->send(buffer);
}

void GB28181UdpClient::addTimerTask()
{
    weak_ptr<GB28181UdpClient> wSelf = shared_from_this();
    
    _loop->addTimerTask(5000, [wSelf](){
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

