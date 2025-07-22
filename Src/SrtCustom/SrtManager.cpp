#include "SrtManager.h"
#include "Logger.h"
#include "Common/Define.h"

using namespace std;

static thread_local unordered_map<uint32_t, SrtContext::Wptr> _mapContextPerThread;

SrtManager::SrtManager()
{}

SrtManager::~SrtManager()
{
}

SrtManager::Ptr& SrtManager::instance()
{
    static SrtManager::Ptr instance = make_shared<SrtManager>();
    return instance;
}

void SrtManager::init(const EventLoop::Ptr& loop)
{
    // _loop = EventLoop::getCurrentLoop();
    weak_ptr<SrtManager> wSelf = shared_from_this();
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

void SrtManager::onSrtPacket(const Socket::Ptr& socket, const Buffer::Ptr& buffer, struct sockaddr* addr, int len)
{
    uint32_t ukey;
    if (DataPacket::isDataPacket((uint8_t*)buffer->data(), buffer->size())) {
        logTrace << "get a data packet";
        ukey = DataPacket::getDstSocketID((uint8_t*)buffer->data(), buffer->size());
    } else {
        logTrace << "get a control packet";
        if (HandshakePacket::isHandshakePacket((uint8_t*)buffer->data(), buffer->size())) {
            auto type = HandshakePacket::getHandshakeType((uint8_t*)buffer->data(), buffer->size());
            logTrace << "get a control packet, type: " << type;
            if (type == HS_TYPE_INDUCTION) {
                logTrace << "get a induction packet";
                string uri = "/live/";
                auto context = make_shared<SrtContext>(EventLoop::getCurrentLoop(), uri, 
                                                    DEFAULT_VHOST, PROTOCOL_SRT, DEFAULT_TYPE);

                // if (!context->init()) {
                //     return ;
                // }
                context->onSrtPacket(socket, buffer, addr, len, true);

                // _mapContextPerThread[socketId] = context;
                // lock_guard<mutex> lock(_contextLck);
                // _mapContext[socketId] = context;

                return ;
            } else if (type == HS_TYPE_CONCLUSION) {
                logTrace << "get a conclusion packet";
                ukey = HandshakePacket::getSynCookie((uint8_t*)buffer->data(), buffer->size());
            } else {
                logTrace << "get a unknown packet";
                ukey = ControlPacket::getDstSocketID((uint8_t*)buffer->data(), buffer->size());
            }
        } else {
            ukey = ControlPacket::getDstSocketID((uint8_t*)buffer->data(), buffer->size());
        }
    }
    // ControlPacket::Ptr pkt = make_shared<ControlPacket>();
    // pkt->parse((uint8_t*)buffer->data(), buffer->size());
    // auto socketId = pkt->getDstSocketId();
    auto iter = _mapContextPerThread.find(ukey);
    if (iter != _mapContextPerThread.end())
    {
        auto context = iter->second.lock();
        if (context) {
            if (context->isAlive()) {
                context->onSrtPacket(socket, buffer, addr, len, true);
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

    SrtContext::Ptr context;
    {
        lock_guard<mutex> lock(_contextLck);
        auto iterAll = _mapContext.find(ukey);
        if (iterAll != _mapContext.end())
        {
            context = iterAll->second;
            if (!context->isAlive()) {
                _mapContext.erase(iterAll);
            }
            // return ;
        }
    }

    if (context && context->isAlive()) {
        context->onSrtPacket(socket, buffer, addr, len, true);
        _mapContextPerThread[ukey] = context;
    }
}

void SrtManager::heartbeat()
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

void SrtManager::addContext(uint32_t ssrc, const SrtContext::Ptr& context)
{
    lock_guard<mutex> lock(_contextLck);
    _mapContext[ssrc] = context;
}

void SrtManager::delContext(uint32_t ssrc)
{
    lock_guard<mutex> lock(_contextLck);
    _mapContext.erase(ssrc);
}

