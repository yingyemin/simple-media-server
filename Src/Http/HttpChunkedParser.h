#ifndef HttpChunkedParser_h
#define HttpChunkedParser_h

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <functional>

#include "Net/Buffer.h"
#include "Common/Config.h"

using namespace std;

class HttpChunkedParser : public enable_shared_from_this<HttpChunkedParser> {
public:
    using Ptr = shared_ptr<HttpChunkedParser>;
    using Wptr = weak_ptr<HttpChunkedParser>;

    HttpChunkedParser();
    ~HttpChunkedParser();

public:
    void parse(const char *data, size_t len);
;
    void onHttpBody(const char* data, int len);
    void setOnHttpBody(const function<void(const char* data, int len)>& cb);

public:
    int _contentLen = 0;
    string _content;
    string _method;
    string _url;
    string _version;
    json _body;
    unordered_map<string, string> _mapHeaders;

private:
    int _stage = 1; //1:parse size; 2:parse body
    StringBuffer _remainData;
    function<void(const char* data, int len)> _onHttpBody;
};

#endif //HttpChunkedParser_h