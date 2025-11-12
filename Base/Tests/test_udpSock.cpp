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
#include <memory>
#include <sstream>
#include <TcpClient.h>
#include <Log/Logger.h>
#include <EventLoopPool.h>
#include <Util/semaphore.h>

using namespace std;


//主线程退出标志  [AUTO-TRANSLATED:4465f04c]
// Main thread exit flag
bool exitProgram = false;

//赋值struct sockaddr  [AUTO-TRANSLATED:07f9df9d]
// Assign struct sockaddr
//void makeAddr(struct sockaddr_storage *out,const char *ip,uint16_t port){
//    *out = SockUtil::make_sockaddr(ip, port);
//}
//
////获取struct sockaddr的IP字符串  [AUTO-TRANSLATED:651562f1]
//// Get IP string from struct sockaddr
//string getIP(struct sockaddr *addr){
//    return SockUtil::inet_ntoa(addr);
//}

int main() {
    //设置程序退出信号处理函数  [AUTO-TRANSLATED:419fb1c3]
    // Set program exit signal handling function
    signal(SIGINT, [](int){exitProgram = true;});
    //设置日志系统  [AUTO-TRANSLATED:ad15b8d6]
    // Set up logging system
    // Set up the logging system
    Logger::instance()->addChannel(std::make_shared<ConsoleChannel>("ConsoleChannel", LTrace));
    Logger::instance()->setWriter(std::make_shared<AsyncLogWriter>());
    // 多线程开启epoll
    EventLoopPool::instance()->init(0, true, true);

    auto sockRecv = make_shared<Socket>(EventLoopPool::instance()->getLoopByCircle());

    sockRecv->createSocket(SOCKET_UDP);

   
    sockRecv->bind(9001,"0.0.0.0");//接收UDP绑定9001端口

    sockRecv->addToEpoll();
    sockRecv->setReadCb([sockRecv](const Buffer::Ptr& buf, struct sockaddr* addr, int)->int {
        //接收到数据回调  [AUTO-TRANSLATED:1cd064ad]
        // int recvLen = recvfrom(sock, buffer, BUFFER_SIZE, 0, (sockaddr*)&clientAddr, &clientAddrLen);

        // Data received callback
        //DebugL << "recv data form " << getIP(addr) << ":" << buf->data();
        logInfo << "###recv buf" << buf->data() << " from port:" << sockRecv->getPeerPort();
        return 0;
        });
    static semaphore sem;
    signal(SIGINT, [](int) { sem.post(); });// 设置退出信号
    sem.wait();

    return 0;
}





