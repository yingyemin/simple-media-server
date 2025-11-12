/*
 * Copyright (c) 2016 The ZLToolKit project authors. All Rights Reserved.
 *
 * This file is part of ZLToolKit(https://github.com/ZLMediaKit/ZLToolKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include <iostream>
#include <csignal>

#include <memory>
#include <sstream>
#include <TcpClient.h>
#include <Log/Logger.h>
#include <EventLoopPool.h>
#include <Util/semaphore.h>

class TestClient: public TcpClient {
public:
    using Ptr = std::shared_ptr<TestClient>;
    TestClient(const EventLoop::Ptr& loop):TcpClient(loop) {
        logTrace << "TestClient Creator";
    }
    ~TestClient(){
        logTrace << "TestClient Detory";
    }
protected:
    virtual void onRecv(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
    {
        logInfo << buffer->data() << " from ip:"<<getPeerIp()<< "port:" << getPeerPort();
    }
    virtual void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
    {

    }
    virtual void onError(const string& err)
    {
        logInfo << "errer occr:" << err;
    }
    virtual void onWrite()
    {

    }
   /* virtual void close()
    {

    }*/
    virtual void onConnect(){
        logTrace << "connect success";
    }
private:
    int _nTick = 0;
};


int main() {
    // 设置日志系统  [AUTO-TRANSLATED:45646031]
    // Set up the logging system
    Logger::instance()->addChannel(std::make_shared<ConsoleChannel>("ConsoleChannel", LTrace));
    Logger::instance()->setWriter(std::make_shared<AsyncLogWriter>());
    // 多线程开启epoll
    EventLoopPool::instance()->init(0, true, true);

    TestClient::Ptr client(new TestClient(EventLoopPool::instance()->getLoopByCircle()));//必须使用智能指针
    if (client->create("", 8888) < 0) {
        client->close();
        logInfo << "TcpClient::create failed: " << strerror(errno);
        return -1;
    }

    if (client->connect("192.168.1.104", 9000) < 0) {
        client->close();
        logInfo << "TcpClient::connect, ip error";
        return -1;
    }
    ;
    //TcpClientWithSSL<TestClient>::Ptr clientSSL(new TcpClientWithSSL<TestClient>());//必须使用智能指针
    //clientSSL->startConnect("127.0.0.1",9001);//连接服务器

    //退出程序事件处理  [AUTO-TRANSLATED:80065cb7]
    // Exit program event handling
    static semaphore sem;
    signal(SIGINT, [](int) { sem.post(); });// 设置退出信号
    sem.wait();
    return 0;
}