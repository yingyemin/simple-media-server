#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>
#include <arpa/inet.h>

#include "JT808Connection.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"
#include "Common/HookManager.h"
#include "Common/Config.h"
#include "JT808Body/T8100.h"
#include "JT808Body/T0100.h"
#include "JT808Body/T0001.h"
#include "JT808Body/T0200.h"
#include "JT1078Body/T9101.h"
#include "JT808Manager.h"

using namespace std;

JT808Connection::JT808Connection(const EventLoop::Ptr& loop, const Socket::Ptr& socket)
    :TcpConnection(loop, socket)
    ,_loop(loop)
    ,_socket(socket)
{
    logTrace << "JT808Connection: " << this;
}

JT808Connection::~JT808Connection()
{
    logTrace << "~JT808Connection: " << this << ", path: " << _simCode;

    // if (_onClose) {
    //     _onClose();
    // }
}

void JT808Connection::init()
{
    weak_ptr<JT808Connection> wSelf = static_pointer_cast<JT808Connection>(shared_from_this());
    _parser.setOnJT808Packet([wSelf](const JT808Packet::Ptr& buffer){
        auto self = wSelf.lock();
        if(!self){
            return;
        }
        
        self->onJT808Packet(buffer);
    });

    _timeClock.start();
}

void JT808Connection::close()
{
    logTrace << "path: " << _simCode << "JT808Connection::close()";
    TcpConnection::close();
    if (_onClose) {
        _onClose();
    }
}

void JT808Connection::onManager()
{
    logTrace << "path: " << _simCode << ", on manager";
    static int timeout = Config::instance()->getAndListen([](const json& config){
        timeout = Config::instance()->get("JT808", "Server", "Server1", "timeout", "60000");
    }, "JT808", "Server", "Server1", "timeout", "60000");

    // logInfo << "_timeClock.startToNow() " << _timeClock.startToNow();
    // logInfo << "timeout " << timeout;
    if (_timeClock.startToNow() > timeout) {
        logWarn << _simCode <<  ": timeout";
        weak_ptr<JT808Connection> wSelf = dynamic_pointer_cast<JT808Connection>(shared_from_this());
        // 直接close会将tcpserver的map迭代器破坏，用异步接口关闭
        _loop->async([wSelf](){
            auto self = wSelf.lock();
            if (self) {
                self->close();
            }
        }, false);
    }
}

void JT808Connection::onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    logInfo << "get a buf: " << buffer->size();
    _parser.parse(buffer->data(), buffer->size());
}

void JT808Connection::onError(const string& msg)
{
    close();
    logWarn << "get a error: " << msg;
}

ssize_t JT808Connection::send(Buffer::Ptr pkt)
{
    logInfo << "pkt size: " << pkt->size();
    return TcpConnection::send(pkt);
}

