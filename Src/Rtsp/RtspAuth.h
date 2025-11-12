#ifndef RtspAuth_H
#define RtspAuth_H

#include <string>
#include <memory>

#include "RtspParser.h"

// using namespace std;

class RtspAuth
{
public:
    static bool rtspAuth(const std::string& nonce, RtspParser& parser);
    static bool authDigest(const std::string &realm, const std::string &authStr, const std::string& nonce);
    static bool authBasic(const std::string &realm, const std::string &base64);
};


#endif //RtspAuth_H
