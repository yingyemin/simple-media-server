#include "HookManager.h"

HookManager::Ptr HookManager::instance() 
{ 
    static HookManager::Ptr hook(new HookManager()); 
    return hook; 
}

void HookManager::addHook(const std::string& hookName, const HookBase::Ptr& hook)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _mapHook[hookName] = hook;
}

HookBase::Ptr HookManager::getHook(const std::string& hookName)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto iter = _mapHook.find(hookName);
    if (iter != _mapHook.end()) {
        return iter->second;
    }
    return nullptr;
}

void HookManager::setOnHookReportByHttp(const HookManager::hookReportFunc& func)
{
    _onHookReport = func;
}

void HookManager::reportByHttp(const std::string& url, const std::string&method, const std::string& msg, const std::function<void(const std::string& err, 
                const nlohmann::json& res)>& cb)
{
    if (_onHookReport) {
        _onHookReport(url, method, msg, cb);
    }
}