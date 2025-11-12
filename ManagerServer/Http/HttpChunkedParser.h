#ifndef HttpChunkedParser_h
#define HttpChunkedParser_h

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <functional>

#include "Net/Buffer.h"
#include "Common/Config.h"

// using namespace std;

class HttpChunkedParser : public std::enable_shared_from_this<HttpChunkedParser> {
public:
    using Ptr = std::shared_ptr<HttpChunkedParser>;
    using Wptr = std::weak_ptr<HttpChunkedParser>;

    HttpChunkedParser();
    ~HttpChunkedParser();

public:
    void parse(const char *data, size_t len);
;
    void onHttpBody(const char* data, int len);
    void setOnHttpBody(const std::function<void(const char* data, int len)>& cb);

public:
    int _contentLen = 0;
    std::string _content;
    std::string _method;
    std::string _url;
    std::string _version;
    nlohmann::json _body;
    std::unordered_map<std::string, std::string> _mapHeaders;

private:
    int _stage = 1; //1:parse size; 2:parse body
    StringBuffer _remainData;
    std::function<void(const char* data, int len)> _onHttpBody;
};

#endif //HttpChunkedParser_h