void JT808Connection::onJT808Packet(const JT808Packet::Ptr& buffer)
{
    logInfo << "onJT808Packet: " << (int)buffer->_header.getMsgId();
    _timeClock.update();

    auto self = static_pointer_cast<JT808Connection>(shared_from_this());
    static unordered_map<int, void (JT808Connection::*)(const JT808Packet::Ptr& buffer)> jt808Handle {
        {TerminalReply, &JT808Connection::handleTerminalReply},
        {PlatformReply, &JT808Connection::handlePlatformReply},
        {Heartbeat, &JT808Connection::handleHeartbeat},
        {ServerResendReq, &JT808Connection::handleServerResendReq},
        {TerminalResendReq, &JT808Connection::handleTerminalResendReq},
        {Register, &JT808Connection::handleRegister},
        {RegisterResp, &JT808Connection::handleRegisterResp},
        {Logout, &JT808Connection::handleLogout},
        {QueryTime, &JT808Connection::handleQueryTime},
        {QueryTimeResp, &JT808Connection::handleQueryTimeResp},
        {Authenticate, &JT808Connection::handleAuthenticate},
        {SetParam, &JT808Connection::handleSetParam},
        {QueryParam, &JT808Connection::handleQueryParam},
        {ParamResp, &JT808Connection::handleParamResp},
        {CtrlCmd, &JT808Connection::handleCtrlCmd},
        {QuerySpecParam, &JT808Connection::handleQuerySpecParam},
        {QueryAttr, &JT808Connection::handleQueryAttr},
        {AttrResp, &JT808Connection::handleAttrResp},
        {UpgradePkg, &JT808Connection::handleUpgradePkg},
        {UpgradeResult, &JT808Connection::handleUpgradeResult},
        {LocationReport, &JT808Connection::handleLocationReport},
        {LocationQuery, &JT808Connection::handleLocationQuery},
        {LocationResp, &JT808Connection::handleLocationResp},
        {TrackControl, &JT808Connection::handleTrackControl},
        {EventCmd, &JT808Connection::handleEventCmd},
        {EventReport, &JT808Connection::handleEventReport},
        {QueryCmd, &JT808Connection::handleQueryCmd},
        {QueryResp, &JT808Connection::handleQueryResp},
        {InfotainmentMenu, &JT808Connection::handleInfotainmentMenu},
        {InfotainmentCtrl, &JT808Connection::handleInfotainmentCtrl},
        {InfoService, &JT808Connection::handleInfoService},
        {Callback, &JT808Connection::handleCallback},
        {PhonebookSet, &JT808Connection::handlePhonebookSet},
        {VehicleCtrl, &JT808Connection::handleVehicleCtrl},
        {VehicleResp, &JT808Connection::handleVehicleResp},
        {AreaSet, &JT808Connection::handleAreaSet},
        {AreaDelete, &JT808Connection::handleAreaDelete},
        {RectSet, &JT808Connection::handleRectSet},
        {RectDelete, &JT808Connection::handleRectDelete},
        {PolygonSet, &JT808Connection::handlePolygonSet},
        {PolygonDelete, &JT808Connection::handlePolygonDelete},
        {RouteSet, &JT808Connection::handleRouteSet},
        {RouteDelete, &JT808Connection::handleRouteDelete},
        {DataCollectCmd, &JT808Connection::handleDataCollectCmd},
        {DataUpload, &JT808Connection::handleDataUpload},
        {ParamDownCmd, &JT808Connection::handleParamDownCmd},
        {AlarmConfirm, &JT808Connection::handleAlarmConfirm},
        {LinkTest, &JT808Connection::handleLinkTest},
        {TextMsg, &JT808Connection::handleTextMsg},
        {DriverInfoReport, &JT808Connection::handleDriverInfoReport},
        {DriverInfoReq, &JT808Connection::handleDriverInfoReq},
        {BatchLocUpload, &JT808Connection::handleBatchLocUpload},
        {CANData, &JT808Connection::handleCANData},
        {MediaEvent, &JT808Connection::handleMediaEvent},
        {MediaUpload, &JT808Connection::handleMediaUpload},
        {MediaResp, &JT808Connection::handleMediaResp},
        {SnapCmd, &JT808Connection::handleSnapCmd},
        {SnapResp, &JT808Connection::handleSnapResp},
        {MediaSearch, &JT808Connection::handleMediaSearch},
        {SearchResp, &JT808Connection::handleSearchResp},
        {QueryArea, &JT808Connection::handleQueryArea},
        {AreaResp, &JT808Connection::handleAreaResp},
        {WaybillReport, &JT808Connection::handleWaybillReport},
        {MediaUploadDup, &JT808Connection::handleMediaUploadDup},
        {RecStart, &JT808Connection::handleRecStart},
        {SingleMediaSearch, &JT808Connection::handleSingleMediaSearch},
        {TransparentDown, &JT808Connection::handleTransparentDown},
        {TransparentUp, &JT808Connection::handleTransparentUp},
        {DataCompression, &JT808Connection::handleDataCompression},
        {RSAKeyPlatform, &JT808Connection::handleRSAKeyPlatform},
        {RSAKeyTerminal, &JT808Connection::handleRSAKeyTerminal},
        {TerminalUploadPassengerFlow, &JT808Connection::handleTerminalUploadPassengerFlow}
    };

    logDebug << "simcode: " << buffer->_header.getSimCode() << ", msgId" << buffer->_header.getMsgId();
    auto it = jt808Handle.find(buffer->_header.getMsgId());
    if (it != jt808Handle.end()) {
        (this->*(it->second))(buffer);
    } else {
        // sendRtspResponse("403 Forbidden");
        // throw SockException(Err_shutdown, StrPrinter << "403 Forbidden:" << method);
        logWarn << "simcode: " << buffer->_header.getSimCode() << ", msgId" << buffer->_header.getMsgId();
    }
}

