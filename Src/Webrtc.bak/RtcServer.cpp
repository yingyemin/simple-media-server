/**
 * 
 *
 * 
 *
 * 
 * 
 * 
 **/

#include "RtcServer.h"
#include "Log/Logger.h"
#include "Common/MediaSource.h"
#include "openssl/ssl.h"
#include "openssl/err.h"
#include "Rtsp/RtspConnection.h"
#include "Common/Define.h"
#include "Util/String.h"
#include <bitset>

static const size_t kRtpPacketSize = 1500;

static int verify_callback(int preverify_ok, X509_STORE_CTX *ctx)
{
    // Always OK, we don't check the certificate of client,
    // because we allow client self-sign certificate.
    return 1;
}

uint64_t SockAddrHash(struct sockaddr_in* saddr) {
	return ntohl(saddr->sin_addr.s_addr) << 32 + ntohs(saddr->sin_port);
}

void ssl_on_info(const SSL* dtls, int where, int ret)
{
    const char* method;
    int w = where& ~SSL_ST_MASK;
    if (w & SSL_ST_CONNECT) {
        method = "SSL_connect";
    } else if (w & SSL_ST_ACCEPT) {
        method = "SSL_accept";
    } else {
        method = "undefined";
    }

    int r1 = SSL_get_error(dtls, ret);
    if (where & SSL_CB_LOOP) {
		logInfo << "DTLS: method [" << method << "] state[" << SSL_state_string(dtls) << " | " 
			  << SSL_state_string_long(dtls) << "] where [" << where << "] result [" << ret 
			  << "] r1 [" << r1 << "]";
    } else if (where & SSL_CB_ALERT) {
        method = (where & SSL_CB_READ) ? "read":"write";

        // @see https://www.openssl.org/docs/man1.0.2/man3/SSL_alert_type_string_long.html
        string alert_type = SSL_alert_type_string_long(ret);
        string alert_desc = SSL_alert_desc_string(ret);

        if (alert_type == "warning" && alert_desc == "CN") {
			logWarn << "DTLS: SSL3 alert method [" << method << "] type [" << alert_type << "] desc ["
				  << alert_desc << " | " << SSL_alert_desc_string_long(ret) << "] where [" << where
				  << "] result [" << ret << "] r1 [" << r1 << "]";
        } else {
			logWarn << "DTLS: SSL3 alert method [" << method << "] type [" << alert_type << "] desc ["
				  << alert_desc << " | " << SSL_alert_desc_string_long(ret) << "] where [" << where
				  << "] result [" << ret << "] r1 [" << r1 << "]";
        }
    } else if (where & SSL_CB_EXIT) {
        if (ret == 0) {
			logWarn << "DTLS: Fail method [" << method << "] state [" << SSL_state_string(dtls) << " | "
				  << SSL_state_string_long(dtls) << "] where [" << where << "] result [" << ret
				  << "] r1 [" << r1 << "]";
        } else if (ret < 0) {
            if (r1 != SSL_ERROR_NONE && r1 != SSL_ERROR_WANT_READ && r1 != SSL_ERROR_WANT_WRITE) {
				logError << "DTLS: Error method [" << method << "] state [" << SSL_state_string(dtls)
					   << " | " << SSL_state_string_long(dtls) << "] where [" << where
					   << "] result [" << ret << "] r1 [" << r1 << "]";
            } else {
				logError << "DTLS: Error method [" << method << "] state [" << SSL_state_string(dtls)
					   << " | " << SSL_state_string_long(dtls) << "] where [" << where
					   << "] result [" << ret << "] r1 [" << r1 << "]";
            }
        }
    }
}

SSL_CTX* BuildDtlsCtx(const bool ecdsa, const EC_KEY* eckey, X509* cert,  EVP_PKEY* pkey) {
    SSL_CTX* dtls_ctx = nullptr;
#if OPENSSL_VERSION_NUMBER < 0x10002000L // v1.0.2
    dtls_ctx = SSL_CTX_new(DTLSv1_method());
#else
    // SrsDtlsVersionAuto, use version-flexible DTLS methods
    dtls_ctx = SSL_CTX_new(DTLS_method());
#endif

    if (ecdsa) { // By ECDSA, https://stackoverflow.com/a/6006898
#if OPENSSL_VERSION_NUMBER >= 0x10002000L // v1.0.2
    // For ECDSA, we could set the curves list.
    // @see https://www.openssl.org/docs/man1.0.2/man3/SSL_CTX_set1_curves_list.html
    SSL_CTX_set1_curves_list(dtls_ctx, "P-521:P-384:P-256");
#endif

    // For openssl <1.1, we must set the ECDH manually.
    // @see https://stackoverrun.com/cn/q/10791887
#if OPENSSL_VERSION_NUMBER < 0x10100000L // v1.1.x
#if OPENSSL_VERSION_NUMBER < 0x10002000L // v1.0.2
    SSL_CTX_set_tmp_ecdh(dtls_ctx, eckey);
#else
    SSL_CTX_set_ecdh_auto(dtls_ctx, 1);
#endif
#endif
    }

    // Setup DTLS context.
    bool result = false;
    do {
        // We use "ALL", while you can use "DEFAULT" means "ALL:!EXPORT:!LOW:!aNULL:!eNULL:!SSLv2"
        // @see https://www.openssl.org/docs/man1.0.2/man1/ciphers.html
        if (SSL_CTX_set_cipher_list(dtls_ctx, "ALL") != 1) {
			logError << "SSL_CTX_set_cipher_list failed";
			break;
		}

        // Setup the certificate.
        if (SSL_CTX_use_certificate(dtls_ctx, cert) != 1) {
			logError << "SSL_CTX_use_certificate failed";
			break;
		}
		
        if (SSL_CTX_use_PrivateKey(dtls_ctx, pkey) != 1) {
			logError << "SSL_CTX_use_certificate failed";
			break;
		}

        // Server will send Certificate Request.
        // @see https://www.openssl.org/docs/man1.0.2/man3/SSL_CTX_set_verify.html
        // TODO: FIXME: Config it, default to off to make the packet smaller.
        SSL_CTX_set_verify(dtls_ctx, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, verify_callback);
        // The depth count is "level 0:peer certificate", "level 1: CA certificate",
        // "level 2: higher level CA certificate", and so on.
        // @see https://www.openssl.org/docs/man1.0.2/man3/SSL_CTX_set_verify.html
        SSL_CTX_set_verify_depth(dtls_ctx, 4);

        // Whether we should read as many input bytes as possible (for non-blocking reads) or not.
        // @see https://www.openssl.org/docs/man1.0.2/man3/SSL_CTX_set_read_ahead.html
        SSL_CTX_set_read_ahead(dtls_ctx, 1);

        // TODO: Maybe we can use SRTP-GCM in future.
        // @see https://bugs.chromium.org/p/chromium/issues/detail?id=713701
        // @see https://groups.google.com/forum/#!topic/discuss-webrtc/PvCbWSetVAQ
        // @remark Only support SRTP_AES128_CM_SHA1_80, please read ssl/d1_srtp.c
        if (SSL_CTX_set_tlsext_use_srtp(dtls_ctx, "SRTP_AES128_CM_SHA1_80") != 0) {
			logError << "SSL_CTX_set_tlsext_use_srtp failed";
			break;
		}

		result = true;
    }while (0);

	if (!result) {
		SSL_CTX_free(dtls_ctx);
		dtls_ctx = nullptr;
	}
		
    return dtls_ctx;
}


