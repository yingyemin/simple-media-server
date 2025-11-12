#include "srtp2/srtp.h"

#include "WebrtcDtlsSession.h"
#include "Log/Logger.h"
#include "Common/Config.h"

using namespace std;

static int verifyCallback(int ok, X509_STORE_CTX *ctx)
{
    // Always OK, we don't check the certificate of client,
    // because we allow client self-sign certificate.
    return 1;
}

void sslOnInfo(const SSL* dtls, int where, int ret)
{
    const char* method;
    int w = where & ~SSL_ST_MASK;
    if (w & SSL_ST_CONNECT) {
        method = "SSL_connect";
    } else if (w & SSL_ST_ACCEPT) {
        method = "SSL_accept";
    } else {
        method = "undefined";
    }

    int err = SSL_get_error(dtls, ret);
    if (where & SSL_CB_LOOP) {
		logInfo << "DTLS: method [" << method << "] state[" << SSL_state_string(dtls) << " | " 
			  << SSL_state_string_long(dtls) << "] where [" << where << "] result [" << ret 
			  << "] err [" << err << "]";
    } else if (where & SSL_CB_ALERT) {
        method = (where & SSL_CB_READ) ? "read":"write";

        // @see https://www.openssl.org/docs/man1.0.2/man3/SSL_alert_type_string_long.html
        string alert_type = SSL_alert_type_string_long(ret);
        string alert_desc = SSL_alert_desc_string(ret);

        logWarn << "DTLS: SSL3 alert method [" << method << "] type [" << alert_type << "] desc ["
				  << alert_desc << " | " << SSL_alert_desc_string_long(ret) << "] where [" << where
				  << "] result [" << ret << "] err [" << err << "]";

        // if (alert_type == "warning" && alert_desc == "CN") {
			
        // } else {
			
        // }
    } else if (where & SSL_CB_EXIT) {
        stringstream ss;
        ss << "DTLS: Fail method [" << method << "] state [" << SSL_state_string(dtls) << " | "
				  << SSL_state_string_long(dtls) << "] where [" << where << "] result [" << ret
				  << "] err [" << err << "]";
        if (ret == 0) {
			logWarn << ss.str();
        } else if (ret < 0) {
            if (err != SSL_ERROR_NONE && err != SSL_ERROR_WANT_READ && err != SSL_ERROR_WANT_WRITE) {
				logError << ss.str();
            } else {
				logError << ss.str();
            }
        }
    }
}

