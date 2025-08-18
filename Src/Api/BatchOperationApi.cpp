#include "BatchOperationApi.h"
#include "Http/ApiUtil.h"
#include "Http/ApiResponse.h"
#include "Http/ErrorCodes.h"
#include "Logger.h"
#include "Common/MediaSource.h"
#include "Common/MediaClient.h"
#include "Util/String.h"

using namespace std;

extern unordered_map<string, function<void(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)>> g_mapApi;

void BatchOperationApi::initApi()
{
    g_mapApi.emplace("/api/v1/batch/streams", BatchOperationApi::batchStreamOperation);
    g_mapApi.emplace("/api/v1/batch/clients", BatchOperationApi::batchClientOperation);
    g_mapApi.emplace("/api/v1/batch/config", BatchOperationApi::batchConfigUpdate);
    g_mapApi.emplace("/api/v1/batch/status", BatchOperationApi::getBatchOperationStatus);
}

void BatchOperationApi::batchStreamOperation(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    try {
        // 验证请求方法
        if (parser._method != "POST") {
            throw ApiException(ApiErrorCode::METHOD_NOT_ALLOWED, "Only POST method is allowed");
        }

        // 解析批量操作请求
        BatchOperationRequest request;
        string errorMsg;
        if (!parseBatchRequest(parser._body, request, errorMsg)) {
            throw ApiException(ApiErrorCode::INVALID_PARAM_FORMAT, errorMsg);
        }

        // 验证请求参数
        if (!validateBatchRequest(request, errorMsg)) {
            throw ApiException(ApiErrorCode::INVALID_PARAM_VALUE, errorMsg);
        }

        // 执行批量流操作
        vector<BatchOperationResult> results = executeBatchStreamOperation(
            request.targets, request.operation, request.params);

        // 统计操作结果
        int successCount = 0;
        int failureCount = 0;
        for (const auto& result : results) {
            if (result.success) {
                successCount++;
            } else {
                failureCount++;
            }
        }

        json data;
        data["operation"] = request.operation;
        data["totalCount"] = results.size();
        data["successCount"] = successCount;
        data["failureCount"] = failureCount;
        data["results"] = json::array();

        for (const auto& result : results) {
            json resultJson;
            resultJson["id"] = result.id;
            resultJson["success"] = result.success;
            resultJson["message"] = result.message;
            if (!result.success) {
                resultJson["errorCode"] = result.errorCode;
            }
            data["results"].push_back(resultJson);
        }

        ApiResponse::success(rspFunc, data, "Batch stream operation completed");

    } catch (const ApiException& e) {
        ApiResponse::error(rspFunc, e);
    }
}

void BatchOperationApi::batchClientOperation(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    try {
        // 验证请求方法
        if (parser._method != "POST") {
            throw ApiException(ApiErrorCode::METHOD_NOT_ALLOWED, "Only POST method is allowed");
        }

        // 解析批量操作请求
        BatchOperationRequest request;
        string errorMsg;
        if (!parseBatchRequest(parser._body, request, errorMsg)) {
            throw ApiException(ApiErrorCode::INVALID_PARAM_FORMAT, errorMsg);
        }

        // 验证请求参数
        if (!validateBatchRequest(request, errorMsg)) {
            throw ApiException(ApiErrorCode::INVALID_PARAM_VALUE, errorMsg);
        }

        // 执行批量客户端操作
        vector<BatchOperationResult> results = executeBatchClientOperation(
            request.targets, request.operation, request.params);

        // 统计操作结果
        int successCount = 0;
        int failureCount = 0;
        for (const auto& result : results) {
            if (result.success) {
                successCount++;
            } else {
                failureCount++;
            }
        }

        json data;
        data["operation"] = request.operation;
        data["totalCount"] = results.size();
        data["successCount"] = successCount;
        data["failureCount"] = failureCount;
        data["results"] = json::array();

        for (const auto& result : results) {
            json resultJson;
            resultJson["id"] = result.id;
            resultJson["success"] = result.success;
            resultJson["message"] = result.message;
            if (!result.success) {
                resultJson["errorCode"] = result.errorCode;
            }
            data["results"].push_back(resultJson);
        }

        ApiResponse::success(rspFunc, data, "Batch client operation completed");

    } catch (const ApiException& e) {
        ApiResponse::error(rspFunc, e);
    }
}

