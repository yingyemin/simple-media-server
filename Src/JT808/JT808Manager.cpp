#include "JT808Manager.h"
#include "Logger.h"
#include "Common/Define.h"

using namespace std;

static thread_local unordered_map<string, JT808Context::Wptr> _mapContextPerThread;

JT808Manager::JT808Manager()
{}

JT808Manager::~JT808Manager()
{
}

JT808Manager::Ptr& JT808Manager::instance()
{
    static JT808Manager::Ptr instance = make_shared<JT808Manager>();
    return instance;
}

void JT808Manager::init(const EventLoop::Ptr& loop)
{
    // _loop = EventLoop::getCurrentLoop();
    weak_ptr<JT808Manager> wSelf = shared_from_this();
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

void JT808Manager::onJT808Packet(const Socket::Ptr& socket, const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    // TODO 解析808 simcode

    string deviceId;
    JT808Packet::Ptr packet = make_shared<JT808Packet>();
    // if (req->cmdtype == SipCmdRequest) {
        // deviceId = req->sip_username;
    // }

    logInfo << "get a deviceId: " << deviceId;

    // auto ssrc = req->sip_channel_id;
    auto iter = _mapContextPerThread.find(deviceId);
    if (iter != _mapContextPerThread.end())
    {
        auto context = iter->second.lock();
        if (context) {
            if (context->isAlive()) {
                context->onJT808Packet(socket, packet, addr, len, true);
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
                context->onJT808Packet(socket, packet, addr, len, true);
                _mapContextPerThread[deviceId] = context;
            } else {
                _mapContext.erase(iterAll);
            }
            return ;
        }
    }

    // string uri = "/live/" + to_string(deviceId);
    auto context = make_shared<JT808Context>(EventLoop::getCurrentLoop(), deviceId);

    if (!context->init()) {
        return ;
    }
    context->onJT808Packet(socket, packet, addr, len, true);
    logInfo << "_mapContextPerThread, deviceId: " << deviceId;
    _mapContextPerThread[deviceId] = context;
    lock_guard<mutex> lock(_contextLck);
    _mapContext[deviceId] = context;

}

void JT808Manager::heartbeat()
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

void JT808Manager::addContext(const string& deviceId, const JT808Context::Ptr& context)
{
    lock_guard<mutex> lock(_contextLck);
    _mapContext[deviceId] = context;
}

void JT808Manager::delContext(const string& deviceId)
{
    lock_guard<mutex> lock(_contextLck);
    _mapContext.erase(deviceId);
}

JT808Context::Ptr JT808Manager::getContext(const string& deviceId)
{
    lock_guard<mutex> lock(_contextLck);
    auto iter = _mapContext.find(deviceId);
    if (iter == _mapContext.end()) 
    {
        return nullptr;
    }
    return iter->second;
}

unordered_map<string, JT808Context::Ptr> JT808Manager::getContexts()
{
    lock_guard<mutex> lock(_contextLck);
    return _mapContext;
}
