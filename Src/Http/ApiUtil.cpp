#include "ApiUtil.h"

using namespace std;

void checkArgs(const nlohmann::json &allArgs, const std::vector<std::string>& requiredParams)
{
    string lossParams = "";
    for (const auto& param : requiredParams) {
        if (allArgs.find(param) == allArgs.end() || allArgs[param].is_null() || allArgs[param].empty() || (allArgs[param].is_string() && allArgs.value(param, "") == "")) {
            lossParams += param + " ";
        }
    }
    if (!lossParams.empty()) {
        string errorMsg = "缺少必要参数: " + lossParams;
        throw ApiException(ApiErrorCode::MISSING_REQUIRED_PARAMS, errorMsg);
        return ;
    }
}

void validateParams(const nlohmann::json &params, const vector<ParamRule>& rules)
{
    for (const auto& rule : rules) {
        if (rule.required) {
            if (params.find(rule.name) == params.end() || params[rule.name].is_null()) {
                throw ApiException(ApiErrorCode::MISSING_REQUIRED_PARAMS, "缺少必要参数: " + rule.name);
            }
        }

        if (params.find(rule.name) != params.end() && !params[rule.name].is_null()) {
            validateParam(params[rule.name], rule);
        }
    }
}

void validateParam(const nlohmann::json &value, const ParamRule& rule)
{
    // 类型验证
    if (rule.type == "string" && !value.is_string()) {
        throw ApiException(ApiErrorCode::INVALID_PARAM_FORMAT, "参数 " + rule.name + " 必须是字符串类型");
    } else if (rule.type == "int" && !value.is_number_integer()) {
        throw ApiException(ApiErrorCode::INVALID_PARAM_FORMAT, "参数 " + rule.name + " 必须是整数类型");
    } else if (rule.type == "float" && !value.is_number()) {
        throw ApiException(ApiErrorCode::INVALID_PARAM_FORMAT, "参数 " + rule.name + " 必须是数值类型");
    } else if (rule.type == "bool" && !value.is_boolean()) {
        throw ApiException(ApiErrorCode::INVALID_PARAM_FORMAT, "参数 " + rule.name + " 必须是布尔类型");
    } else if (rule.type == "array" && !value.is_array()) {
        throw ApiException(ApiErrorCode::INVALID_PARAM_FORMAT, "参数 " + rule.name + " 必须是数组类型");
    } else if (rule.type == "object" && !value.is_object()) {
        throw ApiException(ApiErrorCode::INVALID_PARAM_FORMAT, "参数 " + rule.name + " 必须是对象类型");
    }

    // 字符串长度验证
    if (value.is_string()) {
        string strValue = value.get<string>();
        if (rule.minLength >= 0 && strValue.length() < rule.minLength) {
            throw ApiException(ApiErrorCode::PARAM_OUT_OF_RANGE,
                "参数 " + rule.name + " 长度不能少于 " + to_string(rule.minLength) + " 个字符");
        }
        if (rule.maxLength >= 0 && strValue.length() > rule.maxLength) {
            throw ApiException(ApiErrorCode::PARAM_OUT_OF_RANGE,
                "参数 " + rule.name + " 长度不能超过 " + to_string(rule.maxLength) + " 个字符");
        }

        // 正则表达式验证
        if (!rule.pattern.empty()) {
            regex pattern(rule.pattern);
            if (!regex_match(strValue, pattern)) {
                throw ApiException(ApiErrorCode::INVALID_PARAM_FORMAT,
                    "参数 " + rule.name + " 格式不正确");
            }
        }

        // 允许值列表验证
        if (!rule.allowedValues.empty()) {
            bool found = false;
            for (const auto& allowedValue : rule.allowedValues) {
                if (strValue == allowedValue) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                throw ApiException(ApiErrorCode::INVALID_PARAM_VALUE,
                    "参数 " + rule.name + " 的值不在允许范围内");
            }
        }
    }

    // 数值范围验证
    if (value.is_number()) {
        int intValue = value.get<int>();
        if (intValue < rule.minValue) {
            throw ApiException(ApiErrorCode::PARAM_OUT_OF_RANGE,
                "参数 " + rule.name + " 不能小于 " + to_string(rule.minValue));
        }
        if (intValue > rule.maxValue) {
            throw ApiException(ApiErrorCode::PARAM_OUT_OF_RANGE,
                "参数 " + rule.name + " 不能大于 " + to_string(rule.maxValue));
        }
    }
}

int toInt(const nlohmann::json& j)
{
    if (j.is_string()) {
        return stoi(j.get<string>());
    }
    return j;
}

float toFloat(const nlohmann::json& j)
{
    if (j.is_string()) {
        return stof(j.get<string>());
    }
    return j;
}

int getInt(const nlohmann::json& j, const string& key, int defaultValue)
{
    if (j.find(key) == j.end()) {
        return defaultValue;
    }

    auto v = j[key];
    if (v.is_string()) {
        return stoi(v.get<string>());
    }
    return v;
}

float getFloat(const nlohmann::json& j, const string& key, float defaultValue)
{
    if (j.find(key) == j.end()) {
        return defaultValue;
    }

    auto v = j[key];
    if (v.is_string()) {
        return stof(v.get<string>());
    }
    return v;
}