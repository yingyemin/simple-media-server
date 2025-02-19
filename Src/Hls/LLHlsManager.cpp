#include "LLHlsManager.h"
#include "Log/Logger.h"

LLHlsManager::LLHlsManager()
{}

LLHlsManager::~LLHlsManager()
{}

LLHlsManager::Ptr& LLHlsManager::instance()
{
    static LLHlsManager::Ptr instance = make_shared<LLHlsManager>();
    return instance;
}

// void LLHlsManager::addMuxer(int uid, const LLHlsMuxer::Ptr& muxer)
// {
//     logInfo << "add muxer: " << uid;

//     lock_guard<mutex> lck(_muxerMtx);
//     _mapMuxer.emplace(uid, muxer);
// }

// LLHlsMuxer::Ptr LLHlsManager::getMuxer(int uid)
// {
//     logInfo << "get muxer: " << uid;
    
//     lock_guard<mutex> lck(_muxerMtx);
//     auto it = _mapMuxer.find(uid);
//     if (it != _mapMuxer.end()) {
//         return it->second;
//     }

//     return nullptr;
// }

// void LLHlsManager::delMuxer(int uid)
// {
//     logInfo << "del muxer: " << uid;
    
//     lock_guard<mutex> lck(_muxerStrMtx);
//     _mapMuxer.erase(uid);
// }

void LLHlsManager::addMuxer(const string& key, const LLHlsMuxer::Ptr& muxer)
{
    logInfo << "add muxer: " << key;

    lock_guard<mutex> lck(_muxerStrMtx);
    _mapStrMuxer.emplace(key, muxer);
}

LLHlsMuxer::Ptr LLHlsManager::getMuxer(const string& key)
{
    logInfo << "get muxer: " << key;
    
    lock_guard<mutex> lck(_muxerStrMtx);
    auto it = _mapStrMuxer.find(key);
    if (it != _mapStrMuxer.end()) {
        return it->second;
    }

    return nullptr;
}

void LLHlsManager::delMuxer(const string& key)
{
    logInfo << "del muxer: " << key;
    
    lock_guard<mutex> lck(_muxerStrMtx);
    _mapStrMuxer.erase(key);
}