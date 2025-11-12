/*
 * Copyright (c) 2016 The ZLToolKit project authors. All Rights Reserved.
 *
 * This file is part of ZLToolKit(https://github.com/ZLMediaKit/ZLToolKit).
 *
 * Use of this source code is governed by MIT license that can be found in the
 * LICENSE file in the root of the source tree. All contributing project authors
 * may be found in the AUTHORS file in the root of the source tree.
 */

#include <csignal>
#include <iostream>

#ifndef _WIN32
#include <unistd.h>
#endif

#include <TcpClient.h>
#include <Log/Logger.h>
#include <EventLoopPool.h>
#include <Net/TcpServer.h>
#include <Net/TcpConnection.h>
#include <Util/semaphore.h>

using namespace std;

class EchoSession: public TcpConnection {
public:
    using Ptr = std::shared_ptr<EchoSession>;
    EchoSession(const EventLoop::Ptr& loop, const Socket::Ptr& socket) :
            TcpConnection(loop,socket) {
        logTrace << "EchoSession Creator";
    }
    ~EchoSession() {
        logTrace << "EchoSession Destory";
    }
    virtual void onRecv(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override{
        //处理客户端发送过来的数据  [AUTO-TRANSLATED:c095b82e]
        // Handle data sent from the client
        logInfo << buffer->data() << " from port:" << getSocket()->getPeerPort();
        send(buffer);
    }
    virtual void onError() override{
        //客户端断开连接或其他原因导致该对象脱离TCPServer管理  [AUTO-TRANSLATED:6b958a7b]
        // Client disconnects or other reasons cause the object to be removed from TCPServer management
        logError << "TcpConnection error";
        close();
    }
    virtual void onManager() override{
        //定时管理该对象，譬如会话超时检查  [AUTO-TRANSLATED:2caa54f6]
        // Periodically manage the object, such as session timeout check
        //DebugL;
    }
};


int main() {
    //初始化日志模块  [AUTO-TRANSLATED:fd9321b2]
    // Initialize the log module
    Logger::instance()->addChannel(std::make_shared<ConsoleChannel>());
    Logger::instance()->setWriter(std::make_shared<AsyncLogWriter>());
    
    EventLoopPool::instance()->init(0, true, true);

    TcpServer::Ptr server(new TcpServer(EventLoopPool::instance()->getLoopByCircle(),"127.0.0.1",9000,0,0));

    server->setOnCreateSession([](const EventLoop::Ptr& loop, const Socket::Ptr& socket) -> EchoSession::Ptr {
        return make_shared<EchoSession>(loop, socket);
        });
    server->start();
    //退出程序事件处理  [AUTO-TRANSLATED:80065cb7]
    // Exit program event handling
    static semaphore sem;
    signal(SIGINT, [](int) { sem.post(); });// 设置退出信号
    sem.wait();
    return 0;
}