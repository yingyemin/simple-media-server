#ifndef JT808Context_H
#define JT808Context_H

#include "Buffer.h"
#include "Util/TimeClock.h"
#include "EventPoller/EventLoop.h"
#include "Net/Socket.h"
#include "JT808Packet.h"
#include "JT808Body/T0200.h"

#include <memory>
#include <unordered_map>

using namespace std;

class JT808MediaInfo{
public:
    string simCode;
    string streamId;
    int channelNum;
    int type; // 0: 实时流 1: 回放 2：语音对讲
    uint64_t startStamp;
    uint64_t endStamp;
    string ip;
    int tcpPort;
    int udpPort = 0;
    int mediaType;
    int streamType;
};

class JT808DeviceInfo
{
public:
    uint16_t devicePort;
    uint16_t deviceVersion;
    uint16_t provinceId;  // 省域 ID，2 字节
    uint16_t cityId;      // 市县域 ID，2 字节
    std::string makerId; // 制造商 ID，5 字节
    std::string deviceModel; // 终端型号，8 字节
    std::string deviceId; // 终端 ID，7 字节
    uint8_t plateColor; // 车牌颜色，1 字节
    std::string plateNo; // 车辆标识，长度不定
    std::string deviceIp;

    // location
    std::shared_ptr<T0200_Version1> location;
};

class JT808Context : public enable_shared_from_this<JT808Context>
{
public:
    using Ptr = shared_ptr<JT808Context>;
    using Wptr = weak_ptr<JT808Context>;

    JT808Context(const EventLoop::Ptr& loop, const string& deviceId);
    ~JT808Context();
public:
    bool init();
    void update();
    void onRegister(const Socket::Ptr& socket, const JT808Packet::Ptr& packet);
    void onLocationReport(const JT808Packet::Ptr& packet);
    void onJT808Packet(const Socket::Ptr& socket, const JT808Packet::Ptr& packet, struct sockaddr* addr = nullptr, int len = 0, bool sort = false);
    void heartbeat();
    bool isAlive() {return _alive;}
    void on9101(const JT808MediaInfo& mediainfo);
    void on9102(int channel, int control, int closeType, int mediaType);
    void sendMessage(const Buffer::Ptr& buffer);
    void packHeader(JT808Header& header);
    bool isMediaExist(const string& simCode, const string& streamId);
    JT808DeviceInfo getDeviceInfo() {return _deviceInfo;}

private:
    bool _alive = true;
    string _simCode;
    JT808DeviceInfo _deviceInfo;
    TimeClock _timeClock;
    JT808Header _header;
    shared_ptr<sockaddr> _addr;
    Socket::Ptr _socket;
    EventLoop::Ptr _loop;

    mutex _mtx;
    // simCode, streamId/port, JT808MediaInfo
    unordered_map<string, unordered_map<string, JT808MediaInfo>> _mapMediaInfo;
};



#endif //JT808Context_H
