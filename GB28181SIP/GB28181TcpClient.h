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

#ifndef GB28181_TCP_CLIENT_H
#define GB28181_TCP_CLIENT_H

#include <string>
#include <vector>
#include <sstream>

#include "GB28181Client.h"
#include "GB28181SIPParser.h"
#include "Net/TcpClient.h"

// using namespace std;

// The gb28181 client.
class GB28181TcpClient : public TcpClient, public GB28181Client
{
public:
    GB28181TcpClient();
    ~GB28181TcpClient();
public:
    //for GB28181Client override
    void start() override;
    void sendMessage(const std::string& message) override;
    void addTimerTask() override;
    
    //for Tcpclient override
    void onRead(const StreamBuffer::Ptr &buf, struct sockaddr* addr, int len) override;
    void onConnect() override;
    void onError(const std::string &ex) override;

    //for HttpRequestSplitter override
    // int64_t onRecvHeader(const char *data,uint64_t len) override;
    void onGB28181Packet(const char *data, uint64_t len);

private:
    GB28181SIPParser _parser;
};

#endif //GB28181_TCP_CLIENT_H

