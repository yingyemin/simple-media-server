#ifndef HttpParser_h
#define HttpParser_h

#include <string>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <functional>

#include "Net/Buffer.h"
#include "Common/Config.h"

using namespace std;

class HttpParser : public enable_shared_from_this<HttpParser> {
public:
    using Ptr = shared_ptr<HttpParser>;
    using Wptr = weak_ptr<HttpParser>;

    HttpParser();
    ~HttpParser();

public:
    void parse(const char *data, size_t len);
    void onHttpRequest();
    void setOnHttpRequest(const function<void()>& cb);
;
    void onHttpBody(const char* data, int len);
    void setOnHttpBody(const function<void(const char* data, int len)>& cb);
    void clear();
    int getStage() {return _stage;}

public:
    int _contentLen = -1;
    string _content;
    string _method;
    string _url;
    string _version;
    json _body;
    unordered_map<string, string> _mapHeaders;

private:
    int _stage = 1; //1:handle request line, 2:handle header line, 3:handle content
    StringBuffer _remainData;
    function<void()> _onHttpRequest;
    function<void(const char* data, int len)> _onHttpBody;
};

#endif //HttpParser_h