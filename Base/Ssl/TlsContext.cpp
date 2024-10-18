#include "TlsContext.h"
#include "Log/Logger.h"
#include "Util/Thread.h"

#include <openssl/err.h>

string TlsContext::_keyFile;
string TlsContext::_crtFile;

TlsContext::TlsContext(bool server, const Socket::Ptr& socket)
    :_server(server)
    ,_socket(socket)
{
    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_digests();
    OpenSSL_add_all_ciphers();
    OpenSSL_add_all_algorithms();
}

TlsContext::~TlsContext()
{}

void TlsContext::setKeyFile(const string& keyFile, const string& crtFile)
{
    _keyFile = keyFile;
    _crtFile = crtFile;
}

void TlsContext::initSsl()
{
    SSL_CTX* sslCtx;
#if (OPENSSL_VERSION_NUMBER < 0x10002000L) // v1.0.2
    sslCtx = SSL_CTX_new(TLS_method());
#else
    sslCtx = SSL_CTX_new(TLS_method());
#endif
    _sslCtx.reset(sslCtx, [](SSL_CTX *ptr){ SSL_CTX_free(ptr); });

    if (!_sslCtx) {
        ERR_print_errors_fp(stderr);
        logError << "SSL_CTX_new failed";
        return ;
    }

    SSL_CTX_set_verify(_sslCtx.get(), SSL_VERIFY_NONE, NULL);
    if (SSL_CTX_set_cipher_list(_sslCtx.get(), "ALL") != 1) {
        logError << "SSL_CTX_set_cipher_list failed: " << getSslError();
        return ;
    }

    // 载入用户的数字证书， 此证书用来发送给客户端。证书里包含有公钥
    if (SSL_CTX_use_certificate_file(_sslCtx.get(), _crtFile.c_str() /*"cert.pem"*/, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        logError << "SSL_CTX_use_certificate_file failed: " << getSslError();
        return ;
    }

    // 载入用户私钥
    if (SSL_CTX_use_PrivateKey_file(_sslCtx.get(), _keyFile.c_str()/*"key.pem"*/, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        logError << "SSL_CTX_use_PrivateKey_file failed: " << getSslError();
        return;
    }

    // 检查用户私钥是否正确
    if (!SSL_CTX_check_private_key(_sslCtx.get())) {
        ERR_print_errors_fp(stdout);
        logError << "SSL_CTX_check_private_key failed: " << getSslError();
        return;
    }

    auto sslPtr = SSL_new(_sslCtx.get());
    _ssl.reset(sslPtr, [](SSL *ptr){ SSL_free(ptr); });

    if (!_ssl) {
        ERR_print_errors_fp(stderr);
        logError << "SSL_new failed";
        return ;
    }

    if ((_bioIn = BIO_new(BIO_s_mem())) == NULL) {
        ERR_print_errors_fp(stderr);
        logError << "BIO_new in failed";
        return ;
    }

    if ((_bioOut = BIO_new(BIO_s_mem())) == NULL) {
        BIO_free(_bioIn);
        ERR_print_errors_fp(stderr);
        logError << "BIO_new out failed";
    }

    SSL_set_bio(_ssl.get(), _bioIn, _bioOut);

    // SSL setup active, as server role.
    if (_server) {
        SSL_set_accept_state(_ssl.get());
    } else {
        SSL_set_connect_state(_ssl.get());
    }
    SSL_set_mode(_ssl.get(), SSL_MODE_ENABLE_PARTIAL_WRITE);
}

void TlsContext::handshake()
{
    int r0 = SSL_do_handshake(_ssl.get()); int r1 = SSL_get_error(_ssl.get(), r0); ERR_clear_error();
    if (r0 != -1 || r1 != SSL_ERROR_WANT_READ) {
        logError << "handshake failed: " << getSslError();
        return ;
    }

    uint8_t* data = NULL;
    int size = BIO_get_mem_data(_bioOut, &data);
    if (!data || size <= 0) {
        logError << "BIO_get_mem_data failed: " << getSslError();
        return ;
    }
    auto buffer = make_shared<StreamBuffer>();
    buffer->setCapacity(size + 1);
    buffer->assign((char*)data, size);
    if (_socket) {
        _socket->send(buffer);
    }
    if ((r0 = BIO_reset(_bioOut)) != 1) {
        logError << "BIO_reset r0=" << r0 << " " << getSslError();
        return ;
    }
}

void TlsContext::onRead(const StreamBuffer::Ptr &buffer) {
    if (!buffer->size()) {
        logWarn << "buffer is empty";
        return;
    }

    uint32_t offset = 0;
    int handshakeFlag = 0;
    if (!SSL_is_init_finished(_ssl.get())) {
        handshakeFlag = 1;
        while (offset < buffer->size()) {
            auto nwrite = BIO_write(_bioIn, buffer->data() + offset, buffer->size() - offset);
            if (nwrite <= 0) {
                logError << getSslError();
                return ;
            }
            offset += nwrite;
            int r0 = SSL_do_handshake(_ssl.get());
            int r1 = SSL_get_error(_ssl.get(), r0);
            logWarn << "handshake r0: " << r0;

            if (r0 == 1 && r1 == SSL_ERROR_NONE) {
                logInfo << "handshake success =================== " << _server;
                handshakeFlag = 2;
                if (!_server) {
                    send(nullptr);
                }
                break;
            } else {
                uint8_t* data = nullptr;
                int size = 0;
                if ((size = BIO_get_mem_data(_bioOut, &data)) > 0) {
                    // OK, reset it for the next write.
                    if ((r0 = BIO_reset(_bioIn)) != 1) {
                        logError << "BIO_reset r0=" << r0 << " " << getSslError();
                        return ;
                    }
                } else {
                    continue;
                }
                auto buffer = make_shared<StreamBuffer>();
                buffer->setCapacity(size + 1);
                buffer->assign((char*)data, size);
                if (_socket) {
                    _socket->send(buffer);
                }

                if ((r0 = BIO_reset(_bioOut)) != 1) {
                    logError << "BIO_reset r0=" << r0 << " " << getSslError();
                    return ;
                }
                // return ;
            }
        }
    }

    if (handshakeFlag == 2) {
        unprotect();
    } else if (handshakeFlag == 1) {
        return ;
    }
    
    while (offset < buffer->size()) {
        auto nwrite = BIO_write(_bioIn, buffer->data() + offset, buffer->size() - offset);
        if (nwrite > 0) {
            //部分或全部写入bio完毕
            offset += nwrite;
            unprotect();
            continue;
        }
        //nwrite <= 0,出现异常
        logError << "ssl error:" << getSslError();
        shutdown();
        break;
    }
}

void TlsContext::unprotect()
{
    int total = 0;
    int nread = 0;
    auto buffer_bio = make_shared<StreamBuffer>();
    buffer_bio->setCapacity(_bufSize);
    int buf_size = buffer_bio->getCapacity() - 1;
    do {
        nread = SSL_read(_ssl.get(), buffer_bio->data() + total, buf_size - total);
        if (nread > 0) {
            total += nread;
        }
    } while (nread > 0 && buf_size - total > 0);

    if (!total) {
        //未有数据
        logWarn << "ssl read empty";
        return;
    }

    //触发此次回调
    buffer_bio->data()[total] = '\0';
    buffer_bio->setSize(total);
    if (_onConnRead) {
        _onConnRead(buffer_bio);
    }

    if (nread > 0) {
        //还有剩余数据，读取剩余数据
        unprotect();
    }
}

ssize_t TlsContext::send(Buffer::Ptr buffer)
{
    int totalSendSize = 0;

    if (buffer && buffer->size()) {
        _bufferSend.emplace_back(buffer);
    }
    
    // if (!_server && !_hasHandshake) {
    //     _hasHandshake = true;
    //     SSL_do_handshake(_ssl.get());
    // }
    
    logInfo << "buffer list size: " << _bufferSend.size() << this << ", thresd: " << Thread::getThreadName() << endl;
    // logInfo << "buffer size: " << buffer->size() << this << ", thresd: " << Thread::getThreadName() << endl;

    if (!SSL_is_init_finished(_ssl.get()) && !_server) {
        return 0;
    }

    if (!SSL_is_init_finished(_ssl.get()) || _bufferSend.empty()) {
        //ssl未握手结束或没有需要发送的数据
        protect();
        return 0;
    }

    //加密数据并发送
    while (!_bufferSend.empty()) {
        cout << "get buffer front" << this << ", thresd: " << Thread::getThreadName() << endl;
        auto front = _bufferSend.front();
        _bufferSend.pop_front();
        uint32_t offset = 0;
        while (offset < front->size()) {
            auto nwrite = SSL_write(_ssl.get(), front->data() + offset, front->size() - offset);
            if (nwrite > 0) {
                //部分或全部写入完毕
                offset += nwrite;
                totalSendSize += nwrite;
                protect();
                continue;
            }
            //nwrite <= 0,出现异常
            break;
        }

        if (offset != front->size()) {
            //这个包未消费完毕，出现了异常,清空数据并断开ssl
            cout << "ssl error:" << getSslError() << endl;
            shutdown();
            break;
        }

        //这个包消费完毕，开始消费下一个包
        // _bufferSend.pop_front();
    }

    cout << "end func, buffer list size: " << _bufferSend.size() << this << ", thread: " << Thread::getThreadName() << endl;
    return totalSendSize;
}

void TlsContext::protect()
{
    int total = 0;
    int nread = 0;
    auto buffer_bio = make_shared<StreamBuffer>();
    buffer_bio->setCapacity(_bufSize);
    int buf_size = buffer_bio->getCapacity() - 1;
    do {
        nread = BIO_read(_bioOut, buffer_bio->data() + total, buf_size - total);
        if (nread > 0) {
            total += nread;
        }
    } while (nread > 0 && buf_size - total > 0);

    if (!total) {
        //未有数据
        return;
    }

    //触发此次回调
    buffer_bio->data()[total] = '\0';
    buffer_bio->setSize(total);
    if (_socket) {
        _socket->send(buffer_bio);
    }

    if (nread > 0) {
        //还有剩余数据，读取剩余数据
        protect();
    }
}

void TlsContext::setOnConnRead(const function<void(const StreamBuffer::Ptr& buffer)>& cb)
{
    _onConnRead = cb;
}

void TlsContext::setOnConnSend(const function<void(const Buffer::Ptr& buffer)>& cb)
{
    _onConnSend = cb;
}

string TlsContext::getSslError()
{
    unsigned long code = ERR_get_error();
    auto buffer = ERR_reason_error_string(code);
    
    return buffer;
}

void TlsContext::shutdown()
{
    int ret = SSL_shutdown(_ssl.get());
    if (ret != 1) {
        logError << "SSL shutdown failed:" << getSslError();
    } else {
        unprotect();
    }
}