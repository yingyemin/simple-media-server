#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#if !defined(_WIN32)
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iconv.h>
#endif

#include "JT808Context.h"
#include "Logger.h"
#if defined(_WIN32)
#include "Util/String.hpp"
#else
#include "Util/String.hpp"
#endif
#include "Common/Define.h"
#include "Common/Config.h"
#include "Hook/MediaHook.h"
#include "JT1078Body/T9101.h"
#include "JT1078Body/T9102.h"
#include "JT808Body/T0100.h"

using namespace std;

JT808Context::JT808Context(const EventLoop::Ptr& loop, const string& deviceId)
    :_simCode(deviceId)
    ,_loop(loop)
{
    logTrace << "JT808Context::JT808Context";
}

JT808Context::~JT808Context()
{
}

bool JT808Context::init()
{
    
    return true;
}

void JT808Context::update()
{
    _timeClock.update();
}

#if defined(_WIN32)
// GBK 转 UTF-8
std::string GBKToUTF8(const std::string& gbk_str) {
    // 先将 GBK 转为宽字符（UTF-16）
    int wcs_len = MultiByteToWideChar(CP_ACP, 0, gbk_str.c_str(), -1, nullptr, 0);
    wchar_t* wcs_buf = new wchar_t[wcs_len];
    MultiByteToWideChar(CP_ACP, 0, gbk_str.c_str(), -1, wcs_buf, wcs_len);

    // 再将宽字符转为 UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wcs_buf, -1, nullptr, 0, nullptr, nullptr);
    char* utf8_buf = new char[utf8_len];
    WideCharToMultiByte(CP_UTF8, 0, wcs_buf, -1, utf8_buf, utf8_len, nullptr, nullptr);

    std::string utf8_str(utf8_buf);
    delete[] wcs_buf;
    delete[] utf8_buf;
    return utf8_str;
}
#else
std::string GBKToUTF8(const std::string& gbkStr) {
    iconv_t cd = iconv_open("UTF - 8", "GBK");
    if (cd == (iconv_t)-1) {
        return "";
    }
    
    size_t inlen = gbkStr.size();
    size_t outlen = inlen * 3; // 预估输出长度
    char* inbuf = const_cast<char*>(gbkStr.c_str());
    std::string utf8Str(outlen, 0);
    char* outbuf = &utf8Str[0];
    
    if (iconv(cd, &inbuf, &inlen, &outbuf, &outlen) == (size_t)-1) {
        iconv_close(cd);
        return "";
    }
    
    iconv_close(cd);
    utf8Str.resize(utf8Str.size() - outlen);
    return utf8Str;
}
#endif

void JT808Context::onRegister(const Socket::Ptr& socket, const JT808Packet::Ptr& packet)
{
    _timeClock.update();

    if (_socket) {
        return ;
    }

    _socket = socket;
    _header = packet->_header;

    if (packet->_header.bodyLength > 0 && packet->_header.bodyLength < 37) {
        // 2011
        _deviceInfo.deviceVersion = 2011;
        T0100VNeg1 t0100;
        t0100.parse(packet->_content.data(), packet->_content.size());

        _deviceInfo.provinceId = t0100.getProvinceId();
        _deviceInfo.cityId = t0100.getCityId();
        _deviceInfo.deviceModel = t0100.getDeviceModel();
        _deviceInfo.deviceId = t0100.getDeviceId();
        _deviceInfo.makerId = t0100.getMakerId();
        _deviceInfo.plateNo = t0100.getPlateNo();
        _deviceInfo.plateColor = t0100.getPlateColor();
        _deviceInfo.deviceIp = _socket->getPeerIp();
        _deviceInfo.devicePort = _socket->getPeerPort();
    } else if (packet->_header.version == 1) {
        // 2019
        _deviceInfo.deviceVersion = 2019;
        T0100V1 t0100;
        t0100.parse(packet->_content.data(), packet->_content.size());

        _deviceInfo.provinceId = t0100.getProvinceId();
        _deviceInfo.cityId = t0100.getCityId();
        _deviceInfo.deviceModel = t0100.getDeviceModel();
        _deviceInfo.deviceId = t0100.getDeviceId();
        _deviceInfo.makerId = t0100.getMakerId();
        _deviceInfo.plateNo = t0100.getPlateNo();
        _deviceInfo.deviceIp = _socket->getPeerIp();
        _deviceInfo.devicePort = _socket->getPeerPort();
    } else {
        // 2013
        _deviceInfo.deviceVersion = 2013;
        T0100V0 t0100;
        t0100.parse(packet->_content.data(), packet->_content.size());

        _deviceInfo.provinceId = t0100.getProvinceId();
        _deviceInfo.cityId = t0100.getCityId();
        _deviceInfo.deviceModel = t0100.getDeviceModel();
        _deviceInfo.deviceId = t0100.getDeviceId();
        _deviceInfo.makerId = t0100.getMakerId();
        _deviceInfo.plateNo = t0100.getPlateNo();
        _deviceInfo.deviceIp = _socket->getPeerIp();
        _deviceInfo.devicePort = _socket->getPeerPort();
    }

    auto pos = _deviceInfo.deviceModel.find('\0');
    _deviceInfo.deviceModel = _deviceInfo.deviceModel.substr(0, pos);
    _deviceInfo.plateNo = GBKToUTF8(_deviceInfo.plateNo);
}

