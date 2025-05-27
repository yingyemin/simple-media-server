#ifndef ErrorCodes_h
#define ErrorCodes_h

#include <string>
#include <unordered_map>

using namespace std;

// 统一错误码定义
enum class ApiErrorCode {
    // 成功
    SUCCESS = 200,

    // 客户端错误 (4xx)
    BAD_REQUEST = 400,           // 请求格式错误
    UNAUTHORIZED = 401,          // 未授权
    FORBIDDEN = 403,             // 禁止访问
    NOT_FOUND = 404,             // 资源不存在
    METHOD_NOT_ALLOWED = 405,    // 方法不允许
    CONFLICT = 409,              // 资源冲突
    UNPROCESSABLE_ENTITY = 422,  // 参数验证失败

    // 服务器错误 (5xx)
    INTERNAL_ERROR = 500,        // 内部服务器错误
    NOT_IMPLEMENTED = 501,       // 功能未实现
    SERVICE_UNAVAILABLE = 503,   // 服务不可用

    // 业务错误 (6xx)
    MISSING_REQUIRED_PARAMS = 600,  // 缺少必要参数
    INVALID_PARAM_VALUE = 601,      // 参数值无效
    INVALID_PARAM_FORMAT = 602,     // 参数格式无效
    PARAM_OUT_OF_RANGE = 603,       // 参数超出范围

    // 媒体流相关错误 (7xx)
    STREAM_NOT_FOUND = 700,         // 流不存在
    STREAM_ALREADY_EXISTS = 701,    // 流已存在
    STREAM_CREATE_FAILED = 702,     // 流创建失败
    STREAM_START_FAILED = 703,      // 流启动失败
    STREAM_STOP_FAILED = 704,       // 流停止失败
    CLIENT_NOT_FOUND = 705,         // 客户端不存在
    CLIENT_CONNECT_FAILED = 706,    // 客户端连接失败

    // 服务器相关错误 (8xx)
    SERVER_NOT_FOUND = 800,         // 服务器不存在
    SERVER_ALREADY_EXISTS = 801,    // 服务器已存在
    SERVER_START_FAILED = 802,      // 服务器启动失败
    SERVER_STOP_FAILED = 803,       // 服务器停止失败
    PORT_ALREADY_IN_USE = 804,      // 端口已被占用

    // 协议相关错误 (9xx)
    PROTOCOL_NOT_SUPPORTED = 900,   // 协议不支持
    PROTOCOL_VERSION_MISMATCH = 901, // 协议版本不匹配
    HANDSHAKE_FAILED = 902,         // 握手失败
    AUTHENTICATION_FAILED = 903,    // 认证失败
    TIMEOUT = 904,                  // 超时

    // 模板相关错误 (10xx)
    TEMPLATE_NOT_FOUND = 1000,      // 模板不存在
    TEMPLATE_ALREADY_EXISTS = 1001, // 模板已存在
    TEMPLATE_INVALID = 1002         // 模板无效
};

class ErrorCodeHelper {
public:
    // 获取错误码对应的HTTP状态码
    static int getHttpStatus(ApiErrorCode code);

    // 获取错误码对应的消息
    static string getErrorMessage(ApiErrorCode code);

    // 获取错误码的字符串表示
    static string getErrorCodeString(ApiErrorCode code);

    // 判断是否为成功状态
    static bool isSuccess(ApiErrorCode code);

    // 判断是否为客户端错误
    static bool isClientError(ApiErrorCode code);

    // 判断是否为服务器错误
    static bool isServerError(ApiErrorCode code);

private:
    static unordered_map<ApiErrorCode, string> errorMessages;
    static void initErrorMessages();
};

#endif //ErrorCodes_h
