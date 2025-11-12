#ifndef SrtConnection_H
#define SrtConnection_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Net/SrtSocket.h"
#include "Common/UrlParser.h"
#include "Mpeg/TsMediaSource.h"


// using namespace std;

#ifdef ENABLE_SRT

class SrtConnection : public std::enable_shared_from_this<SrtConnection>
{
public:
    using Ptr = std::shared_ptr<SrtConnection>;
    using Wptr = std::weak_ptr<SrtConnection>;

    SrtConnection(const SrtEventLoop::Ptr& loop, const SrtSocket::Ptr& socket);
    ~SrtConnection();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len);
    void close();
    void init();
    void setOnClose(const std::function<void()>& cb) {_onClose = cb;}

private:
    void handlePull();
    void onPlayTs(const TsMediaSource::Ptr &tsSrc);
    void initPush();
    void handlePush(const StreamBuffer::Ptr& buffer);
    void onError(const std::string& err);

private:
    bool _inited = false;
    int _index = 0;
    FILE* _fp;
    std::string _request = "pull";
    UrlParser _urlParser;
    SrtEventLoop::Ptr _loop;
    SrtSocket::Ptr _socket;
    TsMediaSource::Wptr _source;
    TsMediaSource::RingType::DataQueReaderT::Ptr _playTsReader;
    std::unordered_map<std::string, std::string> _mapParam;
    std::function<void()> _onClose;
};

#endif

#endif //Ehome2Connection_H
