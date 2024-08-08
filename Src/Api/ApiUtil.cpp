#include "ApiUtil.h"

void checkArgs(const nlohmann::json &allArgs, const std::vector<std::string>& requiredParams)
{
    string lossParams = "";
    for (const auto& param : requiredParams) {
        if (allArgs.find(param) == allArgs.end() || allArgs[param].is_null() || allArgs[param].empty() || (allArgs[param].is_string() && allArgs.value(param, "") == "")) {
            lossParams += param + " ";
        }
    }
    if (!lossParams.empty()) {
        string errorMsg = "缺少必要参数:" + lossParams;
        throw ApiException(302, errorMsg);
        return ;
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