SSL_CTX* buildDtlsCtx(const bool ecdsa, const EC_KEY* eckey, X509* cert,  EVP_PKEY* pkey) {
    SSL_CTX* dtlsCtx = nullptr;
#if OPENSSL_VERSION_NUMBER < 0x10002000L // v1.0.2
    dtlsCtx = SSL_CTX_new(DTLSv1_method());
#else
    // SrsDtlsVersionAuto, use version-flexible DTLS methods
    dtlsCtx = SSL_CTX_new(DTLS_method());
#endif

    if (ecdsa) { // By ECDSA, https://stackoverflow.com/a/6006898
#if OPENSSL_VERSION_NUMBER >= 0x10002000L // v1.0.2
    // For ECDSA, we could set the curves list.
    // @see https://www.openssl.org/docs/man1.0.2/man3/SSL_CTX_set1_curves_list.html
    SSL_CTX_set1_curves_list(dtlsCtx, "P-521:P-384:P-256");
#endif

    // For openssl <1.1, we must set the ECDH manually.
    // @see https://stackoverrun.com/cn/q/10791887
#if OPENSSL_VERSION_NUMBER < 0x10100000L // v1.1.x
#if OPENSSL_VERSION_NUMBER < 0x10002000L // v1.0.2
    SSL_CTX_set_tmp_ecdh(dtlsCtx, eckey);
#else
    SSL_CTX_set_ecdh_auto(dtlsCtx, 1);
#endif
#endif
    }

    // Setup DTLS context.
    bool result = false;
    do {
        // We use "ALL", while you can use "DEFAULT" means "ALL:!EXPORT:!LOW:!aNULL:!eNULL:!SSLv2"
        // @see https://www.openssl.org/docs/man1.0.2/man1/ciphers.html
        if (SSL_CTX_set_cipher_list(dtlsCtx, "ALL") != 1) {
			logError << "SSL_CTX_set_cipher_list failed";
			break;
		}

        // Setup the certificate.
        if (SSL_CTX_use_certificate(dtlsCtx, cert) != 1) {
			logError << "SSL_CTX_use_certificate failed";
			break;
		}
		
        if (SSL_CTX_use_PrivateKey(dtlsCtx, pkey) != 1) {
			logError << "SSL_CTX_use_certificate failed";
			break;
		}

        // Server will send Certificate Request.
        // @see https://www.openssl.org/docs/man1.0.2/man3/SSL_CTX_set_verify.html
        // TODO: FIXME: Config it, default to off to make the packet smaller.
        SSL_CTX_set_verify(dtlsCtx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, verifyCallback);
        // The depth count is "level 0:peer certificate", "level 1: CA certificate",
        // "level 2: higher level CA certificate", and so on.
        // @see https://www.openssl.org/docs/man1.0.2/man3/SSL_CTX_set_verify.html
        SSL_CTX_set_verify_depth(dtlsCtx, 4);

        // Whether we should read as many input bytes as possible (for non-blocking reads) or not.
        // @see https://www.openssl.org/docs/man1.0.2/man3/SSL_CTX_set_read_ahead.html
        SSL_CTX_set_read_ahead(dtlsCtx, 1);

        // TODO: Maybe we can use SRTP-GCM in future.
        // @see https://bugs.chromium.org/p/chromium/issues/detail?id=713701
        // @see https://groups.google.com/forum/#!topic/discuss-webrtc/PvCbWSetVAQ
        // @remark Only support SRTP_AES128_CM_SHA1_80, please read ssl/d1_srtp.c
        if (SSL_CTX_set_tlsext_use_srtp(dtlsCtx, "SRTP_AES128_CM_SHA1_80") != 0) {
			logError << "SSL_CTX_set_tlsext_use_srtp failed";
			break;
		}

		result = true;
    }while (0);

	if (!result) {
		SSL_CTX_free(dtlsCtx);
		dtlsCtx = nullptr;
	}
		
    return dtlsCtx;
}


DtlsCertificate::~DtlsCertificate() {
    if (_eckey) 
        EC_KEY_free(_eckey);

    if (_dtlsPkey)
        EVP_PKEY_free(_dtlsPkey);

    if (_dtlsCert) 
        X509_free(_dtlsCert);
}

