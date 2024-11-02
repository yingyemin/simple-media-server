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

#ifndef GB28181_CLIENT_H
#define GB28181_CLIENT_H

#include <string>
#include <vector>
#include <sstream>
#include <memory>

#include "SipMessage.h"
#include "EventPoller/EventLoop.h"

using namespace std;

class CatalogInfo
{
public:
    string _sn;
    int    _total;
    int    _channelNum;
    string _channelStartId;
    string _channelEndId = "1310000001";
};

// The gb28181 client.
class GB28181Client : public enable_shared_from_this<GB28181Client>
{
public:
    GB28181Client();
    ~GB28181Client();
public:
    virtual void start() = 0;
    virtual void sendMessage(const string& message) = 0;
    void gbRegister(shared_ptr<SipRequest> req);
    void keepalive();
    string sendDevice(const string& callId, CatalogInfo& info);
    void catalog(shared_ptr<SipRequest> req);
    void sendDeviceInfo(shared_ptr<SipRequest> req);
    void sendDeviceStatus(shared_ptr<SipRequest> req);
    int isRegister() {return _registerStatus;}

    void onWholeSipPacket(shared_ptr<SipRequest> req);

protected:
    int    _channelNum;
    string _channelStartId;
    string _channelEndId = "1310000001";
    int _aliveStatus = 0;
    int _registerStatus = 0;

    map<string, map<string, shared_ptr<SipRequest>>> _channel2Req;

    SipStack _sipStack;
    shared_ptr<SipRequest> _req = NULL;
    map<string, map<string, Timer::Ptr>> _channel2Timer;
    EventLoop::Ptr _loop;
    map<string, CatalogInfo> _callid2Catalog;
    char _buf[1024 * 1024];
};

#endif //GB28181_CLIENT_H

