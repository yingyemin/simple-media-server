#ifndef JT808Connection_H
#define JT808Connection_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Net/TcpConnection.h"
#include "JT808Parser.h"
#include "Util/TimeClock.h"
#include "JT808Context.h"

using namespace std;

class JT808Connection : public TcpConnection
{
public:
    using Ptr = shared_ptr<JT808Connection>;
    using Wptr = weak_ptr<JT808Connection>;

    JT808Connection(const EventLoop::Ptr& loop, const Socket::Ptr& socket);
    ~JT808Connection();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError(const string& msg) override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;

    void onJT808Packet(const JT808Packet::Ptr& buffer);
    void setOnClose(const function<void()>& cb) {_onClose = cb;}

private:
    void packHeader(const JT808Packet::Ptr& buffer, JT808Header& header);
    StringBuffer::Ptr pack(JT808Header& header, const StringBuffer::Ptr& body);

    // 补全函数声明
    void commonRespose(const JT808Packet::Ptr& buffer);
    void handleTerminalReply(const JT808Packet::Ptr& buffer);
    void handlePlatformReply(const JT808Packet::Ptr& buffer);
    void handleHeartbeat(const JT808Packet::Ptr& buffer);
    void handleServerResendReq(const JT808Packet::Ptr& buffer);
    void handleTerminalResendReq(const JT808Packet::Ptr& buffer);
    void handleRegister(const JT808Packet::Ptr& buffer);
    void handleRegisterResp(const JT808Packet::Ptr& buffer);
    void handleLogout(const JT808Packet::Ptr& buffer);
    void handleQueryTime(const JT808Packet::Ptr& buffer);
    void handleQueryTimeResp(const JT808Packet::Ptr& buffer);
    void handleAuthenticate(const JT808Packet::Ptr& buffer);
    void handleSetParam(const JT808Packet::Ptr& buffer);
    void handleQueryParam(const JT808Packet::Ptr& buffer);
    void handleParamResp(const JT808Packet::Ptr& buffer);
    void handleCtrlCmd(const JT808Packet::Ptr& buffer);
    void handleQuerySpecParam(const JT808Packet::Ptr& buffer);
    void handleQueryAttr(const JT808Packet::Ptr& buffer);
    void handleAttrResp(const JT808Packet::Ptr& buffer);
    void handleUpgradePkg(const JT808Packet::Ptr& buffer);
    void handleUpgradeResult(const JT808Packet::Ptr& buffer);
    void handleLocationReport(const JT808Packet::Ptr& buffer);
    void handleLocationQuery(const JT808Packet::Ptr& buffer);
    void handleLocationResp(const JT808Packet::Ptr& buffer);
    void handleTrackControl(const JT808Packet::Ptr& buffer);
    void handleEventCmd(const JT808Packet::Ptr& buffer);
    void handleEventReport(const JT808Packet::Ptr& buffer);
    void handleQueryCmd(const JT808Packet::Ptr& buffer);
    void handleQueryResp(const JT808Packet::Ptr& buffer);
    void handleInfotainmentMenu(const JT808Packet::Ptr& buffer);
    void handleInfotainmentCtrl(const JT808Packet::Ptr& buffer);
    void handleInfoService(const JT808Packet::Ptr& buffer);
    void handleCallback(const JT808Packet::Ptr& buffer);
    void handlePhonebookSet(const JT808Packet::Ptr& buffer);
    void handleVehicleCtrl(const JT808Packet::Ptr& buffer);
    void handleVehicleResp(const JT808Packet::Ptr& buffer);
    void handleAreaSet(const JT808Packet::Ptr& buffer);
    void handleAreaDelete(const JT808Packet::Ptr& buffer);
    void handleRectSet(const JT808Packet::Ptr& buffer);
    void handleRectDelete(const JT808Packet::Ptr& buffer);
    void handlePolygonSet(const JT808Packet::Ptr& buffer);
    void handlePolygonDelete(const JT808Packet::Ptr& buffer);
    void handleRouteSet(const JT808Packet::Ptr& buffer);
    void handleRouteDelete(const JT808Packet::Ptr& buffer);
    void handleDataCollectCmd(const JT808Packet::Ptr& buffer);
    void handleDataUpload(const JT808Packet::Ptr& buffer);
    void handleParamDownCmd(const JT808Packet::Ptr& buffer);
    void handleAlarmConfirm(const JT808Packet::Ptr& buffer);
    void handleLinkTest(const JT808Packet::Ptr& buffer);
    void handleTextMsg(const JT808Packet::Ptr& buffer);
    void handleDriverInfoReport(const JT808Packet::Ptr& buffer);
    void handleDriverInfoReq(const JT808Packet::Ptr& buffer);
    void handleBatchLocUpload(const JT808Packet::Ptr& buffer);
    void handleCANData(const JT808Packet::Ptr& buffer);
    void handleMediaEvent(const JT808Packet::Ptr& buffer);
    void handleMediaUpload(const JT808Packet::Ptr& buffer);
    void handleMediaResp(const JT808Packet::Ptr& buffer);
    void handleSnapCmd(const JT808Packet::Ptr& buffer);
    void handleSnapResp(const JT808Packet::Ptr& buffer);
    void handleMediaSearch(const JT808Packet::Ptr& buffer);
    void handleSearchResp(const JT808Packet::Ptr& buffer);
    void handleQueryArea(const JT808Packet::Ptr& buffer);
    void handleAreaResp(const JT808Packet::Ptr& buffer);
    void handleWaybillReport(const JT808Packet::Ptr& buffer);
    void handleMediaUploadDup(const JT808Packet::Ptr& buffer);
    void handleRecStart(const JT808Packet::Ptr& buffer);
    void handleSingleMediaSearch(const JT808Packet::Ptr& buffer);
    void handleTransparentDown(const JT808Packet::Ptr& buffer);
    void handleTransparentUp(const JT808Packet::Ptr& buffer);
    void handleDataCompression(const JT808Packet::Ptr& buffer);
    void handleRSAKeyPlatform(const JT808Packet::Ptr& buffer);
    void handleRSAKeyTerminal(const JT808Packet::Ptr& buffer);
    void handleTerminalUploadPassengerFlow(const JT808Packet::Ptr& buffer);

private:
    string _simCode;
    string _key;
    JT808Parser _parser;
    TimeClock _timeClock;
    JT808Context::Ptr _context;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    function<void()> _onClose;
};


#endif //JT808Connection_H