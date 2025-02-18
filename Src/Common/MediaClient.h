#ifndef MediaClient_H
#define MediaClient_H

#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <functional>

using namespace std;

enum MediaClientType
{
    MediaClientType_Push = 0,
    MediaClientType_Pull
};

class MediaClient
{
public:
    using Ptr = shared_ptr<MediaClient>;
    virtual bool start(const string& localIp, int localPort, const string& url, int timeout) {return false;}
    virtual void stop() {}
    virtual void pause() {}
    virtual void setOnClose(const function<void()>& cb) {}
    virtual void addOnReady(void* key, const function<void()>& onReady) {}
    virtual void setTransType(int type) {}

    virtual void getProtocolAndType(string& protocol, MediaClientType& type) {}

public:
    static void addMediaClient(const string& key, const MediaClient::Ptr& client);
    static void delMediaClient(const string& key);
    static MediaClient::Ptr getMediaClient(const string& key);
    static MediaClient::Ptr createClient(const string& protocol, const string& path, MediaClientType type);
    static unordered_map<string, MediaClient::Ptr> getAllMediaClient();
    static void for_each_mediaClieant(const function<void(const MediaClient::Ptr& client)>& func);

    static void registerCreateClient(const string& protocol, const function<MediaClient::Ptr(MediaClientType type, const string& appName, const string& streamName)>& cb);

private:
    static mutex _mapMtx;
    static unordered_map<string, MediaClient::Ptr> _mapMediaClient;

    static unordered_map<string, function<MediaClient::Ptr(MediaClientType type, const string& appName, const string& streamName)>> _mapCreateClient;
};

#endif //MediaClient_H
