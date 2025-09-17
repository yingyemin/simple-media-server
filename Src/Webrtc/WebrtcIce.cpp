#include "WebrtcIce.h"
#include "WebrtcStun.h"
#include "Util/String.h"
#include "Common/UrlParser.h"
#include "Log/Logger.h"

WebrtcIce::WebrtcIce(const std::vector<std::string>& urls)
{
    _urls = urls;
}

WebrtcIce::~WebrtcIce()
{
}

void WebrtcIce::gatherNetwork(const std::string& ifacePrefix)
{
	// 获取本地地址
	_interfaces = Socket::getIfaceList(ifacePrefix);

    for (auto& url : _urls) {
        if (url.find("stun") != std::string::npos) { // 获取stun地址
            gatherStunAddr(url);
        } else if (url.find("turn") != std::string::npos) { // 获取turn地址
            gatherTurnAddr(url, "username", "credential");
        }
    }
}

void WebrtcIce::gatherStunAddr(const std::string& url)
{
    WebrtcStun stun_send;
    WebrtcStun stun_recv;
    UrlParser urlParser;
    urlParser.parse(url);

    stun_send.setType(BindingRequest);
    stun_send.createHeader();
    auto tid = stun_send.getTranscationId();

    _socketStun = make_shared<Socket>(EventLoop::getCurrentLoop());
    _stunAddr = _socketStun->createSocket(urlParser.host_, urlParser.port_, SOCKET_UDP);
    weak_ptr<WebrtcIce> weakThis = shared_from_this();
    _socketStun->setReadCb([tid, weakThis](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) {
        auto thisPtr = weakThis.lock();
        if (!thisPtr) {
            return -1;
        }
        WebrtcStun stun_recv;
        if (stun_recv.parse(buffer->data(), buffer->size())) {
            if (stun_recv.getType() != BindingResponse || stun_recv.getTranscationId() != tid) {
                return -1;
            }
            // 收到绑定响应
            // if (stun_recv.getMappedAddr() != "") {
                thisPtr->_candidates[ICE_CANDIDATE_TYPE_SRFLX] = (stun_recv.getMappedAddr(ICE_CANDIDATE_TYPE_SRFLX));
            // }
        }
        return 0;
    });

    _socketStun->send(stun_send.getBuffer(), 1, 0, 0, (struct sockaddr*)&_stunAddr, sizeof(_stunAddr));
}

void WebrtcIce::gatherTurnAddr(const std::string& url, const std::string& username, const std::string& credential)
{
    int ret = -1;
    uint32_t attr = ntohl(0x11000000);
    Address turn_addr;
    WebrtcStun turn_send;
    WebrtcStun turn_recv;
    UrlParser urlParser;
    urlParser.parse(url);
    
    string tid = randomStr(12);
    turn_send.setType(BindingRequest);
    turn_send.createHeader();

    turn_send.createAttribute(STUN_ATTR_TYPE_REQUESTED_TRANSPORT, sizeof(attr), (char*)&attr);  // UDP
    turn_send.createAttribute(STUN_ATTR_TYPE_USERNAME, username.size(), (char*)username.c_str());

    _socketTurn = make_shared<Socket>(EventLoop::getCurrentLoop());
    _turnAddr = _socketTurn->createSocket(urlParser.host_, urlParser.port_, SOCKET_UDP);
    weak_ptr<WebrtcIce> weakThis = shared_from_this();
    _socketTurn->setReadCb([tid, weakThis, attr, username, credential](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) {
        auto thisPtr = weakThis.lock();
        if (!thisPtr) {
            return -1;
        }
        
        thisPtr->gatherTurnAddr_l(buffer, username, credential);
        return 0;
    });

    _socketTurn->send(turn_send.getBuffer(), 1, 0, 0, (struct sockaddr*)&_turnAddr, sizeof(_turnAddr));
}

