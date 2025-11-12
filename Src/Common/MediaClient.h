#ifndef MediaClient_H
#define MediaClient_H

#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <functional>

// using namespace std;

enum MediaClientType
{
    MediaClientType_Push = 0,
    MediaClientType_Pull,
    MediaClientType_Both,
};

class MediaClient
{
public:
    using Ptr = std::shared_ptr<MediaClient>;
    virtual bool start(const std::string& localIp, int localPort, const std::string& url, int timeout)
    {
        _localIp = localIp;
        _localPort = localPort;
        _timeout = timeout;
        _url = url;
        return false;
    }
    virtual void stop() {}
    virtual void pause() {}
    virtual void setOnClose(const std::function<void()>& cb) {}
    virtual void addOnReady(void* key, const std::function<void()>& onReady) {}
    virtual void setTransType(int type) {}

    virtual void getProtocolAndType(std::string& protocol, MediaClientType& type) {}
    virtual void setRetryCount(int count) {_retryCount = count;}

public:
    static void addMediaClient(const std::string& key, const MediaClient::Ptr& client);
    static void delMediaClient(const std::string& key);
    static MediaClient::Ptr getMediaClient(const std::string& key);
    static MediaClient::Ptr createClient(const std::string& protocol, const std::string& path, MediaClientType type);
    static std::unordered_map<std::string, MediaClient::Ptr> getAllMediaClient();
    static void for_each_mediaClieant(const std::function<void(const MediaClient::Ptr& client)>& func);

    static void registerCreateClient(const std::string& protocol, const std::function<MediaClient::Ptr(MediaClientType type, const std::string& appName, const std::string& streamName)>& cb);

protected:
    int _retryCount = 0;
    int _localPort;
    int _timeout = 0;
    std::string _localIp;
    std::string _url;

private:
    static std::mutex _mapMtx;
    static std::unordered_map<std::string, MediaClient::Ptr> _mapMediaClient;

    static std::unordered_map<std::string, std::function<MediaClient::Ptr(MediaClientType type, const std::string& appName, const std::string& streamName)>> _mapCreateClient;
};

#endif //MediaClient_H