bool DtlsCertificate::init() {
#if OPENSSL_VERSION_NUMBER < 0x10100000L // v1.1.x
    // Initialize SSL library by registering algorithms
    // The SSL_library_init() and OpenSSL_add_ssl_algorithms() functions were deprecated in OpenSSL 1.1.0 by OPENSSL_init_ssl().
    // @see https://www.openssl.org/docs/man1.1.0/man3/OpenSSL_add_ssl_algorithms.html
    // @see https://web.archive.org/web/20150806185102/http://sctp.fh-muenster.de:80/dtls/dtls_udp_echo.c
    OpenSSL_add_ssl_algorithms();
#else
    // As of version 1.1.0 OpenSSL will automatically allocate all resources that it needs so no explicit
    // initialisation is required. Similarly it will also automatically deinitialise as required.
    // @see https://www.openssl.org/docs/man1.1.0/man3/OPENSSL_init_ssl.html
    OPENSSL_init_ssl(OPENSSL_INIT_ENGINE_ALL_BUILTIN | OPENSSL_INIT_LOAD_CONFIG, nullptr);
#endif

    // Initialize SRTP first.
    if (srtp_init() != 0) {
		logError << "srtp_init failed";
		return false;
	}


    static string certFile = Config::instance()->getAndListen([](const json& config) {
        certFile = Config::instance()->get("Ssl", "cert");
    }, "Ssl", "cert");

    static string keyFile = Config::instance()->getAndListen([](const json& config) {
        keyFile = Config::instance()->get("Ssl", "key");
    }, "Ssl", "key");

    static bool loadCertFile = Config::instance()->getAndListen([](const json& config) {
        loadCertFile = Config::instance()->get("Webrtc", "Server", "Server1", "loadCertFile", "0");
    }, "Webrtc", "Server", "Server1", "loadCertFile", "0");

    if (certFile.empty() || keyFile.empty() || !loadCertFile) {
        // Create keys by RSA or ECDSA.
        _dtlsPkey = EVP_PKEY_new();
        if (!_dtlsPkey) {
            logError << "EVP_PKEY_new failed";
            return false;
        }
        
        _ecdsaMode = false;
        if (!_ecdsaMode) { // By RSA
            RSA* rsa = RSA_new();
            if (!rsa) {
                logError << "RSA_new failed";
                return false;
            }

            // Initialize the big-number for private key.
            BIGNUM* exponent = BN_new();
            if (!exponent) {
                logError << "BN_new failed";
                return false;
            }
            
            BN_set_word(exponent, RSA_F4);

            // Generates a key pair and stores it in the RSA structure provided in rsa.
            // @see https://www.openssl.org/docs/man1.0.2/man3/RSA_generate_key_ex.html
            int key_bits = 1024;
            RSA_generate_key_ex(rsa, key_bits, exponent, NULL);

            // @see https://www.openssl.org/docs/man1.1.0/man3/EVP_PKEY_type.html
            if (EVP_PKEY_set1_RSA(_dtlsPkey, rsa) != 1) {
                logError << "EVP_PKEY_set1_RSA failed";
                return false;
            }

            RSA_free(rsa);
            BN_free(exponent);
        }

        if (_ecdsaMode) { // By ECDSA, https://stackoverflow.com/a/6006898
            // _eckey = EC_KEY_new();
            // if (!_eckey) {
            // 	logError << "EC_KEY_new failed";
            // 	return false;
            // }

            // Should use the curves in ClientHello.supported_groups
            // For example:
            //      Supported Group: x25519 (0x001d)
            //      Supported Group: secp256r1 (0x0017)
            //      Supported Group: secp384r1 (0x0018)
            // @remark The curve NID_secp256k1 is not secp256r1, k1 != r1.
            // TODO: FIXME: Parse ClientHello and choose the curve.
            // Note that secp256r1 in openssl is called NID_X9_62_prime256v1, not NID_secp256k1
            // @see https://stackoverflow.com/questions/41950056/openssl1-1-0-b-is-not-support-secp256r1openssl-ecparam-list-curves
            // EC_GROUP* ecgroup = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
            // //EC_GROUP* ecgroup = EC_GROUP_new_by_curve_name(NID_secp384r1);
            // if (!ecgroup) {
            // 	logError << "EC_GROUP_new_by_curve_name failed";
            // 	return false;
            // }
    // #if OPENSSL_VERSION_NUMBER < 0x10100000L // v1.1.x
    //         // For openssl 1.0, we must set the group parameters, so that cert is ok.
    //         // @see https://github.com/monero-project/monero/blob/master/contrib/epee/src/net_ssl.cpp#L225
    //         EC_GROUP_set_asn1_flag(ecgroup, OPENSSL_EC_NAMED_CURVE);
    // #endif

            // if (EC_KEY_set_group(_eckey, ecgroup) != 1) {
            // 	logError << "EC_KEY_set_group failed";
            // 	return false;
            // }

            // Create key with curve.
            _eckey = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
            if (!_eckey) {
                logError << "EC_KEY_new_by_curve_name failed";
                return false;
            }

            EC_KEY_set_asn1_flag(_eckey, OPENSSL_EC_NAMED_CURVE);
            
            if (EC_KEY_generate_key(_eckey) != 1) {
                logError << "EC_KEY_generate_key failed";
                return false;
            }

            // @see https://www.openssl.org/docs/man1.1.0/man3/EVP_PKEY_type.html
            if (EVP_PKEY_set1_EC_KEY(_dtlsPkey, _eckey) != 1) {
                logError << "EVP_PKEY_set1_EC_KEY failed";
                return false;
            }

            // EC_GROUP_free(ecgroup);
            _eckey = nullptr;
        }

        // Create certificate, from previous generated pkey.
        // TODO: Support ECDSA certificate.
        _dtlsCert = X509_new();
        if (!_dtlsCert) {
            logError << "X509_new failed";
            return false;
        }
        
        if (true) {
            X509_NAME* subject = X509_NAME_new();
            if (!subject) {
                logError << "X509_NAME_new failed";
                return false;
            }

            int serial = rand();
            ASN1_INTEGER_set(X509_get_serialNumber(_dtlsCert), serial);

            const std::string& aor = "ossrs.net";
            X509_NAME_add_entry_by_txt(subject, "CN", MBSTRING_ASC, (unsigned char *) aor.data(), aor.size(), -1, 0);

            X509_set_issuer_name(_dtlsCert, subject);
            X509_set_subject_name(_dtlsCert, subject);

            int expire_day = 365;
            const long cert_duration = 60*60*24*expire_day;

            X509_gmtime_adj(X509_get_notBefore(_dtlsCert), 0);
            X509_gmtime_adj(X509_get_notAfter(_dtlsCert), cert_duration);

            X509_set_version(_dtlsCert, 2);
            if (X509_set_pubkey(_dtlsCert, _dtlsPkey) != 1) {
                logError << "X509_set_pubkey failed";
                return false;
            }
            
            if (X509_sign(_dtlsCert, _dtlsPkey, EVP_sha1()) == 0) {
                logError << "X509_sign failed";
                return false;
            }

            X509_NAME_free(subject);
        }
    } else {
        FILE* file{ nullptr };

		file = fopen(certFile.c_str(), "r");

		if (!file)
		{
			logError << "error reading DTLS certificate file: " << std::strerror(errno);

			return false;
		}

		_dtlsCert = PEM_read_X509(file, nullptr, nullptr, nullptr);

		if (!_dtlsCert)
		{
			logError << "PEM_read_X509() failed";

			return false;
		}

		fclose(file);

		file = fopen(keyFile.c_str(), "r");

		if (!file)
		{
			logError << "error reading DTLS private key file: " << std::strerror(errno);

			return false;
		}

		_dtlsPkey = PEM_read_PrivateKey(file, nullptr, nullptr, nullptr);

		if (!_dtlsPkey)
		{
			logError << "PEM_read_PrivateKey() failed";

			return false;
		}

		fclose(file);
    }

    // Show DTLS fingerprint
    if (true) {
        // char fp[100] = {0};
        // char *p = fp;
        unsigned char md[EVP_MAX_MD_SIZE];
        unsigned int n = 0;

        X509_digest(_dtlsCert, EVP_sha256(), md, &n);
        _fingerprint.resize(3*n - 1);
        char* p = (char*)_fingerprint.data();

        for (unsigned int i = 0; i < n; i++, ++p) {
            sprintf(p, "%02X", md[i]);
            p += 2;
            if (i < (n-1)) {
                *p = ':';
            }
			// *p = (i < (n-1)) ? ':' : '\0';
        }

        // _fingerprint.assign(fp, strlen(fp));
		logInfo << "fingerprint: [" << _fingerprint << "]";
    }

    return true;	
}

