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
    virtual void start(const string& localIp, int localPort, const string& url, int timeout) {}
    virtual void stop() {}
    virtual void pause() {}
    virtual void setOnClose(const function<void()>& cb) {}

public:
    static void addMediaClient(const string& key, const MediaClient::Ptr& client);
    static void delMediaClient(const string& key);
    static MediaClient::Ptr getMediaClient(const string& key);

private:
    static mutex _mapMtx;
    static unordered_map<string, MediaClient::Ptr> _mapMediaClient;
};

#endif //MediaClient_H
