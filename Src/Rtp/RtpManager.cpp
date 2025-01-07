#include "RtpManager.h"
#include "Logger.h"
#include "Common/Define.h"

using namespace std;

static thread_local unordered_map<uint32_t, RtpContext::Wptr> _mapContextPerThread;

RtpManager::RtpManager()
{}

RtpManager::~RtpManager()
{
}

RtpManager::Ptr& RtpManager::instance()
{
    static RtpManager::Ptr instance = make_shared<RtpManager>();
    return instance;
}

void RtpManager::init(const EventLoop::Ptr& loop)
{
    // _loop = EventLoop::getCurrentLoop();
    weak_ptr<RtpManager> wSelf = shared_from_this();
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

void RtpManager::onRtpPacket(const RtpPacket::Ptr& rtp, struct sockaddr* addr, int len)
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
    auto context = make_shared<RtpContext>(EventLoop::getCurrentLoop(), uri, 
                                        DEFAULT_VHOST, PROTOCOL_RTP, DEFAULT_TYPE);

    if (!context->init()) {
        return ;
    }
    context->onRtpPacket(rtp, addr, len, true);
    logInfo << "_mapContextPerThread, ssrc: " << ssrc;
    _mapContextPerThread[ssrc] = context;
    lock_guard<mutex> lock(_contextLck);
    _mapContext[ssrc] = context;

}

void RtpManager::heartbeat()
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

void RtpManager::addContext(uint32_t ssrc, const RtpContext::Ptr& context)
{
    lock_guard<mutex> lock(_contextLck);
    _mapContext[ssrc] = context;
}

void RtpManager::delContext(uint32_t ssrc)
{
    lock_guard<mutex> lock(_contextLck);
    _mapContext.erase(ssrc);
}