DtlsCertificate::~DtlsCertificate() {
    if (eckey_) 
        EC_KEY_free(eckey_);

    if (dtls_pkey_)
        EVP_PKEY_free(dtls_pkey_);

    if (dtls_cert_) 
        X509_free(dtls_cert_);
}

bool DtlsCertificate::Initialize() {
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

    // Create keys by RSA or ECDSA.
    dtls_pkey_ = EVP_PKEY_new();
    if (!dtls_pkey_) {
		logError << "EVP_PKEY_new failed";
		return false;
	}
	
    if (!ecdsa_mode_) { // By RSA
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
        if (EVP_PKEY_set1_RSA(dtls_pkey_, rsa) != 1) {
			logError << "EVP_PKEY_set1_RSA failed";
			return false;
		}

        RSA_free(rsa);
        BN_free(exponent);
    }

	if (ecdsa_mode_) { // By ECDSA, https://stackoverflow.com/a/6006898
        eckey_ = EC_KEY_new();
        if (!eckey_) {
			logError << "EC_KEY_new failed";
			return false;
		}

        // Should use the curves in ClientHello.supported_groups
        // For example:
        //      Supported Group: x25519 (0x001d)
        //      Supported Group: secp256r1 (0x0017)
        //      Supported Group: secp384r1 (0x0018)
        // @remark The curve NID_secp256k1 is not secp256r1, k1 != r1.
        // TODO: FIXME: Parse ClientHello and choose the curve.
        // Note that secp256r1 in openssl is called NID_X9_62_prime256v1, not NID_secp256k1
        // @see https://stackoverflow.com/questions/41950056/openssl1-1-0-b-is-not-support-secp256r1openssl-ecparam-list-curves
        EC_GROUP* ecgroup = EC_GROUP_new_by_curve_name(NID_X9_62_prime256v1);
        //EC_GROUP* ecgroup = EC_GROUP_new_by_curve_name(NID_secp384r1);
        if (!ecgroup) {
			logError << "EC_GROUP_new_by_curve_name failed";
			return false;
		}
#if OPENSSL_VERSION_NUMBER < 0x10100000L // v1.1.x
        // For openssl 1.0, we must set the group parameters, so that cert is ok.
        // @see https://github.com/monero-project/monero/blob/master/contrib/epee/src/net_ssl.cpp#L225
        EC_GROUP_set_asn1_flag(ecgroup, OPENSSL_EC_NAMED_CURVE);
#endif

        if (EC_KEY_set_group(eckey_, ecgroup) != 1) {
			logError << "EC_KEY_set_group failed";
			return false;
		}
		
        if (EC_KEY_generate_key(eckey_) != 1) {
			logError << "EC_KEY_generate_key failed";
			return false;
		}

        // @see https://www.openssl.org/docs/man1.1.0/man3/EVP_PKEY_type.html
        if (EVP_PKEY_set1_EC_KEY(dtls_pkey_, eckey_) != 1) {
			logError << "EVP_PKEY_set1_EC_KEY failed";
			return false;
		}

        EC_GROUP_free(ecgroup);
    }

    // Create certificate, from previous generated pkey.
    // TODO: Support ECDSA certificate.
    dtls_cert_ = X509_new();
    if (!dtls_cert_) {
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
        ASN1_INTEGER_set(X509_get_serialNumber(dtls_cert_), serial);

        const std::string& aor = "ossrs.net";
        X509_NAME_add_entry_by_txt(subject, "CN", MBSTRING_ASC, (unsigned char *) aor.data(), aor.size(), -1, 0);

        X509_set_issuer_name(dtls_cert_, subject);
        X509_set_subject_name(dtls_cert_, subject);

        int expire_day = 365;
        const long cert_duration = 60*60*24*expire_day;

        X509_gmtime_adj(X509_get_notBefore(dtls_cert_), 0);
        X509_gmtime_adj(X509_get_notAfter(dtls_cert_), cert_duration);

        X509_set_version(dtls_cert_, 2);
        if (X509_set_pubkey(dtls_cert_, dtls_pkey_) != 1) {
			logError << "X509_set_pubkey failed";
			return false;
		}
		
        if (X509_sign(dtls_cert_, dtls_pkey_, EVP_sha1()) == 0) {
			logError << "X509_sign failed";
			return false;
		}

        X509_NAME_free(subject);
    }

    // Show DTLS fingerprint
    if (true) {
        char fp[100] = {0};
        char *p = fp;
        unsigned char md[EVP_MAX_MD_SIZE];
        unsigned int n = 0;

        X509_digest(dtls_cert_, EVP_sha256(), md, &n);

        for (unsigned int i = 0; i < n; i++, ++p) {
            sprintf(p, "%02X", md[i]);
            p += 2;
			*p = (i < (n-1)) ? ':' : '\0';
        }

        fingerprint_.assign(fp, strlen(fp));
		logInfo << "fingerprint: [" << fingerprint_ << "]";
    }

    return true;	
}


