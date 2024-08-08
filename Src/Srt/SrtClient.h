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


using namespace std;

class SrtClient : public MediaClient, public enable_shared_from_this<SrtClient>
{
public:
    using Ptr = shared_ptr<SrtClient>;
    using Wptr = weak_ptr<SrtClient>;

    SrtClient(MediaClientType type, const string& appName, const string& streamName);
    ~SrtClient();

public:
    // override MediaClient
    void start(const string& localIp, int localPort, const string& param, int timeout) override;
    void stop() override;
    void pause() override;
    void setOnClose(const function<void()>& cb) override;

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len);
    void close();

private:
    void handlePush();
    void onPushTs(const TsMediaSource::Ptr &tsSrc);
    void initPull();
    void handlePull(const StreamBuffer::Ptr& buffer);
    void onError(const string& err);

private:
    bool _inited = false;
    string _request = "pull";
    UrlParser _urlParser;
    UrlParser _peerUrlParser;
    SrtEventLoop::Ptr _loop;
    SrtSocket::Ptr _socket;
    TsMediaSource::Wptr _source;
    TsMediaSource::RingType::DataQueReaderT::Ptr _playTsReader;
    function<void()> _onClose;
    unordered_map<string, string> _mapParam;
};


#endif //Ehome2Connection_H
