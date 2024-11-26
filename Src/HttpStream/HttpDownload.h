#ifndef HttpDownload_h
#define HttpDownload_h

#include "Http/HttpClient.h"
#include "Http/HttpChunkedParser.h"

#include <string>
#include <memory>
#include <functional>

using namespace std;

class HttpDownload : public HttpClient
{
public:
    using Ptr = shared_ptr<HttpDownload>;
    HttpDownload();
    ~HttpDownload();

public:
    // HttpClient override
    void onHttpRequest() override;
    void onRecvContent(const char *data, uint64_t len) override;
    void onConnect() override;
    void close() override;

public:
    void setRange(uint64_t startPos, uint64_t size = 4 * 1024 * 1024);
    void start(const string& localIp, int localPort, const string& url, int timeout);
    void onHttpResponce(const char* data, int len);
    void onError(const string& err);
    void setOnHttpResponce(const function<void(const char* data, int len)>& cb);
    void setOnClose(const function<void(uint64_t filesize)>& cb);
    void stop();
    void pause();
    uint64_t getFileSize()  {return _fileSize;}


private:
    uint64_t _recvSize = 0;
    uint64_t _fileSize = 0;
    EventLoop::Ptr _loop;
    Socket::Ptr _socket;
    HttpChunkedParser::Ptr _chunkedParser;

    function<void(const char* data, int len)> _onHttpResponce;
    function<void(uint64_t filesize)> _onClose;
};

#endif //HttpFlvClient_h