static DtlsCertificate* global_certificate_;

DtlsSession::~DtlsSession() {
    if (dtls_ctx_) {
        SSL_CTX_free(dtls_ctx_);
        dtls_ctx_ = NULL;
    }

    if (dtls_) {
        SSL_free(dtls_);
        dtls_ = NULL;
    }
}


bool DtlsSession::Init() {
	dtls_ctx_ = BuildDtlsCtx(true, global_certificate_->GetEcdsaKey(), 
			global_certificate_->GetCert(), global_certificate_->GetPublicKey());
	if (!dtls_ctx_)
		return false;
	
	// TODO: FIXME: Support config by vhost to use RSA or ECDSA certificate.
    if (nullptr == (dtls_ = SSL_new(dtls_ctx_))) {
		logError << "ssl_new dtls failed";
		return false;
    }

    SSL_set_info_callback(dtls_, ssl_on_info);
	
	if (nullptr == (bio_in_ = BIO_new(BIO_s_mem()))) {
		logError << "BIO_in new failed";
		return false;
    }

	if (nullptr ==(bio_out_ = BIO_new(BIO_s_mem()))) {
		logError << "BIO_out new failed";
		BIO_free(bio_in_);
		return false;
	}
	SSL_set_bio(dtls_, bio_in_, bio_out_);
/*
	STACK_OF(SSL_CIPHER) *srvr = SSL_get_ciphers(dtls_);
    for (int i = 0; i < sk_SSL_CIPHER_num(srvr); ++i) {
    	const SSL_CIPHER *c = sk_SSL_CIPHER_value(srvr, i);
    	logInfo << "id [" << c->id << "] name " << c->name;
	}
*/
	// Dtls setup passive, as server role.
    SSL_set_accept_state(dtls_);	
	return true;
}

int DtlsSession::OnDtls(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len) {
    if (BIO_reset(bio_in_) != 1 || BIO_reset(bio_out_) != 1) {
		logError << "BIO_reset failed";
		return -1;
	}

    if (BIO_write(bio_in_, buf->data(), buf->size()) <= 0) {
        // TODO: 0 or -1 maybe block, use BIO_should_retry to check.
        logError << "BIO_write failed";
        return -1;
    }

	if (handshake_done_) {
		// TODO: support sctp
		return 0;
	}

	// Do handshake and get the result.
    int r0 = SSL_do_handshake(dtls_);
    uint8_t* out_bio_data = nullptr;
    int out_bio_len = BIO_get_mem_data(bio_out_, &out_bio_data);
	if (out_bio_len) 
		sock->send((char*)out_bio_data, out_bio_len, 1, addr, addr_len);
	
    int r1 = SSL_get_error(dtls_, r0);
    // Fatal SSL error, for example, no available suite when peer is DTLS 1.0 while we are DTLS 1.2.
    if (r0 < 0 && (r1 != SSL_ERROR_NONE && r1 != SSL_ERROR_WANT_READ && r1 != SSL_ERROR_WANT_WRITE)) {
		ERR_load_ERR_strings();
		ERR_load_crypto_strings();

		unsigned long ulErr = ERR_get_error(); 
		char szErrMsg[1024] = {0};
		char *pTmp = NULL;
		pTmp = ERR_error_string(ulErr, szErrMsg); 
		logError << "handshake r0 [" << r0 << "] r1 [" << r1 << "] msg [" << szErrMsg << "]";
		return -1;
    }
	
	if (SSL_ERROR_NONE == r1) {
		logInfo << "dtls handshake done";
		handshake_done_ = true;	
	}

	return 0;
}

int DtlsSession::GetSrtpKey(std::string& recv_key, std::string& send_key) {
	const int SRTP_MASTER_KEY_KEY_LEN = 16;
	const int SRTP_MASTER_KEY_SALT_LEN = 14;
	
    unsigned char material[SRTP_MASTER_KEY_LEN * 2] = {0};  // client(SRTP_MASTER_KEY_KEY_LEN + SRTP_MASTER_KEY_SALT_LEN) + server
    static const string dtls_srtp_lable = "EXTRACTOR-dtls_srtp";
    if (!SSL_export_keying_material(dtls_, material, sizeof(material), dtls_srtp_lable.c_str(), dtls_srtp_lable.size(), nullptr, 0, 0)) {
		logError << "SSL export key failed. result [" << ERR_get_error() << "]";
		return -1;
    }

    size_t offset = 0;
    std::string client_master_key(reinterpret_cast<char*>(material), SRTP_MASTER_KEY_KEY_LEN);
    offset += SRTP_MASTER_KEY_KEY_LEN;
    std::string server_master_key(reinterpret_cast<char*>(material + offset), SRTP_MASTER_KEY_KEY_LEN);
    offset += SRTP_MASTER_KEY_KEY_LEN;
    std::string client_master_salt(reinterpret_cast<char*>(material + offset), SRTP_MASTER_KEY_SALT_LEN);
    offset += SRTP_MASTER_KEY_SALT_LEN;
    std::string server_master_salt(reinterpret_cast<char*>(material + offset), SRTP_MASTER_KEY_SALT_LEN);

    recv_key = client_master_key + client_master_salt;
    send_key = server_master_key + server_master_salt;
    return 0;	
}

