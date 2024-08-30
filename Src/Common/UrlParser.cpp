#include <cstdlib>
#include <string>
#include <algorithm>
#include <cctype>

#include "UrlParser.h"
#include "Logger.h"
#include "Util/String.h"
#include "Common/Define.h"

using namespace std;

static char HexToI(char c){
    if(isdigit(c))
        return c - '0';
    else if (islower(c))
        return c - 'a' + 10;
    else if (isupper(c))
        return c - 'A' + 10;
    return -1;
}


void UrlParser::parse(string url)
{
    // http://admin:123456@127.0.0.1/api/v1?test=1&rr=2
    vhost_ = DEFAULT_VHOST;
    type_ = DEFAULT_TYPE;
    url_ = url;
    auto pos = url.find("://");
    if (pos == string::npos) {
        // throw parse failed
        return ;
    }
    // http
    protocol_ = url.substr(0, pos);
    logInfo << "protocol_ " << protocol_;

    auto nextPos = url.find_first_of("?");
    if (nextPos != string::npos) {
        // test=1&rr=2
        param_ = url.substr(nextPos + 1);
        // admin:123456@127.0.0.1/api/v1
        url = url.substr(pos + 3, nextPos - pos - 3);
        vecParam_ = split(param_, "&", "=");
        logInfo << param_  << " " << url;
    } else {
        // admin:123456@127.0.0.1/api/v1
        url = url.substr(pos + 3);
    }

    pos = url.find_first_of("/");
    // admin:123456@127.0.0.1
    string host = url.substr(0, pos);
    if (host.find("@") != string::npos) {
        // 127.0.0.1
        host = findSubStr(host, "@", "");
    }
    logInfo << host;
    if (host[0] == '[') {
        host_ = findSubStr(host, "[", "]");
        string port = findSubStr(host, "]:", "");
        port_ = (port == "" ? 0 : stoi(port));
    } else {
        host_ = findSubStr(host, "", ":");
        string port = findSubStr(host, ":", "");
        port_ = (port == "" ? 0 : stoi(port));
    }

    path_ = url.substr(pos);
    for (auto& param : vecParam_) {
        if (param.first == "vhost" && !param.second.empty()) {
            vhost_ = param.second;
        } else if (param.first == "type" && !param.second.empty()) {
            type_ = param.second;
        }
    }
    logInfo << "vhost_ " << vhost_;
    logInfo << "type_ " << type_;
}

std::string UrlParser::urlEncode(const std::string& url, bool convert_space_to_plus) // encode 避免ERL中资源路径与查询字符串中的特殊字符与HTTP请求中的特殊字符产生歧义
// 编码格式：；将特殊字符的ascii值，转换为两个16禁止的字符 前缀% c++ -> c%20%20
// 不编码的特殊字符：RFC3986 .-_~ 字母数字属于绝对不编码字符
// W3C文档中固定，查询字符串' ' -> + 解码就是 + -> ' '
{
    std::string res;
    for(const auto &ch : url){
        if(ch == '.' || ch == '-' || ch == '_' || ch == '~' || static_cast<bool>(isalnum(ch))){
            res += ch;
            continue;
        }else if(ch == ' ' && convert_space_to_plus){
            res += "+";
            continue;
        }
        // 转换为 %HH操作
        char tmp[4] = {0};
        snprintf(tmp,4,"%%%02X",ch);
        res += tmp;
    }
    return res;
}

std::string UrlParser::urlDecode(const std::string& url,bool convert_space_to_plus) // decode
{
    // 遇到% 将后后面的2个字符转换为数字 nums[0] << 4 + nums[1]  
    std::string res;
    for(int i = 0; i < url.size();i++){
        if(url[i] =='+' && convert_space_to_plus){
            res += ' ';
            continue;
        }
        if(url[i] == '%' && i + 2 < url.size()){
            char v1 = HexToI(url[i+1]);
            char v2 = HexToI(url[i+2]);
            char v = (v1 << 4) + v2;
            res += v;
            i += 2;
        }else{
            res += url[i];
        }
    }
    return res;
}


