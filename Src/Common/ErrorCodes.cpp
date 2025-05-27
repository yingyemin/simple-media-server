#include "ErrorCodes.h"
#include <unordered_map>

using namespace std;

unordered_map<ApiErrorCode, string> ErrorCodeHelper::errorMessages;

void ErrorCodeHelper::initErrorMessages() {
    if (!errorMessages.empty()) {
        return;
    }

    // 成功
    errorMessages[ApiErrorCode::SUCCESS] = "操作成功";

    // 客户端错误
    errorMessages[ApiErrorCode::BAD_REQUEST] = "请求格式错误";
    errorMessages[ApiErrorCode::UNAUTHORIZED] = "未授权访问";
    errorMessages[ApiErrorCode::FORBIDDEN] = "禁止访问";
    errorMessages[ApiErrorCode::NOT_FOUND] = "资源不存在";
    errorMessages[ApiErrorCode::METHOD_NOT_ALLOWED] = "请求方法不允许";
    errorMessages[ApiErrorCode::CONFLICT] = "资源冲突";
    errorMessages[ApiErrorCode::UNPROCESSABLE_ENTITY] = "参数验证失败";

    // 服务器错误
    errorMessages[ApiErrorCode::INTERNAL_ERROR] = "内部服务器错误";
    errorMessages[ApiErrorCode::NOT_IMPLEMENTED] = "功能未实现";
    errorMessages[ApiErrorCode::SERVICE_UNAVAILABLE] = "服务不可用";

    // 业务错误
    errorMessages[ApiErrorCode::MISSING_REQUIRED_PARAMS] = "缺少必要参数";
    errorMessages[ApiErrorCode::INVALID_PARAM_VALUE] = "参数值无效";
    errorMessages[ApiErrorCode::INVALID_PARAM_FORMAT] = "参数格式无效";
    errorMessages[ApiErrorCode::PARAM_OUT_OF_RANGE] = "参数超出范围";

    // 媒体流相关错误
    errorMessages[ApiErrorCode::STREAM_NOT_FOUND] = "流不存在";
    errorMessages[ApiErrorCode::STREAM_ALREADY_EXISTS] = "流已存在";
    errorMessages[ApiErrorCode::STREAM_CREATE_FAILED] = "流创建失败";
    errorMessages[ApiErrorCode::STREAM_START_FAILED] = "流启动失败";
    errorMessages[ApiErrorCode::STREAM_STOP_FAILED] = "流停止失败";
    errorMessages[ApiErrorCode::CLIENT_NOT_FOUND] = "客户端不存在";
    errorMessages[ApiErrorCode::CLIENT_CONNECT_FAILED] = "客户端连接失败";

    // 服务器相关错误
    errorMessages[ApiErrorCode::SERVER_NOT_FOUND] = "服务器不存在";
    errorMessages[ApiErrorCode::SERVER_ALREADY_EXISTS] = "服务器已存在";
    errorMessages[ApiErrorCode::SERVER_START_FAILED] = "服务器启动失败";
    errorMessages[ApiErrorCode::SERVER_STOP_FAILED] = "服务器停止失败";
    errorMessages[ApiErrorCode::PORT_ALREADY_IN_USE] = "端口已被占用";

    // 协议相关错误
    errorMessages[ApiErrorCode::PROTOCOL_NOT_SUPPORTED] = "协议不支持";
    errorMessages[ApiErrorCode::PROTOCOL_VERSION_MISMATCH] = "协议版本不匹配";
    errorMessages[ApiErrorCode::HANDSHAKE_FAILED] = "握手失败";
    errorMessages[ApiErrorCode::AUTHENTICATION_FAILED] = "认证失败";
    errorMessages[ApiErrorCode::TIMEOUT] = "操作超时";
}

int ErrorCodeHelper::getHttpStatus(ApiErrorCode code) {
    int codeValue = static_cast<int>(code);

    if (codeValue >= 200 && codeValue < 300) {
        return 200; // 成功
    } else if (codeValue >= 400 && codeValue < 500) {
        return codeValue; // 客户端错误，直接使用HTTP状态码
    } else if (codeValue >= 500 && codeValue < 600) {
        return codeValue; // 服务器错误，直接使用HTTP状态码
    } else {
        return 400; // 业务错误默认返回400
    }
}

string ErrorCodeHelper::getErrorMessage(ApiErrorCode code) {
    initErrorMessages();
    auto it = errorMessages.find(code);
    if (it != errorMessages.end()) {
        return it->second;
    }
    return "未知错误";
}

string ErrorCodeHelper::getErrorCodeString(ApiErrorCode code) {
    return to_string(static_cast<int>(code));
}

bool ErrorCodeHelper::isSuccess(ApiErrorCode code) {
    return code == ApiErrorCode::SUCCESS;
}

bool ErrorCodeHelper::isClientError(ApiErrorCode code) {
    int codeValue = static_cast<int>(code);
    return codeValue >= 400 && codeValue < 500;
}

bool ErrorCodeHelper::isServerError(ApiErrorCode code) {
    int codeValue = static_cast<int>(code);
    return codeValue >= 500 && codeValue < 600;
}