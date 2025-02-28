#ifndef JT1078Connection_H
#define JT1078Connection_H

#include <unordered_map>
#include <string>
#include <memory>
#include <vector>

#include "Net/Buffer.h"
#include "Net/TcpConnection.h"
#include "Rtp/RtpPacket.h"
#include "JT1078Parser.h"
#include "JT1078DecodeTrack.h"
#include "JT1078MediaSource.h"
#include "Util/TimeClock.h"

using namespace std;

class JT1078Info
{
public:
    string appName = "live";
    string streamName;
};

class JT1078TalkInfo
{
public:
    string simCode;
    int channel;
    UrlParser urlParser;
};

class JT1078Connection : public TcpConnection
{
public:
    using Ptr = shared_ptr<JT1078Connection>;
    using Wptr = weak_ptr<JT1078Connection>;

    JT1078Connection(const EventLoop::Ptr& loop, const Socket::Ptr& socket);
    ~JT1078Connection();

public:
    // 继承自tcpseesion
    void onRead(const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len) override;
    void onError() override;
    void onManager() override;
    void init() override;
    void close() override;
    ssize_t send(Buffer::Ptr pkt) override;

    void setPath(const string& path) {_path = path;}
    void setAppName(const string& appName) {_app = appName;}
    void setTalkFlag() {_isTalk = true;}
    void onRtpPacket(const JT1078RtpPacket::Ptr& buffer);
    void setOnClose(const function<void()>& cb) {_onClose = cb;}

    static bool addJt1078Info(const string& key, const JT1078Info& info);
    static JT1078Info getJt1078Info(const string& key);
    static void delJt1078Info(const string& key);

    static bool addTalkInfo(const string& key, const JT1078TalkInfo& info);
    static JT1078TalkInfo getTalkInfo(const string& key);
    static void delTalkInfo(const string& key);

private:
    void onJT1078Talk(const JT1078RtpPacket::Ptr& buffer);
    void startSendTalkData(const JT1078MediaSource::Ptr &src, const JT1078TalkInfo& talkInfo);
    void sendRtpPacket(const JT1078MediaSource::RingDataType &pkt);
    void createSource(const JT1078RtpPacket::Ptr& buffer);

private:
    bool _isTalk = false;
    bool _first = true;
    int _channel = 0;
    string _simCode;
    string _path;
    string _app = "live";
    string _key;
    JT1078Parser _parser;
    TimeClock _timeClock;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    JT1078MediaSource::Wptr _source;
    JT1078MediaSource::Wptr _talkSource;
    unordered_map<int, JT1078DecodeTrack::Ptr> _mapTrack;
    JT1078MediaSource::RingType::DataQueReaderT::Ptr _playReader;
    function<void()> _onClose;

    static mutex _lck;
    static unordered_map<string, JT1078Info> _mapJt1078Info;

    static mutex _talkLck;
    // 关联语音对讲的信息
    static unordered_map<string, JT1078TalkInfo> _mapTalkInfo;
};


#endif //JT1078Connection_H