StringBuffer::Ptr JT808Connection::pack(JT808Header& header, const StringBuffer::Ptr& body)
{
    StringBuffer::Ptr buffer = std::make_shared<StringBuffer>();
    buffer->push_back((header.msgId >> 8) & 0xFF);
    buffer->push_back(header.msgId & 0xFF);
    header.bodyLength = body->size();

    //消息体属性构造
    uint16_t bprops = (header.bodyLength & 0x03FF) |
                      (header.isEncrypted ? 0x0400 : 0) |
                      (header.isPacketSplit ? 0x0020 : 0) |
                      (header.versionFlag << 14);
                      
    buffer->push_back((bprops >> 8) & 0xFF);
    buffer->push_back(bprops & 0xFF);

    if (header.versionFlag)
    {
        buffer->push_back(header.version);
    }
    buffer->append(header.simCode);
    buffer->push_back((header.serialNo >> 8) & 0xFF);
    buffer->push_back(header.serialNo & 0xFF);

    if (header.isPacketSplit)
    {
        buffer->push_back((header.packSum >> 8) & 0xFF);
        buffer->push_back(header.packSum & 0xFF);
        buffer->push_back((header.packSeq >> 8) & 0xFF);
        buffer->push_back(header.packSeq & 0xFF);
    }

    if (body)
    {
        buffer->append(*body);
    }

    buffer->push_back(JT808Packet::calcCheckSum((const unsigned char*)buffer->data(), buffer->size()));
    auto escapedData = JT808Packet::escape((const unsigned char*)buffer->data(), buffer->size());

    return escapedData;
}

void JT808Connection::packHeader(const JT808Packet::Ptr& buffer, JT808Header& header)
{
    // header.msgId = PlatformReply;
    header.isEncrypted = buffer->_header.isEncrypted;
    header.isPacketSplit = false;
    header.versionFlag = buffer->_header.versionFlag;
    header.version = buffer->_header.version;
    header.simCode = buffer->_header.getSimCode();
    header.serialNo = buffer->_header.getSerialNo();
}

// 平台通用应答
void JT808Connection::commonRespose(const JT808Packet::Ptr& buffer)
{
    JT808Header header;
    header.msgId = PlatformReply;
    packHeader(buffer, header);

    StringBuffer::Ptr body = std::make_shared<StringBuffer>();
    body->push_back(header.serialNo >> 8);
    body->push_back(header.serialNo & 0xFF);
    body->push_back(header.msgId >> 8);
    body->push_back(header.msgId & 0xFF);
    body->push_back(0); //0：成功，1：失败；2：消息有误；3：不支持；4：报警处理确认

    auto escapedData = pack(header, body);
    send(escapedData);
}

// 终端通用应答
void JT808Connection::handleTerminalReply(const JT808Packet::Ptr& buffer)
{
    T0001 t0001;
    t0001.parse(buffer->_content.data(), buffer->_content.size());

    logInfo << "handleTerminalReply, serialNo: " << (int)t0001.getResponseSerialNo() 
            << ", msgId: " << (int)t0001.getResponseMessageId() 
            << ", resultCode: " << (int)t0001.getResultCode();
}

