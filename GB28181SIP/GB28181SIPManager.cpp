#include "GB28181SIPManager.h"
#include "Logger.h"
#include "Common/Define.h"

using namespace std;

static thread_local unordered_map<string, GB28181SIPContext::Wptr> _mapContextPerThread;

GB28181SIPManager::GB28181SIPManager()
{}

GB28181SIPManager::~GB28181SIPManager()
{
}

GB28181SIPManager::Ptr& GB28181SIPManager::instance()
{
    static GB28181SIPManager::Ptr instance = make_shared<GB28181SIPManager>();
    return instance;
}

void GB28181SIPManager::init(const EventLoop::Ptr& loop)
{
    // _loop = EventLoop::getCurrentLoop();
    weak_ptr<GB28181SIPManager> wSelf = shared_from_this();
    loop->addTimerTask(5000, [wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return 0;
        }

        self->heartbeat();
        return 5000;
    }, [wSelf](bool success, shared_ptr<TimerTask>){

    });
}

void GB28181SIPManager::onSipPacket(const Socket::Ptr& socket, const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    logTrace << "get a message: " << buffer->data();
    shared_ptr<SipRequest> req;
    _sipStack.parse_request(req, buffer->data(), buffer->size());

    string deviceId;
    // if (req->cmdtype == SipCmdRequest) {
        deviceId = req->sip_username;
    // }

    logInfo << "get a deviceId: " << deviceId;

    // auto ssrc = req->sip_channel_id;
    auto iter = _mapContextPerThread.find(deviceId);
    if (iter != _mapContextPerThread.end())
    {
        auto context = iter->second.lock();
        if (context) {
            if (context->isAlive()) {
                context->onSipPacket(socket, req, addr, len, true);
            } else {
                auto key = iter->first;
                _mapContextPerThread.erase(iter);
                lock_guard<mutex> loc(_contextLck);
                _mapContext.erase(key);
            }
            return ;
        } else {
            _mapContextPerThread.erase(iter);
        }
    }

    {
        lock_guard<mutex> lock(_contextLck);
        auto iterAll = _mapContext.find(deviceId);
        if (iterAll != _mapContext.end())
        {
            auto context = iterAll->second;
            if (context->isAlive()) {
                context->onSipPacket(socket, req, addr, len, true);
                _mapContextPerThread[deviceId] = context;
            } else {
                _mapContext.erase(iterAll);
            }
            return ;
        }
    }

    // string uri = "/live/" + to_string(deviceId);
    auto context = make_shared<GB28181SIPContext>(EventLoop::getCurrentLoop(), deviceId, 
                                        DEFAULT_VHOST, PROTOCOL_GB28181, DEFAULT_TYPE);

    if (!context->init()) {
        return ;
    }
    context->onSipPacket(socket, req, addr, len, true);
    logInfo << "_mapContextPerThread, deviceId: " << deviceId;
    _mapContextPerThread[deviceId] = context;
    lock_guard<mutex> lock(_contextLck);
    _mapContext[deviceId] = context;

}

void GB28181SIPManager::heartbeat()
{
    for (auto iter = _mapContextPerThread.begin(); iter != _mapContextPerThread.end();) {
        auto context = iter->second.lock();
        if (!context || !context->isAlive()) {
            logInfo << "_mapContextPerThread.erase, deviceId: " << iter->first;
            auto deviceId = iter->first;
            iter = _mapContextPerThread.erase(iter);
            lock_guard<mutex> lock(_contextLck);
            _mapContext.erase(deviceId);
        } else {
            ++iter;
        }
    }
}

void GB28181SIPManager::addContext(const string& deviceId, const GB28181SIPContext::Ptr& context)
{
    lock_guard<mutex> lock(_contextLck);
    _mapContext[deviceId] = context;
}

void GB28181SIPManager::delContext(const string& deviceId)
{
    lock_guard<mutex> lock(_contextLck);
    _mapContext.erase(deviceId);
}

GB28181SIPContext::Ptr GB28181SIPManager::getContext(const string& deviceId)
{
    lock_guard<mutex> lock(_contextLck);
    auto iter = _mapContext.find(deviceId);
    if (iter == _mapContext.end()) 
    {
        return nullptr;
    }
    return iter->second;
}

