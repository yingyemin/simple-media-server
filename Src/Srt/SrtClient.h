#ifndef SrtClient_H
#define SrtClient_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Net/SrtSocket.h"
#include "Common/UrlParser.h"
#include "Common/MediaClient.h"
#include "Mpeg/TsMediaSource.h"


// using namespace std;

#ifdef ENABLE_SRT

class SrtClient : public MediaClient, public std::enable_shared_from_this<SrtClient>
{
public:
    using Ptr = std::shared_ptr<SrtClient>;
    using Wptr = std::weak_ptr<SrtClient>;

    SrtClient(MediaClientType type, const std::string& appName, const std::string& streamName);
    ~SrtClient();

    std::string getPath() {return _urlParser.path_;}
    std::string getSourceUrl() {return _url;}
    void getProtocolAndType(std::string& protocol, MediaClientType& type);

public:
    // override MediaClient
    bool start(const std::string& localIp, int localPort, const std::string& url, int timeout) override;
    void stop() override;
    void pause() override;
    void setOnClose(const std::function<void()>& cb) override;

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len);
    void close();

private:
    void handlePush();
    void onPushTs(const TsMediaSource::Ptr &tsSrc);
    void initPull();
    void handlePull(const StreamBuffer::Ptr& buffer);
    void onError(const std::string& err);

private:
    bool _inited = false;
    std::string _request = "pull";
    std::string _url;
    UrlParser _urlParser;
    UrlParser _peerUrlParser;
    SrtEventLoop::Ptr _loop;
    SrtSocket::Ptr _socket;
    SrtSocket::Ptr _acceptSocket;
    TsMediaSource::Wptr _source;
    TsMediaSource::RingType::DataQueReaderT::Ptr _playTsReader;
    std::function<void()> _onClose;
    std::unordered_map<std::string, std::string> _mapParam;
};

#endif

#endif //Ehome2Connection_H