SrtpSession::SrtpSession() {

}

SrtpSession::~SrtpSession() {
    if (recv_ctx_)  
        srtp_dealloc(recv_ctx_);
	
    if (send_ctx_) 
        srtp_dealloc(send_ctx_);
}

bool SrtpSession::Init(const std::string recv_key, const std::string send_key) {
    srtp_policy_t policy;
    bzero(&policy, sizeof(policy));

    // TODO: Maybe we can use SRTP-GCM in future.
    // @see https://bugs.chromium.org/p/chromium/issues/detail?id=713701
    // @see https://groups.google.com/forum/#!topic/discuss-webrtc/PvCbWSetVAQ
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtp);
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&policy.rtcp);

    policy.ssrc.value = 0;
    // TODO: adjust window_size
    policy.window_size = 8192;
    policy.allow_repeat_tx = 1;
    policy.next = NULL;

    // init recv context
    policy.ssrc.type = ssrc_any_inbound;
    uint8_t *rkey = new uint8_t[recv_key.size()];
	SrsAutoFreeA(uint8_t, rkey);
    memcpy(rkey, recv_key.data(), recv_key.size());
    policy.key = rkey;

    srtp_err_status_t r0 = srtp_err_status_ok;
    if ((r0 = srtp_create(&recv_ctx_, &policy)) != srtp_err_status_ok) {
		logError << "srtp create failed, result [" << r0 << "]";
		return false;
    }

    policy.ssrc.type = ssrc_any_outbound;
    uint8_t *skey = new uint8_t[send_key.size()];
	SrsAutoFreeA(uint8_t, skey);
    memcpy(skey, send_key.data(), send_key.size());
    policy.key = skey;

    if ((r0 = srtp_create(&send_ctx_, &policy)) != srtp_err_status_ok)  
		logError << "srtp create failed, result [" << r0 << "]";		
	
    return r0 == srtp_err_status_ok;
}

int SrtpSession::ProtectRtp(void* rtp_hdr, int* len_ptr) {	
    if (!send_ctx_) {
		logError << "srtp session have not init";
		return -1;
    }

    srtp_err_status_t r0 = srtp_err_status_ok;
    if ((r0 = srtp_protect(send_ctx_, rtp_hdr, len_ptr)) != srtp_err_status_ok) 
		logError << "protect rtp failed, result [" << r0 << "]";

    return r0 == srtp_err_status_ok ? 0 : -1;
}

int SrtpSession::ProtectRtcp(const char* plaintext, char* cipher, int& nb_cipher) {
    if (!send_ctx_) {
		logError << "srtp session have not init";
		return -1;
    }

    memcpy(cipher, plaintext, nb_cipher);
    srtp_err_status_t r0 = srtp_err_status_ok;
    if ((r0 = srtp_protect_rtcp(send_ctx_, cipher, &nb_cipher)) != srtp_err_status_ok)
		logError << "protect rtcp failed, result [" << r0 << "]";

    return r0 == srtp_err_status_ok ? 0 : -1;
}

int SrtpSession::UnprotectRtp(const char* cipher, char* plaintext, int& nb_plaintext) {
    if (!recv_ctx_) {
		logError << "srtp session have not init";
		return -1;
    }

    memcpy(plaintext, cipher, nb_plaintext);
    srtp_err_status_t r0 = srtp_err_status_ok;
    if ((r0 = srtp_unprotect(recv_ctx_, plaintext, &nb_plaintext)) != srtp_err_status_ok) 
		logError << "unprotect rtp failed, result [" << r0 << "]";

    return r0 == srtp_err_status_ok ? 0 : -1;
}

int SrtpSession::UnprotectRtcp(const char* cipher, char* plaintext, int& nb_plaintext) {
    if (!recv_ctx_) {
		logError << "srtp session have not init";
		return -1;
    }

    memcpy(plaintext, cipher, nb_plaintext);
    srtp_err_status_t r0 = srtp_err_status_ok;
    if ((r0 = srtp_unprotect_rtcp(recv_ctx_, plaintext, &nb_plaintext)) != srtp_err_status_ok) 
		logError << "unprotect rtcp failed, result [" << r0 << "]";

    return r0 == srtp_err_status_ok ? 0 : -1;	
}

RtcSession::RtcSession() {
	dtls_session_.reset(new DtlsSession());
	if (!dtls_session_->Init()) 
		logError << "dtls session init failed";

	time_t now = time(nullptr);
	lastest_packet_receive_time_ = now;
	lastest_packet_send_time_ = now;
	_stat_time.update();
}

