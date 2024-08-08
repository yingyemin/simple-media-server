#ifndef DtlsSession_H
#define DtlsSession_H

#include "Net/Socket.h"

#include <openssl/ssl.h>
#include <openssl/err.h>

class DtlsCertificate {
public:
	DtlsCertificate() {}
	virtual ~DtlsCertificate();
    bool init();
    X509* getCert() { return dtlsCert_; }
    EVP_PKEY* getPublicKey() { return dtlsPkey_; }
    EC_KEY* getEcdsaKey() { return eckey_; }
    std::string getFingerprint() { return fingerprint_; }

private:
    bool ecdsaMode_ = true;
    X509* dtlsCert_ = nullptr;
    EVP_PKEY* dtlsPkey_ = nullptr;
    EC_KEY* eckey_ = nullptr;
	std::string fingerprint_;
};

class DtlsSession {
public:
	DtlsSession(const string& role = "server");
	~DtlsSession();

	bool init(const shared_ptr<DtlsCertificate>& dtlsCert);
	int onDtls(Socket::Ptr sock, const Buffer::Ptr& buf, struct sockaddr* addr, int addr_len);
	int startActiveHandshake(Socket::Ptr sock);
	int32_t decodeHandshake(Socket::Ptr sock, char *data, int32_t nb_data);
	int checkTimeout(Socket::Ptr sock);
	int getSrtpKey(std::string& recv_key, std::string& send_key);
	void setOnHandshakeDone(const function<void()>& cb) {_onHandshakeDone = cb;}
	
private:
	string _role;
  	SSL_CTX* dtlsCtx_ = nullptr;
    SSL* dtls_ = nullptr;
    BIO* bioIn_ = nullptr;
    BIO* bioOut_ = nullptr;
	bool handshakeFinish_ = false;
	function<void()> _onHandshakeDone;
};

#endif