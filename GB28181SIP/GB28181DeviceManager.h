#ifndef GB28181DeviceManager_H
#define GB28181DeviceManager_H

#include <memory>
#include <string>
#include <mutex>
#include <unordered_map>

#include "GB28181SIPContext.h"

class GB28181DeviceManager
{
public:
	using Ptr = std::shared_ptr<GB28181DeviceManager>;
	GB28181DeviceManager() = default;
		
	static GB28181DeviceManager::Ptr instance();

public:
	void addDevice(const GB28181SIPContext::Ptr& device);
	GB28181SIPContext::Ptr getDevice(const std::string& deviceId);
	bool removeDevice(const std::string& deviceId);
	std::unordered_map<std::string, GB28181SIPContext::Wptr> getMapContext();

private:
	std::unordered_map<std::string, GB28181SIPContext::Wptr> _devices;
	std::mutex _mutex;
};

#endif //GB28181DeviceManager_H