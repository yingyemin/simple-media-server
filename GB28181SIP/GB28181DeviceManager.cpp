#include "GB28181DeviceManager.h"

GB28181DeviceManager::Ptr GB28181DeviceManager::instance()
{
    static GB28181DeviceManager::Ptr instance = std::make_shared<GB28181DeviceManager>();
    return instance;
}

void GB28181DeviceManager::addDevice(const GB28181SIPContext::Ptr& device)
{
    std::lock_guard<std::mutex> lock(_mutex);
    _devices[device->getDeviceInfo()->_device_id] = device;
}

GB28181SIPContext::Ptr GB28181DeviceManager::getDevice(const std::string& deviceId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _devices.find(deviceId);
    if (it == _devices.end())
    {
        return nullptr;
    }
    return it->second.lock();
}

bool GB28181DeviceManager::removeDevice(const std::string& deviceId)
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _devices.find(deviceId);
    if (it == _devices.end())
    {
        return false;
    }
    _devices.erase(it);
    return true;
}

std::unordered_map<std::string, GB28181SIPContext::Wptr> GB28181DeviceManager::getMapContext()
{
    std::lock_guard<std::mutex> lock(_mutex);
    return _devices;
}
