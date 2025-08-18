#ifndef ApiUtil_h
#define ApiUtil_h

#include <exception>
#include <string>
#include <regex>
#include <climits>

#include "Common/json.hpp"
#include "ErrorCodes.h"

using namespace std;

void checkArgs(const nlohmann::json &allArgs, const std::vector<std::string>& requiredParams);

// 参数验证规则
struct ParamRule {
    string name;
    bool required = true;
    string type = "string";  // string, int, float, bool, array, object
    int minLength = -1;      // 字符串最小长度
    int maxLength = -1;      // 字符串最大长度
    int minValue = INT_MIN;  // 数值最小值
    int maxValue = INT_MAX;  // 数值最大值
    string pattern = "";     // 正则表达式模式
    vector<string> allowedValues; // 允许的值列表
};

// 验证参数
void validateParams(const nlohmann::json &params, const vector<ParamRule>& rules);

// 验证单个参数
void validateParam(const nlohmann::json &value, const ParamRule& rule);

class ApiException : public exception
{
public:
    ApiException(ApiErrorCode errorCode, const string& errorMsg = "")
    :_errorCode(errorCode)
    ,_errorMsg(errorMsg.empty() ? ErrorCodeHelper::getErrorMessage(errorCode) : errorMsg)
    {}

    // 兼容旧版本构造函数
    ApiException(int errorCode, const string& errorMsg)
    :_errorCode(static_cast<ApiErrorCode>(errorCode))
    ,_errorMsg(errorMsg)
    {}

    const char* what() const noexcept override {
        return _errorMsg.c_str();
    }

    ApiErrorCode getErrorCode() const { return _errorCode; }
    string getErrorMessage() const { return _errorMsg; }
    int getHttpStatus() const { return ErrorCodeHelper::getHttpStatus(_errorCode); }

private:
    ApiErrorCode _errorCode = ApiErrorCode::SUCCESS;
    string _errorMsg;
};

class asyncResponseFunc
{
public:
    asyncResponseFunc() = default;
    ~asyncResponseFunc()
    {
        if (_callback) {
            _callback(status, _response);
        }
    }

    void setStatus(int status) { this->status = status; }
    void setCallback(const std::function<void(int, const nlohmann::json&)>& callback) { _callback = callback; }
    void setResponse(const nlohmann::json& response) { _response = response; }
    nlohmann::json getResponse() const { return _response; }

private:
    int status = 200;
    nlohmann::json _response;
    std::function<void(int, const nlohmann::json&)> _callback;
};

int toInt(const nlohmann::json& j);

float toFloat(const nlohmann::json& j);

int getInt(const nlohmann::json& j, const string& key, int defaultValue = 0);
float getFloat(const nlohmann::json& j, const string& key, float defaultValue = 0);

#endif //ApiUtil_h