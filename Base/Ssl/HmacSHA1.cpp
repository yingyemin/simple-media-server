#include "HmacSHA1.h"
#include <openssl/hmac.h>
#include <openssl/sha.h>

std::string HmacSHA1::hmac_sha1(const std::string& data, const std::string& key) {
    unsigned char digest[SHA_DIGEST_LENGTH];
    unsigned int digest_len;
    HMAC(EVP_sha1(), key.c_str(), key.length(), reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), digest, &digest_len);
    return std::string(reinterpret_cast<char*>(digest), digest_len);
}

std::string HmacSHA1::hmac_sha1_hex(const std::string& data, const std::string& key) {
    unsigned char digest[SHA_DIGEST_LENGTH];
    unsigned int digest_len;
    HMAC(EVP_sha1(), key.c_str(), key.length(), reinterpret_cast<const unsigned char*>(data.c_str()), data.length(), digest, &digest_len);
    std::stringstream ss;
    for (int i = 0; i < digest_len; ++i) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
    }
    return ss.str();
}