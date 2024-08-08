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

#ifndef ZLMEDIAKIT_GB28181_TCP_CLIENT_H
#define ZLMEDIAKIT_GB28181_TCP_CLIENT_H

#include <string>
#include <vector>
#include <sstream>

#include "GB28181Client.h"
#include "Network/TcpClient.h"
#include "Http/HttpRequestSplitter.h"

using namespace std;
using namespace toolkit;

namespace mediakit {

// The gb28181 client.
class GB28181TcpClient : public TcpClient, public GB28181Client, public HttpRequestSplitter
{
public:
    GB28181TcpClient();
    ~GB28181TcpClient();
public:
    //for GB28181Client override
    void start() override;
    void sendMessage(const string& message) override;
    
    //for Tcpclient override
    void onRecv(const Buffer::Ptr &buf) override;
    void onConnect(const SockException &err) override;
    void onErr(const SockException &ex) override;

    //for HttpRequestSplitter override
    int64_t onRecvHeader(const char *data,uint64_t len) override;
    void onRecvContent(const char *data,uint64_t len) override;

private:

};

} // namespace mediakit

#endif //ZLMEDIAKIT_GB28181_TCP_CLIENT_H