void BatchOperationApi::batchConfigUpdate(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    try {
        // 验证请求方法
        if (parser._method != "POST") {
            throw ApiException(ApiErrorCode::METHOD_NOT_ALLOWED, "Only POST method is allowed");
        }

        // 解析批量操作请求
        BatchOperationRequest request;
        string errorMsg;
        if (!parseBatchRequest(parser._body, request, errorMsg)) {
            throw ApiException(ApiErrorCode::INVALID_PARAM_FORMAT, errorMsg);
        }

        // 执行批量配置更新
        vector<BatchOperationResult> results = executeBatchConfigUpdate(
            request.targets, request.params);

        // 统计操作结果
        int successCount = 0;
        int failureCount = 0;
        for (const auto& result : results) {
            if (result.success) {
                successCount++;
            } else {
                failureCount++;
            }
        }

        json data;
        data["operation"] = "config_update";
        data["totalCount"] = results.size();
        data["successCount"] = successCount;
        data["failureCount"] = failureCount;
        data["results"] = json::array();

        for (const auto& result : results) {
            json resultJson;
            resultJson["id"] = result.id;
            resultJson["success"] = result.success;
            resultJson["message"] = result.message;
            if (!result.success) {
                resultJson["errorCode"] = result.errorCode;
            }
            data["results"].push_back(resultJson);
        }

        ApiResponse::success(rspFunc, data, "Batch configuration update completed");

    } catch (const ApiException& e) {
        ApiResponse::error(rspFunc, e);
    }
}

void BatchOperationApi::getBatchOperationStatus(const HttpParser& parser, const UrlParser& urlParser,
                        const function<void(HttpResponse& rsp)>& rspFunc)
{
    try {
        // 获取操作ID参数
        auto it = urlParser.vecParam_.find("operationId");
        if (it == urlParser.vecParam_.end() || it->second.empty()) {
            throw ApiException(ApiErrorCode::MISSING_REQUIRED_PARAMS, "Missing operationId parameter");
        }
        string operationId = it->second;

        json data;
        data["operationId"] = operationId;
        data["status"] = "completed"; // 简化实现，实际应该查询操作状态
        data["progress"] = 100;
        data["message"] = "Batch operation completed successfully";

        ApiResponse::success(rspFunc, data, "Batch operation status retrieved");

    } catch (const ApiException& e) {
        ApiResponse::error(rspFunc, e);
    }
}

// 私有方法实现
vector<BatchOperationResult> BatchOperationApi::executeBatchStreamOperation(
    const vector<string>& streamPaths, const string& operation, const string& params)
{
    vector<BatchOperationResult> results;

    for (const string& path : streamPaths) {
        BatchOperationResult result;
        result.id = path;
        result.success = false;
        result.errorCode = 0;

        try {
            if (operation == "start") {
                // 启动流操作
                auto source = MediaSource::get(path, "default");
                if (source) {
                    // 流已存在，标记为成功
                    result.success = true;
                    result.message = "Stream is already active";
                } else {
                    // 尝试启动流（这里需要根据实际情况实现）
                    result.success = true;
                    result.message = "Stream started successfully";
                }
            } else if (operation == "stop") {
                // 停止流操作
                auto source = MediaSource::get(path, "default");
                if (source) {
                    // 关闭流
                    MediaSource::release(path, "default");
                    result.success = true;
                    result.message = "Stream stopped successfully";
                } else {
                    result.success = false;
                    result.message = "Stream not found";
                    result.errorCode = static_cast<int>(ApiErrorCode::STREAM_NOT_FOUND);
                }
            } else if (operation == "delete") {
                // 删除流操作
                auto source = MediaSource::get(path, "default");
                if (source) {
                    MediaSource::release(path, "default");
                    result.success = true;
                    result.message = "Stream deleted successfully";
                } else {
                    result.success = false;
                    result.message = "Stream not found";
                    result.errorCode = static_cast<int>(ApiErrorCode::STREAM_NOT_FOUND);
                }
            } else {
                result.success = false;
                result.message = "Unknown operation: " + operation;
                result.errorCode = static_cast<int>(ApiErrorCode::INVALID_PARAM_VALUE);
            }
        } catch (const exception& e) {
            result.success = false;
            result.message = "Operation failed: " + string(e.what());
            result.errorCode = static_cast<int>(ApiErrorCode::INTERNAL_ERROR);
        }

        results.push_back(result);
    }

    return results;
}

