#include "WebrtcP2PManager.h"

WebrtcP2PManager::WebrtcP2PManager()
{
}

WebrtcP2PManager::~WebrtcP2PManager()
{
}

WebrtcP2PManager::Ptr WebrtcP2PManager::getInstance()
{
    static WebrtcP2PManager::Ptr instance = std::make_shared<WebrtcP2PManager>();
    return instance;
}

bool WebrtcP2PManager::addP2PSdp(const std::string& roomId, const std::string& userId, const std::string& sdp)
{
    std::lock_guard<std::mutex> lck(_mtx);
    auto it = _p2pManagers.find(roomId);
    _p2pManagers[roomId][userId] = sdp;
    return true;
}

std::string WebrtcP2PManager::getP2PSdp(const std::string& roomId, const std::string& userId)
{
    std::lock_guard<std::mutex> lck(_mtx);
    auto it = _p2pManagers.find(roomId);
    if (it == _p2pManagers.end())
    {
        return "";
    }
    auto it2 = it->second.find(userId);
    if (it2 == it->second.end())
    {
        return "";
    }
    return it2->second;
}

bool WebrtcP2PManager::delP2PSdp(const std::string& roomId, const std::string& userId)
{
    std::lock_guard<std::mutex> lck(_mtx);
    auto it = _p2pManagers.find(roomId);
    if (it == _p2pManagers.end())
    {
        return false;
    }
    auto it2 = it->second.find(userId);
    if (it2 == it->second.end())
    {
        return false;
    }
    it->second.erase(it2);
    return true;
}

bool WebrtcP2PManager::delP2PManager(const std::string& roomId)
{
    std::lock_guard<std::mutex> lck(_mtx);
    auto it = _p2pManagers.find(roomId);
    if (it == _p2pManagers.end())
    {
        return false;
    }
    
    _p2pManagers.erase(it);
    return true;
}