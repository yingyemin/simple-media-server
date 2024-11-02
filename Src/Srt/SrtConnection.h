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


using namespace std;

#ifdef ENABLE_SRT

class SrtConnection : public enable_shared_from_this<SrtConnection>
{
public:
    using Ptr = shared_ptr<SrtConnection>;
    using Wptr = weak_ptr<SrtConnection>;

    SrtConnection(const SrtEventLoop::Ptr& loop, const SrtSocket::Ptr& socket);
    ~SrtConnection();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len);
    void close();
    void init();
    void setOnClose(const function<void()>& cb) {_onClose = cb;}

private:
    void handlePull();
    void onPlayTs(const TsMediaSource::Ptr &tsSrc);
    void initPush();
    void handlePush(const StreamBuffer::Ptr& buffer);
    void onError(const string& err);

private:
    bool _inited = false;
    string _request = "pull";
    UrlParser _urlParser;
    SrtEventLoop::Ptr _loop;
    SrtSocket::Ptr _socket;
    TsMediaSource::Wptr _source;
    TsMediaSource::RingType::DataQueReaderT::Ptr _playTsReader;
    unordered_map<string, string> _mapParam;
    function<void()> _onClose;
};

#endif

#endif //Ehome2Connection_H