void JT808Connection::handlePlatformReply(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleHeartbeat(const JT808Packet::Ptr& buffer)
{
    // 返回平台通用应答
    commonRespose(buffer);
    
    if (_context) {
        _context->update();
        return ;
    }
    _context = make_shared<JT808Context>(_loop, _simCode);
    JT808Manager::instance()->addContext(_simCode, _context);
    _context->update();
}

void JT808Connection::handleServerResendReq(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleTerminalResendReq(const JT808Packet::Ptr& buffer)
{

}

// 注册
void JT808Connection::handleRegister(const JT808Packet::Ptr& buffer)
{
    JT808Header header;
    header.msgId = RegisterResp;
    packHeader(buffer, header);

    _simCode = JT808Packet::decodeBCDPhoneNumber(buffer->_header.getSimCode());

    // 注册应答
    T8100 t8100;
    t8100.setResponseSerialNo(buffer->_header.getSerialNo());
    t8100.setResultCode(T8100::getSuccess());
    t8100.setToken(randomStr(10));

    auto escapedData = pack(header, t8100.encode());
    send(escapedData);

    if (_context) {
        _context->onRegister(_socket, buffer);
        return ;
    }

    logInfo << "create context, simCode: " << _simCode;

    _context = make_shared<JT808Context>(_loop, _simCode);
    JT808Manager::instance()->addContext(_simCode, _context);
    _context->onRegister(_socket, buffer);
}

void JT808Connection::handleRegisterResp(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleLogout(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleQueryTime(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleQueryTimeResp(const JT808Packet::Ptr& buffer)
{

}

// 鉴权
void JT808Connection::handleAuthenticate(const JT808Packet::Ptr& buffer)
{
    // 鉴权，平台通用应答
    commonRespose(buffer);
}

void JT808Connection::handleSetParam(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleQueryParam(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleParamResp(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleCtrlCmd(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleQuerySpecParam(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleQueryAttr(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleAttrResp(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleUpgradePkg(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleUpgradeResult(const JT808Packet::Ptr& buffer)
{

}

// 位置上报
void JT808Connection::handleLocationReport(const JT808Packet::Ptr& buffer)
{
    // T0200_Version1 t0200;
    // t0200.parse(buffer->_content.data(), buffer->_content.size());

    if (_context) {
        _context->onLocationReport(buffer);
    }

    commonRespose(buffer);
}

void JT808Connection::handleLocationQuery(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleLocationResp(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleTrackControl(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleEventCmd(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleEventReport(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleQueryCmd(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleQueryResp(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleInfotainmentMenu(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleInfotainmentCtrl(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleInfoService(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleCallback(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handlePhonebookSet(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleVehicleCtrl(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleVehicleResp(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleAreaSet(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleAreaDelete(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleRectSet(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleRectDelete(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handlePolygonSet(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handlePolygonDelete(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleRouteSet(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleRouteDelete(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleDataCollectCmd(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleDataUpload(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleParamDownCmd(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleAlarmConfirm(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleLinkTest(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleTextMsg(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleDriverInfoReport(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleDriverInfoReq(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleBatchLocUpload(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleCANData(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleMediaEvent(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleMediaUpload(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleMediaResp(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleSnapCmd(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleSnapResp(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleMediaSearch(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleSearchResp(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleQueryArea(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleAreaResp(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleWaybillReport(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleMediaUploadDup(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleRecStart(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleSingleMediaSearch(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleTransparentDown(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleTransparentUp(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleDataCompression(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleRSAKeyPlatform(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleRSAKeyTerminal(const JT808Packet::Ptr& buffer)
{

}

void JT808Connection::handleTerminalUploadPassengerFlow(const JT808Packet::Ptr& buffer)
{

}
