#include "MediaClient.h"
#include "Log/Logger.h"
#include "Rtmp/RtmpClient.h"
#include "Rtsp/RtspClient.h"
#include "Webrtc/WebrtcClient.h"
#include "Util/String.h"

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

MediaClient::Ptr MediaClient::createClient(const string& protocol, const string& path, MediaClientType type)
{
    auto vecPath = split(path, "/");
    if (protocol == "rtmp") {
        return make_shared<RtmpClient>(type, vecPath[0], vecPath[1]);
    } else if (protocol == "rtsp") {
        return make_shared<RtspClient>(type, Transport_TCP, vecPath[0], vecPath[1]);
    } else if (protocol == "webrtc") {
        return make_shared<WebrtcClient>(type, vecPath[0], vecPath[1]);
    }

    return nullptr;
}