void WebrtcIce::gatherTurnAddr_l(const StreamBuffer::Ptr& buffer, const std::string& username, const std::string& credential)
{
    int ret = -1;
    uint32_t attr = ntohl(0x11000000);
    Address turn_addr;
    WebrtcStun turn_send;
    WebrtcStun turn_recv;
    
    // if (turn_recv.parse(buffer->data(), buffer->size())) {
    //     if (turn_recv.getType() != BindingResponse || turn_recv.getTranscationId() != tid) {
    //         return ;
    //     }
    // }
    
    string password = credential;
    turn_send.setType(BindingRequest);
    turn_send.createHeader();
    turn_send.createAttribute(STUN_ATTR_TYPE_REQUESTED_TRANSPORT, sizeof(attr), (char*)&attr);  
     // UDP
    turn_send.createAttribute(STUN_ATTR_TYPE_USERNAME, username.size(), (char*)username.c_str());
    turn_send.createAttribute(STUN_ATTR_TYPE_NONCE, turn_recv.getNonce().size(), (char*)turn_recv.getNonce().c_str());
    turn_send.createAttribute(STUN_ATTR_TYPE_REALM, turn_recv.getRealm().size(), (char*)turn_recv.getRealm().c_str());
    turn_send.finish(STUN_CREDENTIAL_LONG_TERM, password);// UDP
    string tid = turn_send.getTranscationId();

    weak_ptr<WebrtcIce> weakThis = shared_from_this();
    _socketTurn->setReadCb([tid, weakThis, attr](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) {
        auto thisPtr = weakThis.lock();
        if (!thisPtr) {
            return -1;
        }
        WebrtcStun turn_recv;
        if (turn_recv.parse(buffer->data(), buffer->size())) {
            if (turn_recv.getType() != BindingResponse || turn_recv.getTranscationId() != tid) {
                return -1;
            }

            thisPtr->_candidates[ICE_CANDIDATE_TYPE_RELAY] = (turn_recv.getMappedAddr(ICE_CANDIDATE_TYPE_RELAY));
        }
        return 0;
    });

    _socketTurn->send(turn_send.getBuffer(), 1, 0, 0, (struct sockaddr*)&_turnAddr, sizeof(_turnAddr));
}

std::vector<InterfaceInfo> WebrtcIce::getInterfaces()
{
    return _interfaces;
}

Address WebrtcIce::getCandidate(IceCandidateType type)
{
    return *(_candidates[type]);
}

void WebrtcIce::addCandidate(const CandidateInfo::Ptr& candidate)
{
    if (candidate->candidateType_ == "host") {
        _peerHost.push_back(candidate);
    } else if (candidate->candidateType_ == "srflx") {
        _peerCandidates[ICE_CANDIDATE_TYPE_SRFLX] = candidate;
    } else if (candidate->candidateType_ == "prflx") {
        _peerCandidates[ICE_CANDIDATE_TYPE_PRFLX] = candidate;
    } else if (candidate->candidateType_ == "relay") {
        _peerCandidates[ICE_CANDIDATE_TYPE_RELAY] = candidate;
    } else {
        logError << "unknown candidate type: " << candidate->candidateType_;
    }
}

void WebrtcIce::check()
{
    WebrtcStun stun_send;
    WebrtcStun stun_recv;

    weak_ptr<WebrtcIce> weakThis = shared_from_this();

    auto buffer = make_shared<StringBuffer>();
    stun_send.setType(BindingRequest);
    stun_send.createHeader();
    stun_send.toBuffer("", buffer);
    _socketHost->setReadCb([weakThis](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) {
        auto thisPtr = weakThis.lock();
        if (!thisPtr) {
            return 0;
        }

        // TODO activily
        thisPtr->_pairAddr[ICE_CANDIDATE_TYPE_HOST];
        return 0;
    });
    _socketHost->send(buffer, 1, 0, 0, (struct sockaddr*)&_hostAddr, sizeof(_hostAddr));

    // TODO stun check
    _socketStun->setReadCb([weakThis](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) {
        auto thisPtr = weakThis.lock();
        if (!thisPtr) {
            return 0;
        }

        // TODO activily
        thisPtr->_pairAddr[ICE_CANDIDATE_TYPE_HOST];
        return 0;
    });
    _socketStun->send(buffer, 1, 0, 0, (struct sockaddr*)&_stunAddr, sizeof(_stunAddr));

    // TODO turn check
    _socketTurn->setReadCb([weakThis](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) {
        auto thisPtr = weakThis.lock();
        if (!thisPtr) {
            return 0;
        }

        // TODO activily
        thisPtr->_pairAddr[ICE_CANDIDATE_TYPE_HOST];
        return 0;
    });
    _socketTurn->send(buffer, 1, 0, 0, (struct sockaddr*)&_turnAddr, sizeof(_turnAddr));
}

void WebrtcIce::setSocketHost(const Socket::Ptr& socket)
{
    _socketHost = socket;
}