DtlsSession::DtlsSession(const string& role)
    :_role(role)
{}

DtlsSession::~DtlsSession()
{
    if (_dtlsCtx) {
        SSL_CTX_free(_dtlsCtx);
        _dtlsCtx = NULL;
    }

    if (_dtls) {
        SSL_free(_dtls);
        _dtls = NULL;
    }
}


bool DtlsSession::init(const shared_ptr<DtlsCertificate>& dtlsCert)
{
	_dtlsCtx = buildDtlsCtx(true, dtlsCert->getEcdsaKey(), 
			dtlsCert->getCert(), dtlsCert->getPublicKey());
	if (!_dtlsCtx)
		return false;
	
	// TODO: FIXME: Support config by vhost to use RSA or ECDSA certificate.
    if (nullptr == (_dtls = SSL_new(_dtlsCtx))) {
		logError << "ssl_new dtls failed";
		return false;
    }

    SSL_set_info_callback(_dtls, sslOnInfo);
	
	if (nullptr == (_bioIn = BIO_new(BIO_s_mem()))) {
		logError << "BIO_in new failed";
		return false;
    }

	if (nullptr ==(_bioOut = BIO_new(BIO_s_mem()))) {
		logError << "BIO_out new failed";
		BIO_free(_bioIn);
		return false;
	}
	SSL_set_bio(_dtls, _bioIn, _bioOut);
/*
	STACK_OF(SSL_CIPHER) *srvr = SSL_get_ciphers(_dtls);
    for (int i = 0; i < sk_SSL_CIPHER_num(srvr); ++i) {
    	const SSL_CIPHER *c = sk_SSL_CIPHER_value(srvr, i);
    	logInfo << "id [" << c->id << "] name " << c->name;
	}
*/
	// Dtls setup passive, as server role.
    if (_role == "server") {
        SSL_set_accept_state(_dtls);	
    } else {
        SSL_set_connect_state(_dtls);
    }
	return true;
}