RtcSession::~RtcSession() {
	logInfo << "~RtcSession";
	// Broadcast::AuthInvoker invoker = [](const string &err) {
	// };
	// string status = "off";
	// MediaInfo info;
	// info._streamid = stream_;
	// info._app = app_;
	// NoticeCenter::Instance().emitEvent(Broadcast::kBroadcastPlayerNotify, info, invoker, status);

	// if(_from_Internal){
	// 	MultiMuxerPrivate::Ptr muxer = CephAdapterPool::Instance().GetMuxer(stream_);
	// 	if (muxer) {
	// 		muxer->delInternalPlayerCount();
	// 	}
	// }

	if (peer_addr_)
		free(peer_addr_);
}

int RtcSession::OnStunPacket(Socket::Ptr sock, SrsStunPacket& stun_pkt, struct sockaddr* addr, int addr_len) {
	if (!stun_pkt.is_binding_request()) {
		logError << "only handle stun bind request";
		return -1;
	}

	logInfo << "get a stun packet";

	SrsStunPacket stun_rsp;
	stun_rsp.set_message_type(BindingResponse);
	stun_rsp.set_local_ufrag(stun_pkt.get_remote_ufrag());
    stun_rsp.set_remote_ufrag(stun_pkt.get_local_ufrag());
    stun_rsp.set_transcation_id(stun_pkt.get_transcation_id());
	struct sockaddr_in* peer_addr = (struct sockaddr_in*)addr;
	stun_rsp.set_mapped_address(ntohl(peer_addr->sin_addr.s_addr));
	stun_rsp.set_mapped_port(ntohs(peer_addr->sin_port));

    char buf[kRtpPacketSize];
    SrsBuffer* stream = new SrsBuffer(buf, sizeof(buf));
    SrsAutoFree(SrsBuffer, stream);
	if (0 != stun_rsp.encode(local_sdp_.get_ice_pwd(), stream)) {
		logError << "encode stun response failed";
		return -1;
	}

	sock->send(stream->data(), stream->pos(), 1, addr, addr_len);
	lastest_packet_receive_time_ = time(nullptr);
	return 0;
}

int RtcSession::OnDtls(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len) {
	return dtls_session_->OnDtls(sock, buf, addr, addr_len);
}

int RtcSession::OnRtp(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len) {
	// server should not receive rtp packet
	logWarn << "receive rtp packet on server side";
	return 0;
}

int RtcSession::ParseRtcpType( char* buf){
	uint8_t* data = (uint8_t*)buf;
	//size_t len = buf->size();
	int type = data[1];
	return type;
}

void RtcSession::ParseNackId( char* buf,vector<int> &nack_id){
	uint16_t* data = (uint16_t*)buf;
	//解析nack pid 
	short nack_pid_ori = data[6];
	auto nack_pid = ntohs(nack_pid_ori);
	//logInfo<<"nack_pid  :  "<<nack_pid<<endl;
	if(nack_pid > 0){
		auto nack_blp_ori = ntohs(data[7]);
		//auto nack_blp_ori = data[7];
		nack_id.emplace_back(nack_pid);
		auto nack_blp = std::bitset<16>(nack_blp_ori);
		//logInfo<<" bitset  :  "<<nack_blp<<"; nack blp ori(机器码) is : "<<nack_blp_ori<<"; nack blp (网络码)is : "<<data[7]<<endl;  //bitset最低位为bitset[0]，从右向左递增
		for(int i=0;i<16;i++){
			if(nack_blp[i] == 1){
				nack_id.emplace_back(nack_pid+i+1);
			}
		}

		// for(auto i:nack_id){
		// 	uint64_t session_id = SockAddrHash((struct sockaddr_in*)peer_addr_);
		// 	logInfo<<"缺失frame: "<<i<<" ; session_id is  : "<<session_id<<endl;
		// }
	}
}

//收到Genetic RTP Feedback 报文,进行丢包重传
void RtcSession::ResendRtp(char* buf){

	int packet_type = ParseRtcpType(buf);
	//uint64_t session_id = SockAddrHash((struct sockaddr_in*)peer_addr_);
	//logInfo<<"====packet_type is : "<<packet_type<<"; session_id is  : "<<session_id<<endl;
	if(packet_type == 205){
		// std::weak_ptr<RtcSession> weakSelf = dynamic_pointer_cast<RtcSession>(shared_from_this());
		// auto strongSelf = weakSelf.lock();
		// if (!strongSelf) {
		// 	return;
		// }
		_rtcp_nack_per_10s ++;
		vector<int> nack_id;
		ParseNackId(buf,nack_id); //得到丢包序列号
		for(auto id : nack_id){
			_rtp_loss_per_10s++;
			auto index = id % 256;
			if(_rtp_cache[index]){
				if(_rtp_cache[index]->getSeq() == id){
					SendMedia(_rtp_cache[index]);
					_rtp_resend_per_10s++;
					if(_first_resend){
						_first_resend = false;
						logInfo<<"resend rtp success, stream is : "<<stream_<<endl;
					}
				}
			}
		}

	}

}

void RtcSession::NackHeartBeat(){
	uint32_t heartbeatTime = 10;
	int sec = _stat_time.startToNow() / 1000;
    if (sec >= heartbeatTime) {
		uint64_t rtp_send_per_10s = _rtp_send_per_10s;
		uint64_t rtcp_nack_per_10s = _rtcp_nack_per_10s;
		uint64_t rtp_loss_per_10s = _rtp_loss_per_10s;
		uint64_t rtp_resend_per_10s = _rtp_resend_per_10s;
		uint64_t send = rtp_send_per_10s - rtp_resend_per_10s;
		lossPercent = rtp_loss_per_10s == 0 ? 0 : rtp_loss_per_10s * 1.0 / send;

		logInfo<<" nack heartbeat : streamname "<<stream_<<" : === rtp lose percent : "<<lossPercent<<" === ; rtp_send_per_10s : "<<send<<" ; rtp_loss_per_10s : "<<rtp_loss_per_10s<<" ; rtp_resend_per_10s : "<<rtp_resend_per_10s<<endl;
		_rtcp_nack_per_10s -= rtcp_nack_per_10s;
		_rtp_loss_per_10s -= rtp_loss_per_10s;
		_rtp_resend_per_10s -= rtp_resend_per_10s;
		_rtp_send_per_10s -= rtp_send_per_10s;
		_stat_time.update();
	}
}

