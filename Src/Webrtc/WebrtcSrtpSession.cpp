#include <cstring>
#include <iostream>

#include "WebrtcSrtpSession.h"
#include "Log/Logger.h"

using namespace std;

SrtpSession::SrtpSession() {

}

SrtpSession::~SrtpSession() {
    if (recvCtx_)  
        srtp_dealloc(recvCtx_);
	
    if (sendCtx_) 
        srtp_dealloc(sendCtx_);
}

bool SrtpSession::init(const std::string recvKey, const std::string sendKey) {
    logInfo << "SrtpSession::init ==================== ";
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
    shared_ptr<uint8_t> rkey(new uint8_t[recvKey.size()], [](uint8_t* p){delete[] p;});
    
    memcpy(rkey.get(), recvKey.data(), recvKey.size());
    policy.key = rkey.get();

    srtp_err_status_t res = srtp_err_status_ok;
    if ((res = srtp_create(&recvCtx_, &policy)) != srtp_err_status_ok) {
		logError << "srtp create failed, result [" << res << "]";
		return false;
    }

    policy.ssrc.type = ssrc_any_outbound;
    shared_ptr<uint8_t> skey(new uint8_t[sendKey.size()], [](uint8_t* p){delete[] p;});
    memcpy(skey.get(), sendKey.data(), sendKey.size());
    policy.key = skey.get();

    if ((res = srtp_create(&sendCtx_, &policy)) != srtp_err_status_ok)  
		logError << "srtp create failed, result [" << res << "]";		
	
    return res == srtp_err_status_ok;
}

int SrtpSession::protectRtp(void* rtp, int* len) {	
    if (!sendCtx_) {
		logError << "srtp session have not init";
		return -1;
    }

    srtp_err_status_t res = srtp_err_status_ok;
    if ((res = srtp_protect(sendCtx_, rtp, len)) != srtp_err_status_ok) 
		logError << "protect rtp failed, result [" << res << "]";

    return res == srtp_err_status_ok ? 0 : -1;
}

int SrtpSession::protectRtcp(const char* src, char* dst, int& dstLen) {
    if (!sendCtx_) {
		logError << "srtp session have not init";
		return -1;
    }

    memcpy(dst, src, dstLen);
    srtp_err_status_t res = srtp_err_status_ok;
    if ((res = srtp_protect_rtcp(sendCtx_, dst, &dstLen)) != srtp_err_status_ok)
		logError << "protect rtcp failed, result [" << res << "]";

    return res == srtp_err_status_ok ? 0 : -1;
}

int SrtpSession::unprotectRtp(const char* src, char* dst, int& dstLen) {
    if (!recvCtx_) {
		logError << "srtp session have not init";
		return -1;
    }

    memcpy(dst, src, dstLen);
    srtp_err_status_t res = srtp_err_status_ok;
    if ((res = srtp_unprotect(recvCtx_, dst, &dstLen)) != srtp_err_status_ok) 
		logError << "unprotect rtp failed, result [" << res << "]";

    return res == srtp_err_status_ok ? 0 : -1;
}

int SrtpSession::unprotectRtcp(const char* src, char* dst, int& dstLen) {
    if (!recvCtx_) {
		logError << "srtp session have not init";
		return -1;
    }

    memcpy(dst, src, dstLen);
    srtp_err_status_t res = srtp_err_status_ok;
    if ((res = srtp_unprotect_rtcp(recvCtx_, dst, &dstLen)) != srtp_err_status_ok) 
		logError << "unprotect rtcp failed, result [" << res << "]";

    return res == srtp_err_status_ok ? 0 : -1;	
}