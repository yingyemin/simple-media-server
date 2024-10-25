#include "WebrtcContextManager.h"
#include "Logger.h"
#include "Common/Define.h"
#include "Net/Socket.h"
#include "WebrtcStun.h"
#include "Webrtc.h"

using namespace std;

static thread_local unordered_map<uint64_t, WebrtcContext::Wptr> _mapContextPerThread;

uint64_t sockAddrHash(struct sockaddr_in* saddr) {
	return ntohl(saddr->sin_addr.s_addr) << 32 + ntohs(saddr->sin_port);
}

WebrtcContextManager::WebrtcContextManager()
{}

WebrtcContextManager::~WebrtcContextManager()
{
}

WebrtcContextManager::Ptr& WebrtcContextManager::instance()
{
    static WebrtcContextManager::Ptr instance = make_shared<WebrtcContextManager>();
    return instance;
}

void WebrtcContextManager::init(const EventLoop::Ptr& loop)
{
    // _loop = EventLoop::getCurrentLoop();
    weak_ptr<WebrtcContextManager> wSelf = shared_from_this();
    loop->addTimerTask(5000, [wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return -1;
        }

        self->heartbeat();
        return 5000;
    }, [wSelf](bool success, shared_ptr<TimerTask>){

    });

    WebrtcContext::initDtlsCert();
}

void WebrtcContextManager::onUdpPacket(const Socket::Ptr& socket, const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    int pktType = guessType(buffer);
    switch(pktType) {
        case kStunPkt: {
            onStunPacket(socket, buffer, addr, len);
            break;
        }
        case kDtlsPkt: {
            onDtlsPacket(socket, buffer, addr, len);
            break;
        }
        case kRtpPkt: {
            auto rtp = make_shared<WebrtcRtpPacket>(buffer, 0);
            onRtpPacket(socket, rtp, addr, len);
            break;
        }
        case kRtcpPkt: {
            onRtcpPacket(socket, buffer, addr, len);
            break;
        }
        default: {
            logWarn << "unknown pkt type: " << pktType;
            break;
        }
    }
}

void WebrtcContextManager::onDtlsPacket(const Socket::Ptr& socket, const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
	uint64_t hash = sockAddrHash((struct sockaddr_in*)addr);
	logInfo << "on dtls packet, size is [" << buffer->size() << "]";
    
	auto context = getContext(hash);
	if (context) {
        if (socket->getLoop() != context->getLoop()) {
            logInfo << "changeLoop ========= ";
            // context->changeLoop(socket->getLoop());
            return ;
        }

		context->onDtlsPacket(socket, buffer, addr, len);
    }
}

void WebrtcContextManager::onRtcpPacket(const Socket::Ptr& socket, const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
	//logInfo << "on rtcp packet. size is [" << buf->size() << "]";
	uint64_t hash = sockAddrHash((struct sockaddr_in*)addr);
    
	auto context = getContext(hash);
	if (context) {
        if (socket->getLoop() != context->getLoop()) {
            logInfo << "changeLoop ========= ";
            return ;
        }
		context->onRtcpPacket(socket, buffer, addr, len);
    }
}


void WebrtcContextManager::onStunPacket(const Socket::Ptr& socket, const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
	WebrtcStun stunReq;
	if (0 != stunReq.parse(buffer->data(), buffer->size())) {
		logError << "parse stun packet failed";
		return;
	}

	std::string username = stunReq.getUsername();
	auto context = getContext(username);

	if (context) {
        if (context->getLoop() && socket->getLoop() != context->getLoop()) {
            logInfo << "changeLoop ========= ";
            return ;
        }
        context->onStunPacket(socket, stunReq, addr, len);

		uint64_t hash = sockAddrHash((struct sockaddr_in*)addr);
		std::lock_guard<std::mutex> lock(_addrToContextLck);
		auto it = _mapAddrToContext.find(hash);
		if (it == _mapAddrToContext.end()) {
			_mapAddrToContext[hash] = context;	
        }
	} else {
        logWarn << "find contex failed by username: " << username;
    }
}

void WebrtcContextManager::onRtpPacket(const Socket::Ptr& socket, const RtpPacket::Ptr& rtp, struct sockaddr* addr, int len)
{
    uint64_t hash = sockAddrHash((struct sockaddr_in*)addr);
    auto context = getContext(hash);
	if (context) {
        if (socket->getLoop() != context->getLoop()) {
            logInfo << "changeLoop ========= ";
            return ;
        }
		context->onRtpPacket(socket, rtp, addr, len);
    }
}

void WebrtcContextManager::heartbeat()
{
    for (auto iter = _mapContextPerThread.begin(); iter != _mapContextPerThread.end();) {
        auto context = iter->second.lock();
        if (!context || !context->isAlive()) {
            logInfo << "_mapContextPerThread.erase, hash: " << iter->first;
            auto hash = iter->first;
            iter = _mapContextPerThread.erase(iter);
            {
                lock_guard<mutex> lock(_addrToContextLck);
                _mapAddrToContext.erase(hash);
            }

            lock_guard<mutex> lock(_contextLck);
            if (context) {
                _mapContext.erase(context->getUsername());
            }
        } else {
            ++iter;
        }
    }
}

void WebrtcContextManager::addContext(const string& key, const WebrtcContext::Ptr& context)
{
    logDebug << "add context: " << key;

    lock_guard<mutex> lock(_contextLck);
    _mapContext[key] = context;
}

WebrtcContext::Ptr WebrtcContextManager::getContext(const string& key)
{
    logDebug << "get context: " << key;

    lock_guard<mutex> lock(_contextLck);
    if (_mapContext.find(key) == _mapContext.end()) {
        return nullptr;
    }
    return _mapContext[key];
}

void WebrtcContextManager::delContext(const string& key)
{
    logDebug << "del context: " << key;

    lock_guard<mutex> lock(_contextLck);
    _mapContext.erase(key);
}

void WebrtcContextManager::delContext(uint64_t hash)
{
    logDebug << "del context: " << hash;

    lock_guard<mutex> loc(_contextLck);
    _mapAddrToContext.erase(hash);
}

WebrtcContext::Ptr WebrtcContextManager::getContext(uint64_t hash)
{
    WebrtcContext::Ptr delContex;
    auto iter = _mapContextPerThread.find(hash);
    if (iter != _mapContextPerThread.end())
    {
        auto context = iter->second.lock();
        if (context) {
            if (context->isAlive()) {
                return context;
            } else {
                auto key = iter->first;
                delContex = context;
                _mapContextPerThread.erase(iter);
                lock_guard<mutex> loc(_contextLck);
                _mapAddrToContext.erase(key);

                // return nullptr;
            }
        } else {
            _mapContextPerThread.erase(iter);
        }
    }

    if (delContex) {
        lock_guard<mutex> lock(_contextLck);
        _mapContext.erase(delContex->getUsername());
        return nullptr;
    }

    {
        lock_guard<mutex> lock(_addrToContextLck);
        auto iterAll = _mapAddrToContext.find(hash);
        if (iterAll != _mapAddrToContext.end())
        {
            auto context = iterAll->second;
            if (context->isAlive()) {
                _mapContextPerThread[hash] = context;
                return context;
            } else {
                _mapAddrToContext.erase(iterAll);
                delContex = context;
                // return nullptr;
            }
        }
    }

    if (delContex) {
        lock_guard<mutex> lock(_contextLck);
        _mapContext.erase(delContex->getUsername());
    }
    return nullptr;
}