int RtcSession::OnRtcp(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len) {
	if (!srtp_session_) {
		srtp_session_.reset(new SrtpSession());
		std::string recv_key, send_key;
		if (0 != dtls_session_->GetSrtpKey(recv_key, send_key) || 
			!srtp_session_->Init(recv_key, send_key)) {
			logError << "dtls get srtp key failed OR srtp session init failed";
		}
		OnReceivedRtcp(sock, addr, addr_len);
	}

	NackHeartBeat();

	lastest_packet_receive_time_ = time(nullptr);
	char plaintext[kRtpPacketSize];
    int nb_plaintext = buf->size();
	if (0 != srtp_session_->UnprotectRtcp(buf->data(), plaintext, nb_plaintext)) 
		return -1;
	ResendRtp(plaintext);
	return 0;
}

void RtcSession::PutRtp(RtpPacket::Ptr rtp){
	std::lock_guard<std::mutex> lock(_mutex_rtp_cache);
	int index = rtp->getSeq() % 256;
	//logInfo<<"rtp sequence"<<rtp->sequence<<endl;
	_rtp_cache[index] = rtp;
	//uint64_t session_id = SockAddrHash((struct sockaddr_in*)peer_addr_);
	//logInfo<<"save the rtp package: "<< rtp->sequence <<" ; session_id is :"<<session_id<<endl;
}

int RtcSession::OnReceivedRtcp(Socket::Ptr sock, struct sockaddr* addr, int addr_len) {
	string path = "/" + app_ + "/" + stream_;
	// WebrtcMediaSource::Ptr source = dynamic_pointer_cast<WebrtcMediaSource>(
	// 		MediaSource::get(path, DEFAULT_VHOST, PROTOCOL_WEBRTC, DEFAULT_TYPE));
	// if (!source) {
	// 	logError << "cannot find rtc source: app [" << app_ << "] stream [" << stream_ << "]";
	// 	return -1;
	// }
	recv_sock_ = sock;
	peer_addr_len = addr_len;
	//peer_addr_ = (struct sockaddr*)(new uint8_t[addr_len]);
	peer_addr_ = (struct sockaddr*)malloc(addr_len);
	memcpy(peer_addr_, addr, addr_len);
	if(peer_addr_){
		// _peerIp = SockUtil::inet_ntoa(((struct sockaddr_in *) peer_addr_)->sin_addr);
		_peerPort = ntohs(((struct sockaddr_in *) peer_addr_)->sin_port);
	}

	// if (_source.lock()) {
	// 	return 0;
	// }

	weak_ptr<RtcSession> wSelf = shared_from_this();
	UrlParser _urlParser;
    _urlParser.path_ = path;
    _urlParser.vhost_ = DEFAULT_VHOST;
    _urlParser.protocol_ = PROTOCOL_WEBRTC;
    _urlParser.type_ = DEFAULT_TYPE;

    MediaSource::getOrCreateAsync(_urlParser.path_, _urlParser.vhost_, _urlParser.protocol_, _urlParser.type_, 
    [wSelf, sock, addr, addr_len](const MediaSource::Ptr &src){
        auto self = wSelf.lock();
        if (!self) {
            return ;
        }

		auto rtcSrc = dynamic_pointer_cast<WebrtcMediaSource>(src);
		if (!rtcSrc) {
			logError << "get rtc Src failed";
			return ;
		}

		self->_source = rtcSrc;

        sock->getLoop()->async([wSelf, rtcSrc, sock, addr, addr_len](){
            auto self = wSelf.lock();
            if (!self) {
                return ;
            }

            logInfo << "bind to ring buffer: app [" << self->app_ << "] stream [" << self->stream_ << "]";
			
			self->ring_reader_ = rtcSrc->getRing()->attach(sock->getLoop());
			self->ring_reader_->setReadCB([wSelf](const WebrtcMediaSource::DataType &pkt) {
				auto strongSelf = wSelf.lock();
				if (!strongSelf) {
					return;
				}

				int i = 0;
				int len = pkt->size() - 1;
				auto pktlist = *(pkt.get());

				for (auto& rtp: pktlist) {
					strongSelf->PutRtp(rtp);  //save rtp package
					strongSelf->SendMedia(rtp);
				};
			});
        }, true);
    }, 
    [wSelf,_urlParser]() -> MediaSource::Ptr {
        auto self = wSelf.lock();
        if (!self) {
            return nullptr;
        }
        return make_shared<WebrtcMediaSource>(_urlParser, nullptr, true);
    }, this);

	// if(_from_Internal){
	// 	MultiMuxerPrivate::Ptr muxer = CephAdapterPool::Instance().GetMuxer(stream_);
	// 	if (muxer) {
	// 		muxer->addInternalPlayerCount();
	// 	}else{
	// 		logInfo << "未找到muxer streamid: " << stream_<< endl;
	// 	}
	// }
	// logInfo <<"play streamid is : "<<stream_<< " ; _from_Internal : "<<_from_Internal<<endl;

	// GET_CONFIG(string,vssVersion,General::kVersion);
	// if(vssVersion == "vss" && !_from_Internal && !trafficTimer_){
    //     GET_CONFIG(uint32_t,reportSec,"general.trafficDownReportTime");
    //     if (reportSec == 0) {
    //         reportSec = 10;
    //     }
	// 	int reportInterval = reportSec;
	// 	std::weak_ptr<RtcSession> weakSelf = dynamic_pointer_cast<RtcSession>(shared_from_this());
	// 	trafficTimer_ = std::make_shared<Timer>(reportInterval, [weakSelf,reportInterval](){	
	// 		auto strongSelf = weakSelf.lock();
	// 		if (!strongSelf) {
	// 			return false;
	// 		}
	// 		auto bytes = strongSelf->getBytes();
	// 		string trafficType = "downstream";
	// 		MediaInfo info;
	// 		info._schema = "webrtc";
	// 		info._app = strongSelf->app_;
	// 		info._streamid = strongSelf->stream_;
	// 		NoticeCenter::Instance().emitEvent(Broadcast::kBroadcastTraffic, trafficType, info, bytes, reportInterval, strongSelf->_peerIp, strongSelf->_peerPort);
	// 		return true;
	// 	},sock->getPoller());	
	// }

	return 0;
}

