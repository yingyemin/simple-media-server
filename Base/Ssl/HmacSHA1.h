#ifndef HMACSHA1_H_
#define HMACSHA1_H_

#include <iostream>
#include <iomanip>
#include <sstream>

using namespace std;

class HmacSHA1 {
public:
    HmacSHA1() {}
    ~HmacSHA1() {}
    static std::string hmac_sha1(const std::string& data, const std::string& key);
    static std::string hmac_sha1_hex(const std::string& data, const std::string& key);
};

#endif //HMACSHA1_H_