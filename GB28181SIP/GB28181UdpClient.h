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

#ifndef GB28181_UDP_CLIENT_H
#define GB28181_UDP_CLIENT_H

#include <string>
#include <vector>
#include <sstream>
#include <memory>

#include "GB28181Client.h"
#include "Net/Socket.h"

using namespace std;

// The gb28181 client.
class GB28181UdpClient : public GB28181Client, public enable_shared_from_this<GB28181UdpClient>
{
public:
    GB28181UdpClient();
    ~GB28181UdpClient();
public:
    void start() override;
    void sendMessage(const string& message) override;
    void addTimerTask() override;

private:
    Socket::Ptr _pSock;
};

#endif //GB28181_UDP_CLIENT_H