int DtlsSession::onDtls(Socket::Ptr sock, const Buffer::Ptr& buf, struct sockaddr* addr, int addr_len) {
    if (BIO_reset(_bioIn) != 1 || BIO_reset(_bioOut) != 1) {
		logError << "BIO_reset failed";
		return -1;
	}

    if (BIO_write(_bioIn, buf->data(), buf->size()) <= 0) {
        // TODO: 0 or -1 maybe block, use BIO_should_retry to check.
        logError << "BIO_write failed";
        return -1;
    }

	if (_handshakeFinish) {
		// TODO: support sctp
        int read = SSL_read(_dtls, static_cast<void*>(_sslReadBuffer), 2000);
        if (read > 0) {
            if (_onRecvApplicationData) {
                _onRecvApplicationData((char*)_sslReadBuffer, read);
            }
        }
		return 0;
	}

	// Do handshake and get the result.
    int hsRes = SSL_do_handshake(_dtls);
    uint8_t* outBioData = nullptr;
    int outBioLen = BIO_get_mem_data(_bioOut, &outBioData);
	if (outBioLen) {
        if (sock->getSocketType() == SOCKET_TCP) {
            uint8_t payload_ptr[2];
            payload_ptr[0] = outBioLen >> 8;
            payload_ptr[1] = outBioLen & 0x00FF;

            sock->send((char*)payload_ptr, 2);
        }
		sock->send((char*)outBioData, outBioLen, true, addr, addr_len);
    }
    
    int hsErr = SSL_get_error(_dtls, hsRes);
    // Fatal SSL error, for example, no available suite when peer is DTLS 1.0 while we are DTLS 1.2.
    if (hsRes < 0 && (hsErr != SSL_ERROR_NONE && hsErr != SSL_ERROR_WANT_READ && hsErr != SSL_ERROR_WANT_WRITE)) {
		ERR_load_ERR_strings();
		ERR_load_crypto_strings();

        unsigned long code = ERR_get_error();
        auto buffer = ERR_reason_error_string(code);
		logError << "handshake hsRes [" << hsRes << "] hsErr [" << hsErr << "] msg [" << buffer << "]";
		return -1;
    }
	
	if (SSL_ERROR_NONE == hsErr /* && SSL_is_init_finished(dtls) == 1*/) {
		logInfo << "dtls handshake finish";
		_handshakeFinish = true;
		if (_onHandshakeDone) {
			_onHandshakeDone();
		}
	}

	return 0;
}

