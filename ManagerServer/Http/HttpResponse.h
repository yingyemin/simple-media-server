#ifndef HttpResponse_h
#define HttpResponse_h

#include <string>
#include <unordered_map>
#include <memory>
#include <functional>

// using namespace std;

class HttpResponse
{
public:

    HttpResponse():_redirectFlag(false),_status(200) {}
    HttpResponse(int status):_redirectFlag(false),_status(status) {}
    void reSet();
    void setHeader(const std::string& key,const std::string& value);
    bool hasHeader(const std::string& key);
    std::string getHeader(const std::string& key);
    void setContent(const std::string& body,const std::string& type="text/html");
    void setRedirect(const std::string& url,int statu=302);
    bool close(); // 判断是否是长连接或短链接
public:
    int _status;    
    std::string _body;
    bool _redirectFlag;
    std::string _redirectUrl;
    std::unordered_map<std::string,std::string> _headers;
};

#endif //HttpResponse_h