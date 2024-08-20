#ifndef TlsContext_H_
#define TlsContext_H_

#include <memory>
#include <deque>

#include "openssl/ssl.h"
#include "Net/Buffer.h"
#include "Net/Socket.h"

using namespace std;

class TlsContext : public enable_shared_from_this<TlsContext>
{
public:
    using Ptr = shared_ptr<TlsContext>;
    
    TlsContext(bool server, const Socket::Ptr& socket);
    ~TlsContext();

public:
    static void setKeyFile(const string& keyFile, const string& crtFile);
    void initSsl();
    void onRead(const StreamBuffer::Ptr& buffer);
    ssize_t send(Buffer::Ptr pkt);
    string getSslError();
    void shutdown();
    void unprotect();
    void protect();
    void handshake();

    void setOnConnRead(const function<void(const StreamBuffer::Ptr& buffer)>& cb);
    void setOnConnSend(const function<void(const Buffer::Ptr& buffer)>& cb);

private:
    static string _keyFile;
    static string _crtFile;

    bool _server = true;
    bool _hasHandshake = false;
    int _bufSize = 32 * 1024;

    BIO* _bioIn = nullptr;
    BIO* _bioOut = nullptr;
    shared_ptr<SSL_CTX> _sslCtx;
    shared_ptr<SSL> _ssl;
    Socket::Ptr _socket;

    deque<Buffer::Ptr> _buffer_send;
    function<void(const StreamBuffer::Ptr& buffer)> _onConnRead;
    function<void(const Buffer::Ptr& buffer)> _onConnSend;
};

#endif //TlsContext_H_