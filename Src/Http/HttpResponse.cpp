#include "HttpResponse.h"
#include "Logger.h"

using namespace std;

void HttpResponse::reSet()
{
    _status = 200;
    _body.clear();
    _redirectFlag =false;
    _redirectUrl.clear();
    _headers.clear();
}

void HttpResponse::setHeader(const std::string& key, const std::string& value)
{
    _headers.insert({key,value});
}

bool HttpResponse::hasHeader(const std::string& key)
{
    return _headers.find(key) != _headers.end();
}

std::string HttpResponse::getHeader(const std::string& key)
{
    if(hasHeader(key))
        return _headers.at(key);
    return "";
}

void HttpResponse::setContent(const std::string& body, const std::string& type)
{
    _body = body;
    setHeader("Content-Type",type);
}

void HttpResponse::setRedirect(const std::string& url, int status)
{
    _redirectFlag = true;
    _redirectUrl = url;
    _status=status;
}

bool HttpResponse::close() // 判断是否是长连接或短链接
{
    const std::string key = "Connection";
    if(hasHeader(key) && getHeader(key) == "keep-alive")
        return false;
    return true;
}