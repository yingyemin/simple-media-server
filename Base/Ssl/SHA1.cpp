#include "SHA1.h"
#include <openssl/hmac.h>
#include <openssl/sha.h>

std::string SHA1::encode(const std::string& key) {
    unsigned char digest[SHA_DIGEST_LENGTH];
    unsigned int digest_len;
    auto out = ::SHA1((unsigned char *)key.c_str(), key.length(), digest);

    return std::string(reinterpret_cast<char*>(out), SHA_DIGEST_LENGTH);
}