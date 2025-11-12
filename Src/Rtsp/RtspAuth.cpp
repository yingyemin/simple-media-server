#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "RtspAuth.h"
#include "Logger.h"
#include "Util/String.hpp"
#include "Util/MD5.h"
#include "Util/Base64.h"

using namespace std;


bool RtspAuth::rtspAuth(const string& nonce, RtspParser& parser)
{
    // logInfo << "rtspAuth start";
    for (auto header : parser._mapHeaders) {
        logInfo << "key: " << header.first << ", value: " << header.second;
    }
    string authorization = parser._mapHeaders["authorization"];
    if (authorization.empty()) {
        return false;
    }

    // logInfo << "rtspAuth authorization";
    //请求中包含认证信息
    auto pos = authorization.find_first_of(" ");
    if (pos == string::npos) {
        return false;
    }

    auto type = authorization.substr(0, pos);
    auto content = authorization.substr(pos + 1);
    // logInfo << "type: " << type << ", content: " << content;

    if(type == "Basic"){
        //base64认证，需要明文密码
        return authBasic("SimpleMediaServer", content);
    }else if(type == "Digest"){
        //md5认证
        return authDigest("SimpleMediaServer", content, nonce);
    }else{
        //其他认证方式？不支持！
        return false;
    }
}

bool RtspAuth::authDigest(const string &realm, const string &authStr, const string& nonce)
{
    logInfo << authStr;
    auto map = split(authStr, ",", "=", " \"");
    
    //check realm
    // logInfo << "realm: " << realm;
    // logInfo << "map[\"realm\"]: " << map["realm"];
    // logInfo << "nonce: " << nonce;
    // logInfo << "map[\"nonce\"]: " << map["nonce"];
    if(realm != map["realm"] || nonce != map["nonce"]){
        return false;
    }

    //check username and uri
    auto username = map["username"];
    auto uri = map["uri"];
    auto response = map["response"];
    if(username.empty() || uri.empty() || response.empty()){
        return false;
    }
    
    // TODO 获取实际密码
    string pwd = "123456";
    string encPwd = MD5(username+ ":" + realm + ":" + pwd).hexdigest();

    auto encRes = MD5( encPwd + ":" + nonce + ":" + MD5(string("DESCRIBE") + ":" + uri).hexdigest()).hexdigest();

    // logInfo << "encRes: " << encRes;
    // logInfo << "response: " << response;
    if(strcasecmp(encRes.data(), response.data()) != 0){
        return false;
    }

    return true;
}

bool RtspAuth::authBasic(const string &realm, const string &base64)
{
    //base64认证
    auto user_passwd = Base64::decode(base64);
    auto user_pwd_vec = split(user_passwd, ":");
    if (user_pwd_vec.size() < 2) {
        // 认证信息格式不合法，回复401 Unauthorized
        return false;
    }
    auto user = user_pwd_vec[0];
    auto pwd = user_pwd_vec[1];
    
    // TODO 和实际密码做比较
    return true;
}
