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
#include "Util/logger.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>  
#include <map>

using namespace std;

namespace mediakit{

GB28181TcpClient::GB28181TcpClient()
{}

GB28181TcpClient::~GB28181TcpClient()
{}

void GB28181TcpClient::start()
{
    //get local config
    string localIp = "0.0.0.0";

    create(_req->peer_ip, _req->peer_port);

    _req->host_port = get_local_port();

    if (_req->host_port != 0) {
        startConnect();
    }
}

void GB28181TcpClient::sendMessage(const string& message)
{
    BufferString::Ptr buffer(new BufferString(message, 0, message.size()));
    send(buffer);
}

void GB28181TcpClient::onErr(const SockException &ex) {
    //连接失败
    //TODO 重试以及日志上报
}

void GB28181TcpClient::onConnect(const SockException &err) {
    if (err) {
        //连接失败
        //TODO 重试以及日志上报
        return;
    }
    //推流器不需要多大的接收缓存，节省内存占用
    //_sock->setReadBuffer(std::make_shared<BufferRaw>(1 * 1024));
    gbRegister(_req);
}

void GB28181TcpClient::onRecv(const Buffer::Ptr &buf){
    try {
        input(buf->data(), buf->size());
    } catch (exception &e) {
        SockException ex(Err_other, e.what());
        //连接失败
        //TODO 重试以及日志上报
    }
}

int64_t GB28181TcpClient::onRecvHeader(const char *data, uint64_t len) {
    TraceL << "get message header: " << data << endl;
    _sipStack.parse_request(_req, data, len);
    if (!_req) {
        return 0;
    }

    auto ret = _req->content_length;
    if(ret == 0){
        onWholeSipPacket(_req);
    }
    return ret;
}

void GB28181TcpClient::onRecvContent(const char *data, uint64_t len) {
    TraceL << "message body is: " << endl;
    _req->content = string(data,len);
    onWholeSipPacket(_req);
}

}//namespace mediakit

