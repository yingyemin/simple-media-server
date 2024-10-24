﻿#ifndef DtlsSession_H
#define DtlsSession_H

#include "Net/Socket.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

#pragma pack(push, 1)
struct DtlsHeader {
    uint8_t content_type;
    uint16_t dtls_version;
    uint16_t epoch;
    uint8_t seq[6];
    uint16_t length;
    uint8_t payload[1];
};
#pragma pack(pop)

class DtlsCertificate {
public:
	DtlsCertificate() {}
	virtual ~DtlsCertificate();
    bool init();
    X509* getCert() { return _dtlsCert; }
    EVP_PKEY* getPublicKey() { return _dtlsPkey; }
    EC_KEY* getEcdsaKey() { return _eckey; }
    std::string getFingerprint() { return _fingerprint; }

private:
    bool _ecdsaMode = true;
    X509* _dtlsCert = nullptr;
    EVP_PKEY* _dtlsPkey = nullptr;
    EC_KEY* _eckey = nullptr;
	std::string _fingerprint;
};

class DtlsSession {
public:
	DtlsSession(const string& role = "server");
	~DtlsSession();

	bool init(const shared_ptr<DtlsCertificate>& dtlsCert);
	int onDtls(Socket::Ptr sock, const Buffer::Ptr& buf, struct sockaddr* addr, int addr_len);
	int startActiveHandshake(Socket::Ptr sock);
	int32_t decodeHandshake(Socket::Ptr sock, char *data, int32_t nb_data);
	void sendApplicationData(Socket::Ptr socket, const char* data, int len);
	int checkTimeout(Socket::Ptr sock);
	int getSrtpKey(std::string& recv_key, std::string& send_key);
	void setOnHandshakeDone(const function<void()>& cb) {_onHandshakeDone = cb;}
	void setOnRecvApplicationData(const function<void(const char* data, int len)>& cb) {_onRecvApplicationData = cb;}
	
private:
	string _role;
  	SSL_CTX* _dtlsCtx = nullptr;
    SSL* _dtls = nullptr;
    BIO* _bioIn = nullptr;
    BIO* _bioOut = nullptr;
	bool _handshakeFinish = false;
	uint8_t _sslReadBuffer[2000] = {0};
	function<void()> _onHandshakeDone;
	function<void(const char* data, int len)> _onRecvApplicationData;
};

#endif