uint64_t RtcSession::getBytes(){
    auto bytes = _bytes;
    _bytes = 0;
    return bytes;
}

void RtcSession::SendMedia(const RtpPacket::Ptr rtp) {
	int nb_cipher = rtp->size() - 4;
    char data[kRtpPacketSize];
    memcpy(data, rtp->data() + 4, nb_cipher);
	auto sdp_video_pt = local_sdp_.get_video_pt();
	_video_payload_type = sdp_video_pt == 0 ? 106 : sdp_video_pt;
	//当前webrtc播放只支持h264，h265会被前端拦截，不提供rtc播放界面，因此只需要判断h264的nal_type
	//payload中第一个字节的后5位判断，如果是FU-A,则保持原rtp包中mark值
	//否则，mark值为false
	const uint8_t *frame = (uint8_t *) rtp->data() + 16;
    // int length = rtp->size() - rtp->offset;
    int nal_type = *frame & 0x1F;
	if(nal_type == 28){
		data[1] = (data[1] & 0x80) | (rtp->type_ == "audio" ? 8 : _video_payload_type);
	}else{
		data[1] = (0 << 7) | (rtp->type_ == "audio" ? 8 : _video_payload_type);
	}
	uint32_t ssrc = htonl(rtp->type_ == "audio" ? 10000 : 20000);
	memcpy(data + 8, &ssrc, sizeof(ssrc));	

	// FILE* fp = fopen("test.rtp", "ab+");
	// fwrite(data, nb_cipher, 1, fp);
	// fclose(fp);

	if (0 == srtp_session_->ProtectRtp(data, &nb_cipher)) {
		recv_sock_->send(data, nb_cipher, 1, peer_addr_, peer_addr_len);
		_rtp_send_per_10s++;
		lastest_packet_send_time_ = time(nullptr);
	}
	_bytes += nb_cipher;
}

void RtcSession::SendAudioMedia(const RtpPacket::Ptr rtp) {

}


bool RtcSession::TimeToRemove() {
	time_t now = time(nullptr);
	static const uint32_t kMaxIdleSecons = 30;
	uint32_t received_interval = now - lastest_packet_receive_time_;
	uint32_t send_interval = now - lastest_packet_send_time_;
	bool removed = (received_interval >= kMaxIdleSecons || send_interval >= kMaxIdleSecons);
	if (removed) 
		logInfo << "time to remove rtc session: receive interval [" << received_interval << "] "
			  << "send interval [" << send_interval << "]";
	return removed;
}

RtcServer::RtcServer() = default;
RtcServer::~RtcServer() = default;

RtcServer& RtcServer::Instance() {
	static RtcServer instance;
	return instance;
}

int RtcServer::OnReceivedSdp(const std::string& url,
							 const std::string& remote_sdp_str,
							 std::ostringstream& local_sdp_str,
							 std::string& session_id) { 
	SrsSdp 	remote_sdp;
	if (0 != remote_sdp.parse(remote_sdp_str)) {
		logError << "parse url [" << url << "] sdp failed";
		return -1;
	}

	SrsSdp local_sdp;
	UrlParser parser;
	parser.parse(url);
	// MediaInfo info(url);
	std::string msid = parser.path_;
	if (0 != ExchangeSdp(msid, remote_sdp, local_sdp)) {
		logError << "exchange url [" << url << "] sdp failed";
		return -1;
	}
	std::shared_ptr<RtcSession> session = std::make_shared<RtcSession>();
    local_sdp.set_ice_ufrag("MwfhGFgM");
    local_sdp.set_ice_pwd("6U2CxasY8FAbyVenAzbtxMh4IqQryUtC");
    local_sdp.set_fingerprint_algo("sha-256");
    local_sdp.set_fingerprint(global_certificate_->GetFingerprint());	

	// GET_CONFIG(string,rtc_ip,"rtc.ip");
	// GET_CONFIG(int, rtc_port, "rtc.port");
	string rtc_ip = "172.24.6.238";
	int rtc_port = 7000;
	local_sdp.add_candidate(rtc_ip, rtc_port, "host");
	
	if (0 != local_sdp.encode(local_sdp_str)) {
		logError << "encode url [" << url << "] sdp failed";
		return -1;
	}
    // auto params = Parser::parseArgs(info._param_strs);
    // if (params.find("fromInternal") != params.end()) {
    //     if("1" == params["fromInternal"]){
    //         session->SetInternalFlag();
    //     }
    // }
	session->SetLocalSdp(local_sdp);
	session->SetRemoteSdp(remote_sdp);
	auto vec = split(parser.path_, "/");
	session->SetSource(vec[0], vec[1]);
	session_id = local_sdp.get_ice_ufrag() + ":" + remote_sdp.get_ice_ufrag();
	std::lock_guard<std::mutex> lock(mutex_);
	session_map_[session_id] = session;
	return 0;
}


