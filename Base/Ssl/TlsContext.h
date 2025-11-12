#ifndef TlsContext_H_
#define TlsContext_H_

#include <memory>
#include <deque>

#include "openssl/ssl.h"
#include "Net/Buffer.h"
#include "Net/Socket.h"

// using namespace std;

class TlsContext : public std::enable_shared_from_this<TlsContext>
{
public:
    using Ptr = std::shared_ptr<TlsContext>;
    
    TlsContext(bool server, const Socket::Ptr& socket);
    ~TlsContext();

public:
    static void setKeyFile(const std::string& keyFile, const std::string& crtFile);
    void initSsl();
    void onRead(const StreamBuffer::Ptr& buffer);
    ssize_t send(Buffer::Ptr pkt);
    std::string getSslError();
    void shutdown();
    void unprotect();
    void protect();
    void handshake();

    void setOnConnRead(const std::function<void(const StreamBuffer::Ptr& buffer)>& cb);
    void setOnConnSend(const std::function<void(const Buffer::Ptr& buffer)>& cb);

private:
    static std::string _keyFile;
    static std::string _crtFile;

    bool _server = true;
    bool _hasHandshake = false;
    int _bufSize = 32 * 1024;

    BIO* _bioIn = nullptr;
    BIO* _bioOut = nullptr;
    std::shared_ptr<SSL_CTX> _sslCtx;
    std::shared_ptr<SSL> _ssl;
    Socket::Ptr _socket;

    std::deque<Buffer::Ptr> _bufferSend;
    std::function<void(const StreamBuffer::Ptr& buffer)> _onConnRead;
    std::function<void(const Buffer::Ptr& buffer)> _onConnSend;
};

#endif //TlsContext_H_