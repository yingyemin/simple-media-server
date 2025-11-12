#ifndef HttpParser_h
#define HttpParser_h

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <functional>

#include "Net/Buffer.h"
#include "Common/Config.h"

// using namespace std;

class HttpParser : public std::enable_shared_from_this<HttpParser> {
public:
    using Ptr = std::shared_ptr<HttpParser>;
    using Wptr = std::weak_ptr<HttpParser>;

    HttpParser();
    ~HttpParser();

public:
    void parse(const char *data, size_t len);
    void onHttpRequest();
    void setOnHttpRequest(const std::function<void()>& cb);

    void onHttpBody(const char* data, int len);
    void setOnHttpBody(const std::function<void(const char* data, int len)>& cb);
    void clear();
    int getStage() {return _stage;}

public:
    int _contentLen = -1;
    std::string _content;
    std::string _method;
    std::string _url;
    std::string _version;
    nlohmann::json _body;
    std::unordered_map<std::string, std::string> _mapHeaders;

private:
    int _stage = 1; //1:handle request line, 2:handle header line, 3:handle content
    StringBuffer _remainData;
    std::function<void()> _onHttpRequest;
    std::function<void(const char* data, int len)> _onHttpBody;
};

#endif //HttpParser_h