void JT808Context::onJT808Packet(const Socket::Ptr& socket, const JT808Packet::Ptr& packet, struct sockaddr* addr, int len, bool sort)
{
    // if (addr) {
    //     if (!_addr) {
    //         _addr = make_shared<sockaddr>();
    //         memcpy(_addr.get(), addr, sizeof(sockaddr));
    //     } else if (memcmp(_addr.get(), addr, sizeof(struct sockaddr)) != 0) {
    //         // 记录一下这个流，提供切换流的api
    //         logWarn<< "收到 sip 包，但已经存在一个相同的设备，忽略";
    //         return ;
    //     }
    // }

    if (_socket != socket) {
        _socket = socket;
        _header = packet->_header;
    }

    _timeClock.startToNow();
}

void JT808Context::onLocationReport(const JT808Packet::Ptr& packet)
{
    std::shared_ptr<T0200_Version1> t0200 = make_shared<T0200_Version1>();
    t0200->parse((uint8_t*)packet->_content.data(), packet->_content.size());
    t0200->setDeviceTime(JT808Packet::decodeBCDPhoneNumber(t0200->getDeviceTime()));

    _deviceInfo.location = t0200;
}

void JT808Context::heartbeat()
{
    // static int timeout = Config::instance()->getConfig()["GB28181"]["Server"]["timeout"];
    static int timeout = Config::instance()->getAndListen([](const json& config){
        timeout = Config::instance()->get("JT808", "Server", "timeout", "", "60000");
        logInfo << "timeout: " << timeout;
    }, "JT808", "Server", "timeout", "", "60000");

    if (_timeClock.startToNow() > timeout) {
        logInfo << "alive is false";
        _alive = false;
    }
}

void JT808Context::packHeader(JT808Header& header)
{
    header.isEncrypted = _header.isEncrypted;
    header.isPacketSplit = false;
    header.versionFlag = _header.versionFlag;
    header.version = _header.version;
    header.simCode = _header.getSimCode();
    header.serialNo = _header.getSerialNo();
}

bool JT808Context::isMediaExist(const string& simCode, const string& streamId)
{
    lock_guard<mutex> lck(_mtx);
    auto it = _mapMediaInfo.find(simCode);
    if (it == _mapMediaInfo.end()) {
        return false;
    }

    auto it2 = it->second.find(streamId);
    if (it2 == it->second.end()) {
        return false;
    }

    return true;
}

void JT808Context::on9101(const JT808MediaInfo& mediainfo)
{
    if (!isAlive()) {
        logInfo << "JT808Context::catalog isAlive is false";
        return;
    }

    string streamId = mediainfo.streamId.empty() ? to_string(mediainfo.tcpPort) : mediainfo.streamId;
    if (isMediaExist(mediainfo.simCode, streamId)) {
        logWarn << "JT808Context::on9101 isMediaExist is true";
        return;
    }

    JT808Header header1078;
    header1078.msgId = 0x9101;
    packHeader(header1078);

    T9101 t9101;
    t9101.setChannelNo(mediainfo.channelNum);
    t9101.setIp(mediainfo.ip);
    t9101.setTcpPort(mediainfo.tcpPort);
    t9101.setUdpPort(mediainfo.udpPort);
    t9101.setMediaType(mediainfo.mediaType);
    t9101.setStreamType(mediainfo.streamType);

    auto escapedData1078 = JT808Packet::pack(header1078, t9101.encode());
    sendMessage(escapedData1078);

    lock_guard<mutex> lck(_mtx);
    _mapMediaInfo[mediainfo.simCode][streamId] = mediainfo;
}

void JT808Context::on9102(int channel, int control, int closeType, int mediaType)
{
    if (!isAlive()) {
        logInfo << "JT808Context::catalog isAlive is false";
        return;
    }

    JT808Header header1078;
    header1078.msgId = 0x9102;
    packHeader(header1078);

    T9102 t9102;
    t9102.setChannelNo(channel);
    t9102.setCommand(control);
    t9102.setCloseType(closeType);
    t9102.setStreamType(mediaType);

    auto escapedData1078 = JT808Packet::pack(header1078, t9102.encode());
    sendMessage(escapedData1078);

    string streamId = to_string(_socket->getLocalPort());
    lock_guard<mutex> lck(_mtx);
    auto it = _mapMediaInfo.find(_simCode);
    if (it != _mapMediaInfo.end()) {
        it->second.erase(streamId);
        if(it->second.empty()) {
            _mapMediaInfo.erase(it);
        }
    }
}

void JT808Context::sendMessage(const Buffer::Ptr& buffer)
{
    logTrace << "send a message" << buffer->size();
    _socket->send(buffer, 1, 0, buffer->size(), _addr.get(), sizeof(sockaddr));
}