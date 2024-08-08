#include "MediaClient.h"
#include "Log/Logger.h"

mutex MediaClient::_mapMtx;
unordered_map<string, MediaClient::Ptr> MediaClient::_mapMediaClient;

void MediaClient::addMediaClient(const string& key, const MediaClient::Ptr& client)
{
    logInfo << "add MediaClient: " << key;
    lock_guard<mutex> lck(_mapMtx);
    _mapMediaClient.emplace(key, client);
}

void MediaClient::delMediaClient(const string& key)
{
    logInfo << "del MediaClient: " << key;
    lock_guard<mutex> lck(_mapMtx);
    _mapMediaClient.erase(key);
}

MediaClient::Ptr MediaClient::getMediaClient(const string& key)
{
    lock_guard<mutex> lck(_mapMtx);
    auto it = _mapMediaClient.find(key);
    if (it != _mapMediaClient.end()) {
        return it->second;
    }
    
    return nullptr;
}