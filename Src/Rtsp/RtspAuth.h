#ifndef RtspAuth_H
#define RtspAuth_H

#include <string>
#include <memory>

#include "RtspParser.h"

using namespace std;

class RtspAuth
{
public:
    static bool rtspAuth(const string& nonce, RtspParser& parser);
    static bool authDigest(const string &realm, const string &authStr, const string& nonce);
    static bool authBasic(const string &realm, const string &base64);
};


#endif //RtspAuth_H
