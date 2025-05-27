#ifndef BatchOperationApi_h
#define BatchOperationApi_h

#include "Http/HttpParser.h"
#include "Common/UrlParser.h"
#include "Http/HttpResponse.h"

#include <string>
#include <vector>
#include <functional>

using namespace std;

// 批量操作结果结构
struct BatchOperationResult {
    string id;              // 操作对象ID
    bool success;           // 操作是否成功
    string message;         // 操作结果消息
    int errorCode;          // 错误码（成功时为0）
};

// 批量操作请求结构
struct BatchOperationRequest {
    vector<string> targets; // 操作目标列表
    string operation;       // 操作类型
    string params;          // 操作参数（JSON格式）
};

class BatchOperationApi
{
public:
    static void initApi();
    
    // 批量流操作
    static void batchStreamOperation(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
    
    // 批量客户端操作
    static void batchClientOperation(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
    
    // 批量配置更新
    static void batchConfigUpdate(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);
    
    // 获取批量操作状态
    static void getBatchOperationStatus(const HttpParser& parser, const UrlParser& urlParser, 
                        const function<void(HttpResponse& rsp)>& rspFunc);

private:
    // 执行批量流操作
    static vector<BatchOperationResult> executeBatchStreamOperation(
        const vector<string>& streamPaths, const string& operation, const string& params);
    
    // 执行批量客户端操作
    static vector<BatchOperationResult> executeBatchClientOperation(
        const vector<string>& clientIds, const string& operation, const string& params);
    
    // 执行批量配置更新
    static vector<BatchOperationResult> executeBatchConfigUpdate(
        const vector<string>& targets, const string& configData);
    
    // 验证批量操作请求
    static bool validateBatchRequest(const BatchOperationRequest& request, string& errorMsg);
    
    // 解析批量操作请求
    static bool parseBatchRequest(const string& requestBody, BatchOperationRequest& request, string& errorMsg);
    
    // 格式化批量操作结果
    static string formatBatchResults(const vector<BatchOperationResult>& results);
};

#endif //BatchOperationApi_h