int32_t DtlsSession::decodeHandshake(Socket::Ptr sock, char *data, int32_t nb_data) {

	int32_t r0 = 0;

	if ((r0 = BIO_reset(_bioIn)) != 1) {
		logError << "ERROR_OpenSslBIOReset, BIO_in reset r0=" << r0;
		return 1;
	}
	if ((r0 = BIO_reset(_bioOut)) != 1) {
		logError << "ERROR_OpenSslBIOReset, BIO_out reset r0=" << r0;
		return 1;
	}

	// Trace the detail of DTLS packet.

	if ((r0 = BIO_write(_bioIn, data, nb_data)) <= 0) {

		logInfo << "ERROR_OpenSslBIOWrite, BIO_write r0=" << r0;
		return 1;
	}

	int32_t err = 0;
	if((err=this->startActiveHandshake(sock)!=0)){
		return 1;
	}

	for (int32_t i = 0; i < 1024 && BIO_ctrl_pending(_bioIn) > 0; i++) {
		char buf[8092];
		int32_t r0 = SSL_read(_dtls, buf, sizeof(buf));
		int32_t r1 = SSL_get_error(_dtls, r0);
		   if (r0 <= 0) {

			if (r1 != SSL_ERROR_WANT_READ && r1 != SSL_ERROR_WANT_WRITE) {
				break;
			}

			uint8_t *data = NULL;
			int32_t size = BIO_get_mem_data(_bioOut, (char** )&data);
			if(size>0 && sock) {
                if (sock->getSocketType() == SOCKET_TCP) {
                    uint8_t payload_ptr[2];
                    payload_ptr[0] = size >> 8;
                    payload_ptr[1] = size & 0x00FF;

                    sock->send((char*)payload_ptr, 2);
                }
                sock->send((char*)data, size);
            }
			continue;
		}


	}

	return 0;
}

int DtlsSession::startActiveHandshake(Socket::Ptr sock)
{
    int err = 0;

    // During initialization, we only need to call SSL_do_handshake once because SSL_read consumes
    // the handshake message if the handshake is incomplete.
    // To simplify maintenance, we initiate the handshake for both the DTLS server and client after
    // sending out the ICE response in the start_active_handshake function. It's worth noting that
    // although the DTLS server may receive the ClientHello immediately after sending out the ICE
    // response, this shouldn't be an issue as the handshake function is called before any DTLS
    // packets are received.
    int r0 = SSL_do_handshake(_dtls);
    int r1 = SSL_get_error(_dtls, r0); ERR_clear_error();
    // Fatal SSL error, for example, no available suite when peer is DTLS 1.0 while we are DTLS 1.2.
    if (r0 < 0 && (r1 != SSL_ERROR_NONE && r1 != SSL_ERROR_WANT_READ && r1 != SSL_ERROR_WANT_WRITE)) {
        logError << "handshake failed: r0=" << r0 << ", r1=" << r1;
        return -1;
    }

    bool m_handshake_done = false;
    if (r1 == SSL_ERROR_NONE) {
        m_handshake_done = true;
        logDebug << ("\n******************dtls handshake is sucess....................\n");
    }

    uint8_t *data = NULL;
	int32_t size = BIO_get_mem_data(_bioOut, (char** )&data);

	// filter_data(data, size);

	if(size>0 && sock) {
        if (sock->getSocketType() == SOCKET_TCP) {
            uint8_t payload_ptr[2];
            payload_ptr[0] = size >> 8;
            payload_ptr[1] = size & 0x00FF;

            sock->send((char*)payload_ptr, 2);
        }
        sock->send((char*)data, size);
    }

	if(m_handshake_done && _onHandshakeDone) {
		_onHandshakeDone();
	}

    return 0;
}