void RtcServer::Start(EventLoop::Ptr loop, const uint16_t local_port, const std::string local_ip) {
	logInfo << "bind udp port: [" << local_port << "]";
	Socket::Ptr socket = make_shared<Socket>(loop);
	socket->createSocket(SOCKET_UDP);

	if (socket->bind(local_port, local_ip.data()) != 0) {
		logError << "bind udp port [" << local_port << "] failed";
		return;
	}
	socket->addToEpoll();

    //设置udp socket读缓存
    // SockUtil::setRecvBuf(sock->rawFD(), 16 * 1024 * 1024);

	// sock->setOnRead([sock, this](const Buffer::Ptr& buf, struct sockaddr* addr, int addr_len){
	// 	this->OnUdpPacket(sock, buf, addr, addr_len);
	// });

	socket->setReadCb([socket, this](const StreamBuffer::Ptr& buffer, struct sockaddr* addr, int len){
		// create rtpmanager
		OnUdpPacket(socket, buffer, addr, len);

		return 0;
	});
	socket->setRecvBuf(4 * 1024 * 1024);

	recv_sock_ = socket;

	global_certificate_ = new DtlsCertificate();
	if (!global_certificate_->Initialize())
		logError << "dtls certificate init failed";

    // timer_ = std::make_shared<Timer>(10, [&](){		
	// 	RtcServer::Instance().TimerProcess();
    //     return true;
    // },loop);	
}

void RtcServer::OnUdpPacket(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len) {
	size_t pkt_type = GuessPktType(buf);
	if (kStunPkt == pkt_type)
		OnStunPacket(sock, buf, addr, addr_len);
	else if (kDtlsPkt == pkt_type)
		OnDtlsPacket(sock, buf, addr, addr_len);
	else if (kRtpPkt == pkt_type)
		OnRtpPacket(sock, buf, addr, addr_len);
	else if (kRtcpPkt == pkt_type)
		OnRtcpPacket(sock, buf, addr, addr_len);
	else
		logWarn << "received unkown udp packet";
}

size_t RtcServer::GuessPktType(const StreamBuffer::Ptr& buf) {
	uint8_t* data = (uint8_t*)buf->data();
	size_t len = buf->size();

	// 0x0001 stun bind request
	if (data[0] == 0 && data[1] == 1)
		return kStunPkt;

	if (len >= 13 && data[0] >= 16 && data[0] <= 64)
		return kDtlsPkt;

	// 0x80
	if (len >= 12 && (data[0] & 0xC0) == 0x80) {
		return (data[1] < 128) ? kRtpPkt : kRtcpPkt;
	}
		
		
	return kUnkown;
}

void RtcServer::OnStunPacket(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len) {
	SrsStunPacket stun_req;
	if (0 != stun_req.decode(buf->data(), buf->size())) {
		logError << "decode stun packet failed";
		return;
	}

	std::string username = stun_req.get_username();
	RtcSession::Ptr session = FindRtcSession(username);
	if (session && 0 == session->OnStunPacket(sock, stun_req, addr, addr_len)) {
		uint64_t hash = SockAddrHash((struct sockaddr_in*)addr);
		std::lock_guard<std::mutex> lock(mutex_);
		auto it = session_addr_map_.find(hash);
		if (it == session_addr_map_.end())
			session_addr_map_[hash] = session;	
	}
}


void RtcServer::OnDtlsPacket(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len) {
	uint64_t hash = SockAddrHash((struct sockaddr_in*)addr);
	logInfo << "on dtls packet, size is [" << buf->size() << "]";
	RtcSession::Ptr session = FindRtcSession(hash);
	if (session)
		session->OnDtls(sock, buf, addr, addr_len);
}

void RtcServer::OnRtpPacket(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len) {
	logInfo << "on rtp packet, size is [" << buf->size() << "]";
	uint64_t hash = SockAddrHash((struct sockaddr_in*)addr);
	RtcSession::Ptr session = FindRtcSession(hash);
	if (session)
		session->OnRtp(sock, buf, addr, addr_len);
}

void RtcServer::OnRtcpPacket(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len) {
	//logInfo << "on rtcp packet. size is [" << buf->size() << "]";
	uint64_t hash = SockAddrHash((struct sockaddr_in*)addr);
	RtcSession::Ptr session = FindRtcSession(hash);
	if (session)
		session->OnRtcp(sock, buf, addr, addr_len);
}

RtcSession::Ptr RtcServer::FindRtcSession(const std::string key) {
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = session_map_.find(key);
	return it != session_map_.end() ? it->second : nullptr;
}

RtcSession::Ptr RtcServer::FindRtcSession(const uint64_t hash) {
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = session_addr_map_.find(hash);
	return it != session_addr_map_.end() ? it->second : nullptr;
}

void RtcServer::TimerProcess() {
	std::lock_guard<std::mutex> lock(mutex_);
	auto it = session_map_.begin();
	while (it != session_map_.end()) {
		if (it->second->TimeToRemove()) {
			it = session_map_.erase(it);
		} else {
			++it;
		}
	}

	auto iter = session_addr_map_.begin();
	while (iter != session_addr_map_.end()) {
		if (iter->second->TimeToRemove()) {
			iter = session_addr_map_.erase(iter);
		} else {
			++iter;
		}
	}	
}
