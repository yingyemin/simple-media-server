#include "MediaClient.h"
#include "Log/Logger.h"
#include "Util/String.h"

mutex MediaClient::_mapMtx;
unordered_map<string, MediaClient::Ptr> MediaClient::_mapMediaClient;

unordered_map<string, function<MediaClient::Ptr(MediaClientType type, const string& appName, const string& streamName)>> MediaClient::_mapCreateClient;

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

unordered_map<string, MediaClient::Ptr> MediaClient::getAllMediaClient()
{
    lock_guard<mutex> lck(_mapMtx);
    return _mapMediaClient;
}

void MediaClient::for_each_mediaClieant(const function<void(const MediaClient::Ptr& client)>& func)
{
    lock_guard<mutex> lck(_mapMtx);
    for (auto& pr : _mapMediaClient) {
        func(pr.second);
    }
}

MediaClient::Ptr MediaClient::createClient(const string& protocol, const string& path, MediaClientType type)
{
    auto vecPath = split(path, "/");
    if (_mapCreateClient.find(protocol) != _mapCreateClient.end()) {
        auto client = _mapCreateClient[protocol](type, vecPath[0], vecPath[1]);
        client->setTransType(0);

        return client;
    }

    return nullptr;
}

void MediaClient::registerCreateClient(const string& protocol, const function<MediaClient::Ptr(MediaClientType type, const string& appName, const string& streamName)>& cb)
{
    _mapCreateClient.emplace(protocol, cb);
}