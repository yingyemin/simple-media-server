#include "GB28181Manager.h"
#include "Logger.h"
#include "Common/Define.h"

using namespace std;

static thread_local unordered_map<uint32_t, GB28181Context::Wptr> _mapContextPerThread;

GB28181Manager::GB28181Manager()
{}

GB28181Manager::~GB28181Manager()
{
}

GB28181Manager::Ptr& GB28181Manager::instance()
{
    static GB28181Manager::Ptr instance = make_shared<GB28181Manager>();
    return instance;
}

void GB28181Manager::init(const EventLoop::Ptr& loop)
{
    // _loop = EventLoop::getCurrentLoop();
    weak_ptr<GB28181Manager> wSelf = shared_from_this();
    loop->addTimerTask(5000, [wSelf](){
        auto self = wSelf.lock();
        if (!self) {
            return -1;
        }

        self->heartbeat();
        return 5000;
    }, [wSelf](bool success, shared_ptr<TimerTask>){

    });
}

void GB28181Manager::onRtpPacket(const RtpPacket::Ptr& rtp, struct sockaddr* addr, int len)
{
    auto ssrc = rtp->getSSRC();
    auto iter = _mapContextPerThread.find(ssrc);
    if (iter != _mapContextPerThread.end())
    {
        auto context = iter->second.lock();
        if (context) {
            if (context->isAlive()) {
                context->onRtpPacket(rtp, addr, len, true);
            } else {
                _mapContextPerThread.erase(iter);
                lock_guard<mutex> loc(_contextLck);
                _mapContext.erase(iter->first);
            }
            return ;
        } else {
            _mapContextPerThread.erase(iter);
        }
    }

    {
        lock_guard<mutex> lock(_contextLck);
        auto iterAll = _mapContext.find(ssrc);
        if (iterAll != _mapContext.end())
        {
            auto context = iterAll->second;
            if (context->isAlive()) {
                context->onRtpPacket(rtp, addr, len, true);
                _mapContextPerThread[ssrc] = context;
            } else {
                _mapContext.erase(iterAll);
            }
            return ;
        }
    }

    string uri = "/live/" + to_string(ssrc);
    auto context = make_shared<GB28181Context>(EventLoop::getCurrentLoop(), uri, 
                                        DEFAULT_VHOST, PROTOCOL_GB28181, DEFAULT_TYPE);

    if (!context->init()) {
        return ;
    }
    context->onRtpPacket(rtp, addr, len, true);
    logInfo << "_mapContextPerThread, ssrc: " << ssrc;
    _mapContextPerThread[ssrc] = context;
    lock_guard<mutex> lock(_contextLck);
    _mapContext[ssrc] = context;

}

void GB28181Manager::heartbeat()
{
    for (auto iter = _mapContextPerThread.begin(); iter != _mapContextPerThread.end();) {
        auto context = iter->second.lock();
        if (!context || !context->isAlive()) {
            logInfo << "_mapContextPerThread.erase, ssrc: " << iter->first;
            auto ssrc = iter->first;
            iter = _mapContextPerThread.erase(iter);
            lock_guard<mutex> lock(_contextLck);
            _mapContext.erase(ssrc);
        } else {
            ++iter;
        }
    }
}

void GB28181Manager::addContext(uint32_t ssrc, const GB28181Context::Ptr& context)
{
    lock_guard<mutex> lock(_contextLck);
    _mapContext[ssrc] = context;
}

void GB28181Manager::delContext(uint32_t ssrc)
{
    lock_guard<mutex> lock(_contextLck);
    _mapContext.erase(ssrc);
}

