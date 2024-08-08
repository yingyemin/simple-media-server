/*
 * 
 *
 * 
 *
 * 
 * 
 * 
 */

#ifndef CTVSS_RTCSERVER_H
#define CTVSS_RTCSERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdint.h>
#include <sstream>
#include <mutex>
#include <unordered_map>
#include "EventPoller/EventLoop.h"
#include "RtcStun.h"
#include "Net/Socket.h"
#include "RtcSdp.h"
#include "openssl/ssl.h"
#include "srtp2/srtp.h"
#include "WebrtcMediaSource.h"
#include "EventPoller/Timer.h"
#include "Util/TimeClock.h"

class DtlsCertificate {
  public:
	DtlsCertificate() {}
	virtual ~DtlsCertificate();
    bool Initialize();
    X509* GetCert() { return dtls_cert_; }
    EVP_PKEY* GetPublicKey() { return dtls_pkey_; }
    EC_KEY* GetEcdsaKey() { return eckey_; }
    std::string GetFingerprint() { return fingerprint_; }

  private:
    bool ecdsa_mode_ = true;
    X509* dtls_cert_ = nullptr;
    EVP_PKEY* dtls_pkey_ = nullptr;
    EC_KEY* eckey_ = nullptr;
	std::string fingerprint_;
};

class DtlsSession {
  public:
	DtlsSession(){}
	~DtlsSession();

	bool Init();
	int OnDtls(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len);
	int GetSrtpKey(std::string& recv_key, std::string& send_key);
	
  private:
  	SSL_CTX* dtls_ctx_ = nullptr;
    SSL* dtls_ = nullptr;
    BIO* bio_in_ = nullptr;
    BIO* bio_out_ = nullptr;
	bool handshake_done_ = false;
};


class SrtpSession {
  public:
	SrtpSession();
	~SrtpSession();

	bool Init(const std::string recv_key, const std::string send_key);

    // Encrypt the input plaintext to output cipher with nb_cipher bytes.
    // @remark Note that the nb_cipher is the size of input plaintext, and 
    // it also is the length of output cipher when return.
    int ProtectRtp(void* rtp_hdr, int* len_ptr);
    int ProtectRtcp(const char* plaintext, char* cipher, int& nb_cipher);

    // Decrypt the input cipher to output cipher with nb_cipher bytes.
    // @remark Note that the nb_plaintext is the size of input cipher, and 
    // it also is the length of output plaintext when return.
    int UnprotectRtp(const char* cipher, char* plaintext, int& nb_plaintext);
    int UnprotectRtcp(const char* cipher, char* plaintext, int& nb_plaintext);
	
  private:
    srtp_t recv_ctx_;
    srtp_t send_ctx_;  	
};


class RtcSession : public std::enable_shared_from_this<RtcSession> {
  public:
	typedef std::shared_ptr<RtcSession> Ptr;
	
	RtcSession();
	~RtcSession();

	int OnStunPacket(Socket::Ptr sock, SrsStunPacket& stun_pkt, struct sockaddr* addr, int addr_len);
	int OnDtls(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len);
	int OnRtp(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len);
	int OnRtcp(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len);
	
	void SetLocalSdp(SrsSdp& sdp) { local_sdp_ = sdp; }
	void SetRemoteSdp(SrsSdp& sdp) { remote_sdp_ = sdp; }
	void SetSource(const std::string app, const std::string stream) {
		app_ = app;
		stream_ = stream;
	}
	void SetInternalFlag(){
		_from_Internal = true;
	}
	bool TimeToRemove();
  private:
  	int OnReceivedRtcp(Socket::Ptr sock, struct sockaddr* addr, int addr_len);
	void SendMedia(const RtpPacket::Ptr rtp);
	void SendAudioMedia(const RtpPacket::Ptr rtp);
	void PutRtp(RtpPacket::Ptr rtp);   //save the rtp package in cache
	int ParseRtcpType(char* buf);
	void ParseNackId(char* buf,vector<int> &nack_id);
	void ResendRtp(char* buf);
	void NackHeartBeat();
	uint64_t getBytes();
	std::string app_;
	std::string stream_;
  	SrsSdp local_sdp_;
	SrsSdp remote_sdp_;
	Socket::Ptr	recv_sock_;
	struct sockaddr* peer_addr_ = nullptr;
	int peer_addr_len = 0;
	uint64_t _rtcp_nack_per_10s = 0;
	uint64_t _rtp_resend_per_10s = 0;
	uint64_t _rtp_loss_per_10s = 0;
	uint64_t _rtp_send_per_10s = 0;
	float lossPercent = 0;
	TimeClock _stat_time;

	uint64_t _bytes = 0;
    TimeClock _trafficTicker;
	Timer::Ptr trafficTimer_;

	time_t lastest_packet_send_time_ = 0;
	time_t lastest_packet_receive_time_ = 0;
	std::shared_ptr<DtlsSession> dtls_session_;
	std::shared_ptr<SrtpSession> srtp_session_;
	WebrtcMediaSource::QueType::DataQueReaderT::Ptr ring_reader_;
	WebrtcMediaSource::Wptr _source;
	
	//缓存rtp报文,大小为256
	std::vector<RtpPacket::Ptr> _rtp_cache = std::vector<RtpPacket::Ptr>{256,nullptr};
	std::mutex _mutex_rtp_cache;
	bool _first_resend = true;
	int _video_payload_type = 106;
	bool _from_Internal = false;
	string _peerIp = "";
    uint16_t _peerPort = 0;
};

class RtcServer {
  public:
	RtcServer();
	~RtcServer();

	static RtcServer& Instance();

	int OnReceivedSdp(const std::string& url,
					  const std::string& remote_sdp_str,
					  std::ostringstream& local_sdp_str,
					  std::string& session_id);
	
	void Start(EventLoop::Ptr loop, const uint16_t local_port, const std::string local_ip);
	
  private:
  	enum { kStunPkt, kDtlsPkt, kRtpPkt, kRtcpPkt, kUnkown };
	void OnUdpPacket(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len);
	size_t GuessPktType(const StreamBuffer::Ptr& buf);
	void OnStunPacket(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len);
	void OnDtlsPacket(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len);
	void OnRtpPacket(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len);
	void OnRtcpPacket(Socket::Ptr sock, const StreamBuffer::Ptr& buf, struct sockaddr* addr, int addr_len);
	RtcSession::Ptr FindRtcSession(const std::string key);
	RtcSession::Ptr FindRtcSession(const uint64_t key);
	void TimerProcess();
	
	Timer::Ptr timer_;
	Socket::Ptr recv_sock_;
	std::mutex mutex_;
	std::unordered_map<std::string, RtcSession::Ptr> session_map_;
	std::unordered_map<uint64_t, RtcSession::Ptr> session_addr_map_;
};

#endif
