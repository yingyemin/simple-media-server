#include "ApiResponse.h"

using namespace std;

void ApiResponse::success(const function<void(HttpResponse& rsp)>& rspFunc, const json& data)
{
    success(rspFunc, data, "操作成功");
}

void ApiResponse::success(const function<void(HttpResponse& rsp)>& rspFunc, 
                         const json& data, const string& message)
{
    json response = createBaseResponse(ApiErrorCode::SUCCESS, message);
    
    // 合并数据到响应中
    if (data.is_object()) {
        for (auto it = data.begin(); it != data.end(); ++it) {
            response[it.key()] = it.value();
        }
    } else if (!data.is_null()) {
        response["data"] = data;
    }
    
    HttpResponse rsp;
    setHttpResponse(rsp, response, ApiErrorCode::SUCCESS);
    rspFunc(rsp);
}

void ApiResponse::error(const function<void(HttpResponse& rsp)>& rspFunc, 
                       ApiErrorCode errorCode, const string& message)
{
    string errorMessage = message.empty() ? ErrorCodeHelper::getErrorMessage(errorCode) : message;
    json response = createBaseResponse(errorCode, errorMessage);
    
    HttpResponse rsp;
    setHttpResponse(rsp, response, errorCode);
    rspFunc(rsp);
}

void ApiResponse::error(const function<void(HttpResponse& rsp)>& rspFunc, 
                       const ApiException& ex)
{
    json response = createBaseResponse(ex.getErrorCode(), ex.getErrorMessage());
    
    HttpResponse rsp;
    setHttpResponse(rsp, response, ex.getErrorCode());
    rspFunc(rsp);
}

void ApiResponse::paginated(const function<void(HttpResponse& rsp)>& rspFunc,
                           const json& items, int total, int page, int pageSize)
{
    json response = createBaseResponse(ApiErrorCode::SUCCESS, "操作成功");
    
    response["items"] = items;
    response["total"] = total;
    response["page"] = page;
    response["pageSize"] = pageSize;
    response["totalPages"] = (total + pageSize - 1) / pageSize;
    
    HttpResponse rsp;
    setHttpResponse(rsp, response, ApiErrorCode::SUCCESS);
    rspFunc(rsp);
}

json ApiResponse::createBaseResponse(ApiErrorCode code, const string& message)
{
    json response;
    response["code"] = ErrorCodeHelper::getErrorCodeString(code);
    response["msg"] = message;
    response["timestamp"] = time(nullptr);
    return response;
}

void ApiResponse::setHttpResponse(HttpResponse& rsp, const json& responseData, ApiErrorCode code)
{
    rsp._status = ErrorCodeHelper::getHttpStatus(code);
    rsp.setContent(responseData.dump());
    rsp.setHeader("Content-Type", "application/json; charset=utf-8");
}