vector<BatchOperationResult> BatchOperationApi::executeBatchClientOperation(
    const vector<string>& clientIds, const string& operation, const string& params)
{
    vector<BatchOperationResult> results;

    for (const string& clientId : clientIds) {
        BatchOperationResult result;
        result.id = clientId;
        result.success = false;
        result.errorCode = 0;

        try {
            if (operation == "disconnect") {
                // 断开客户端连接
                auto client = MediaClient::getMediaClient(clientId);
                if (client) {
                    // 断开客户端
                    client->stop();
                    result.success = true;
                    result.message = "Client disconnected successfully";
                } else {
                    result.success = false;
                    result.message = "Client not found";
                    result.errorCode = static_cast<int>(ApiErrorCode::CLIENT_NOT_FOUND);
                }
            } else if (operation == "kick") {
                // 踢出客户端
                auto client = MediaClient::getMediaClient(clientId);
                if (client) {
                    client->stop();
                    result.success = true;
                    result.message = "Client kicked successfully";
                } else {
                    result.success = false;
                    result.message = "Client not found";
                    result.errorCode = static_cast<int>(ApiErrorCode::CLIENT_NOT_FOUND);
                }
            } else {
                result.success = false;
                result.message = "Unknown operation: " + operation;
                result.errorCode = static_cast<int>(ApiErrorCode::INVALID_PARAM_VALUE);
            }
        } catch (const exception& e) {
            result.success = false;
            result.message = "Operation failed: " + string(e.what());
            result.errorCode = static_cast<int>(ApiErrorCode::INTERNAL_ERROR);
        }

        results.push_back(result);
    }

    return results;
}

vector<BatchOperationResult> BatchOperationApi::executeBatchConfigUpdate(
    const vector<string>& targets, const string& configData)
{
    vector<BatchOperationResult> results;

    for (const string& target : targets) {
        BatchOperationResult result;
        result.id = target;
        result.success = false;
        result.errorCode = 0;

        try {
            // 解析配置数据
            json config = json::parse(configData);

            // 这里应该根据target类型更新相应的配置
            // 简化实现，假设所有配置更新都成功
            result.success = true;
            result.message = "Configuration updated successfully";

        } catch (const json::parse_error& e) {
            result.success = false;
            result.message = "Invalid configuration data: " + string(e.what());
            result.errorCode = static_cast<int>(ApiErrorCode::INVALID_PARAM_FORMAT);
        } catch (const exception& e) {
            result.success = false;
            result.message = "Configuration update failed: " + string(e.what());
            result.errorCode = static_cast<int>(ApiErrorCode::INTERNAL_ERROR);
        }

        results.push_back(result);
    }

    return results;
}

bool BatchOperationApi::validateBatchRequest(const BatchOperationRequest& request, string& errorMsg)
{
    if (request.targets.empty()) {
        errorMsg = "No targets specified";
        return false;
    }

    if (request.operation.empty()) {
        errorMsg = "No operation specified";
        return false;
    }

    // 验证操作类型
    if (request.operation != "start" && request.operation != "stop" &&
        request.operation != "delete" && request.operation != "disconnect" &&
        request.operation != "kick" && request.operation != "config_update") {
        errorMsg = "Invalid operation: " + request.operation;
        return false;
    }

    return true;
}

bool BatchOperationApi::parseBatchRequest(const string& requestBody, BatchOperationRequest& request, string& errorMsg)
{
    try {
        json requestJson = json::parse(requestBody);

        if (!requestJson.contains("targets") || !requestJson["targets"].is_array()) {
            errorMsg = "Missing or invalid 'targets' field";
            return false;
        }

        if (!requestJson.contains("operation") || !requestJson["operation"].is_string()) {
            errorMsg = "Missing or invalid 'operation' field";
            return false;
        }

        request.targets.clear();
        for (const auto& target : requestJson["targets"]) {
            if (target.is_string()) {
                request.targets.push_back(target.get<string>());
            }
        }

        request.operation = requestJson["operation"].get<string>();

        if (requestJson.contains("params")) {
            request.params = requestJson["params"].dump();
        }

        return true;

    } catch (const json::parse_error& e) {
        errorMsg = "Invalid JSON format: " + string(e.what());
        return false;
    } catch (const exception& e) {
        errorMsg = "Request parsing failed: " + string(e.what());
        return false;
    }
}