int DtlsSession::checkTimeout(Socket::Ptr sock)
{
    int32_t r0 = 0; 
    timeval to = {0};
    if ((r0 = DTLSv1_get_timeout(_dtls, &to)) == 0) {
        // No timeout, for example?, wait for a default 50ms.
        return 50;
    }
    int64_t timeout = to.tv_sec + to.tv_usec;


    if (timeout > 0) {
        int max = 100 * 1000;
        int min = 50 * 1000;
        timeout = timeout > max ? max : timeout;
        timeout = timeout < min ? min : timeout;
        return timeout / 1000;
    }

    r0 = BIO_reset(_bioOut); 
    int32_t r1 = SSL_get_error(_dtls, r0);
    if (r0 != 1) {
        logError << "OpenSslBIORese BIO_reset r0=" << r0 << ", r1=" << r1;
        return 0;
    }

    r0 = DTLSv1_handle_timeout(_dtls); 
    r1 = SSL_get_error(_dtls, r0);
    if (r0 == 0) {
        return 50; // No timeout had expired.
    }

    if (r0 != 1) {
        logError << "dtls error ARQ r0=" << r0 << ", r1=" << r1;
        return 0;
    }


    uint8_t* data = NULL;
    int32_t size = BIO_get_mem_data(_bioOut, (char**)&data);
        // arq_count++;
    if (sock && size > 0) {
        if (sock->getSocketType() == SOCKET_TCP) {
            uint8_t payload_ptr[2];
            payload_ptr[0] = size >> 8;
            payload_ptr[1] = size & 0x00FF;

            sock->send((char*)payload_ptr, 2);
        }
        sock->send((char*)data, size);
    }

    return 50;
}

void DtlsSession::sendApplicationData(Socket::Ptr socket, const char* data, int len)
{
    if (!_handshakeFinish) {
        return ;
    }

    (void)BIO_reset(_bioOut);

    int written;
    written = SSL_write(_dtls, static_cast<const void*>(data), static_cast<int>(len));
    if (written != static_cast<int>(len))
    {
        logError << "OpenSSL SSL_write() wrote less (" << written << "bytes) than given data (" << len << " bytes)";
    }

    if (BIO_eof(this->_bioOut))
        return;

    int64_t read;
    char* dataOut{ nullptr };

    read = BIO_get_mem_data(_bioOut, &dataOut); // NOLINT

    if (read <= 0)
        return;

    size_t offset = 0;
    while(offset < read) {
        auto *header = reinterpret_cast<const DtlsHeader *>(dataOut + offset);
        auto length = ntohs(header->length) + offsetof(DtlsHeader, payload);
        socket->send(dataOut + offset, length);
        offset += length;
    }
}

int DtlsSession::getSrtpKey(std::string& recvKey, std::string& sendKey)
{	
    unsigned char material[SRTP_MASTER_KEY_LEN * 2] = {0};  // client(SRTP_MASTER_KEY_KEY_LEN + SRTP_MASTER_KEY_SALT_LEN) + server
    static const string dtls_srtp_lable = "EXTRACTOR-dtls_srtp";
    if (!SSL_export_keying_material(_dtls, material, sizeof(material), dtls_srtp_lable.c_str(), dtls_srtp_lable.size(), nullptr, 0, 0)) {
		cerr << "SSL export key failed. result [" << ERR_get_error() << "]";
		return -1;
    }

    int masterKeySize = SRTP_MASTER_KEY_LEN - SRTP_SALT_LEN;

    size_t offset = 0;
    std::string client_master_key(reinterpret_cast<char*>(material), masterKeySize);
    offset += masterKeySize;

    std::string server_master_key(reinterpret_cast<char*>(material + offset), masterKeySize);
    offset += masterKeySize;

    std::string client_master_salt(reinterpret_cast<char*>(material + offset), SRTP_SALT_LEN);
    offset += SRTP_SALT_LEN;

    std::string server_master_salt(reinterpret_cast<char*>(material + offset), SRTP_SALT_LEN);

    if (_role == "server") {
        recvKey = client_master_key + client_master_salt;
        sendKey = server_master_key + server_master_salt;
    } else {
        sendKey = client_master_key + client_master_salt;
        recvKey = server_master_key + server_master_salt;
    }
    return 0;	
}