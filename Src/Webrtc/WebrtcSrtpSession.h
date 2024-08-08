#ifndef SrtpSession_H
#define SrtpSession_H

#include <string>
#include "srtp2/srtp.h"

using namespace std;


class SrtpSession {
  public:
	SrtpSession();
	~SrtpSession();

	bool init(const std::string recvKey, const std::string sendKey);

    // Encrypt the input plaintext to output cipher with nb_cipher bytes.
    // @remark Note that the nb_cipher is the size of input plaintext, and 
    // it also is the length of output cipher when return.
    int protectRtp(void* rtp_hdr, int* len_ptr);
    int protectRtcp(const char* plaintext, char* cipher, int& nb_cipher);

    // Decrypt the input cipher to output cipher with nb_cipher bytes.
    // @remark Note that the nb_plaintext is the size of input cipher, and 
    // it also is the length of output plaintext when return.
    int unprotectRtp(const char* cipher, char* plaintext, int& nb_plaintext);
    int unprotectRtcp(const char* cipher, char* plaintext, int& nb_plaintext);
	
  private:
    srtp_t recvCtx_;
    srtp_t sendCtx_;  	
};


#endif //SrtpSession_H
