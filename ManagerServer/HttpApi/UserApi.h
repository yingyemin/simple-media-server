#ifndef USER_API_H_
#define USER_API_H_

#include "Http/HttpParser.h"
#include "Common/UrlParser.h"
#include "Http/HttpResponse.h"

class UserApi
{
public:
    static void initApi();

private:
    static void login(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
};

#endif //USER_API_H_