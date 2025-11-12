#ifndef BatchOperationApi_h
#define BatchOperationApi_h

#include "Http/HttpParser.h"
#include "Common/UrlParser.h"
#include "Http/HttpResponse.h"

#include <string>
#include <vector>
#include <functional>

// using namespace std;

// 批量操作结果结构
struct BatchOperationResult {
    std::string id;              // 操作对象ID
    bool success;           // 操作是否成功
    std::string message;         // 操作结果消息
    int errorCode;          // 错误码（成功时为0）
};

// 批量操作请求结构
struct BatchOperationRequest {
    std::vector<std::string> targets; // 操作目标列表
    std::string operation;       // 操作类型
    std::string params;          // 操作参数（JSON格式）
};

class BatchOperationApi
{
public:
    static void initApi();
    
    // 批量流操作
    static void batchStreamOperation(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
    
    // 批量客户端操作
    static void batchClientOperation(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
    
    // 批量配置更新
    static void batchConfigUpdate(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);
    
    // 获取批量操作状态
    static void getBatchOperationStatus(const HttpParser& parser, const UrlParser& urlParser, 
                        const std::function<void(HttpResponse& rsp)>& rspFunc);

private:
    // 执行批量流操作
    static std::vector<BatchOperationResult> executeBatchStreamOperation(
        const std::vector<std::string>& streamPaths, const std::string& operation, const std::string& params);
    
    // 执行批量客户端操作
    static std::vector<BatchOperationResult> executeBatchClientOperation(
        const std::vector<std::string>& clientIds, const std::string& operation, const std::string& params);
    
    // 执行批量配置更新
    static std::vector<BatchOperationResult> executeBatchConfigUpdate(
        const std::vector<std::string>& targets, const std::string& configData);
    
    // 验证批量操作请求
    static bool validateBatchRequest(const BatchOperationRequest& request, std::string& errorMsg);
    
    // 解析批量操作请求
    static bool parseBatchRequest(const std::string& requestBody, BatchOperationRequest& request, std::string& errorMsg);
    
    // 格式化批量操作结果
    static std::string formatBatchResults(const std::vector<BatchOperationResult>& results);
};

#endif //BatchOperationApi_h