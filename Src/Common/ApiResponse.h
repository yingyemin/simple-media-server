#ifndef ApiResponse_h
#define ApiResponse_h

#include <string>
#include <functional>
#include "Common/json.hpp"
#include "Common/ErrorCodes.h"
#include "Common/ApiUtil.h"
#include "Http/HttpParser.h"
#include "Http/HttpResponse.h"

using namespace std;
using json = nlohmann::json;

class ApiResponse {
public:
    // 成功响应
    static void success(const function<void(HttpResponse& rsp)>& rspFunc,
                       const json& data = json::object());

    // 成功响应（带自定义消息）
    static void success(const function<void(HttpResponse& rsp)>& rspFunc,
                       const json& data, const string& message);

    // 错误响应
    static void error(const function<void(HttpResponse& rsp)>& rspFunc,
                     ApiErrorCode errorCode, const string& message = "");

    // 错误响应（从异常）
    static void error(const function<void(HttpResponse& rsp)>& rspFunc,
                     const ApiException& ex);

    // 分页响应
    static void paginated(const function<void(HttpResponse& rsp)>& rspFunc,
                         const json& items, int total, int page = 1, int pageSize = 20);

private:
    // 创建基础响应结构
    static json createBaseResponse(ApiErrorCode code, const string& message);

    // 设置HTTP响应
    static void setHttpResponse(HttpResponse& rsp, const json& responseData, ApiErrorCode code);
};

#endif //ApiResponse_h
