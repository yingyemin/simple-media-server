#include "HlsManager.h"
#include "Log/Logger.h"

HlsManager::HlsManager()
{}

HlsManager::~HlsManager()
{}

HlsManager::Ptr& HlsManager::instance()
{
    static HlsManager::Ptr instance = make_shared<HlsManager>();
    return instance;
}

// void HlsManager::addMuxer(int uid, const HlsMuxer::Ptr& muxer)
// {
//     logInfo << "add muxer: " << uid;

//     lock_guard<mutex> lck(_muxerMtx);
//     _mapMuxer.emplace(uid, muxer);
// }

// HlsMuxer::Ptr HlsManager::getMuxer(int uid)
// {
//     logInfo << "get muxer: " << uid;
    
//     lock_guard<mutex> lck(_muxerMtx);
//     auto it = _mapMuxer.find(uid);
//     if (it != _mapMuxer.end()) {
//         return it->second;
//     }

//     return nullptr;
// }

// void HlsManager::delMuxer(int uid)
// {
//     logInfo << "del muxer: " << uid;
    
//     lock_guard<mutex> lck(_muxerMtx);
//     _mapMuxer.erase(uid);
// }

void HlsManager::addMuxer(const string& key, const HlsMuxer::Ptr& muxer)
{
    logDebug << "add muxer: " << key;

    lock_guard<mutex> lck(_muxerStrMtx);
    _mapStrMuxer.emplace(key, muxer);
}

HlsMuxer::Ptr HlsManager::getMuxer(const string& key)
{
    logDebug << "get muxer: " << key;
    
    lock_guard<mutex> lck(_muxerStrMtx);
    auto it = _mapStrMuxer.find(key);
    if (it != _mapStrMuxer.end()) {
        return it->second;
    }

    return nullptr;
}

void HlsManager::delMuxer(const string& key)
{
    logDebug << "del muxer: " << key;
    
    lock_guard<mutex> lck(_muxerStrMtx);
    _mapStrMuxer.erase(key);
}