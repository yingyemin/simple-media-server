#include "DeviceManager.h"
#include "EventPoller/EventLoopPool.h"
#include "Log/Logger.h"

std::shared_ptr<DeviceManager> DeviceManager::instance()
{ 
    static std::shared_ptr<DeviceManager> manager(new DeviceManager()); 
    return manager; 
}

void DeviceManager::Init()
{
	// std::lock_guard<std::mutex> lk(_mutex);
	// auto&& devices = DbManager::GetInstance()->GetDeviceList();
	// for (auto&& dev : devices)
	// {
	// 	_devices[dev->GetDeviceID()] = dev;
	// 	auto&& channels = DbManager::GetInstance()->GetChannelList(dev->GetDeviceID());
	// 	for (auto&& ch : channels)
	// 	{
	// 		dev->InsertChannel(dev->GetDeviceID(), ch->GetChannelID(), ch);
	// 	}
	// }
}

void DeviceManager::AddDevice(Device::Ptr device)
{
	logDebug << "add device: " << device->GetDeviceID();
	std::lock_guard<std::mutex> lk(_mutex);
	_devices[device->GetDeviceID()] = device;
}

void DeviceManager::UpdateDevice(Device::Ptr device)
{
	logDebug << "update device: " << device->GetDeviceID();
	std::lock_guard<std::mutex> lk(_mutex);
	_devices[device->GetDeviceID()] = device;
}

Device::Ptr DeviceManager::GetDevice(const std::string& device_id)
{
	logDebug << "get device: " << device_id;
	std::lock_guard<std::mutex> lk(_mutex);
	auto iter = _devices.find(device_id);
	if (iter != _devices.end())
	{
		return iter->second;
	}
	return nullptr;
}

Device::Ptr DeviceManager::GetDevice(const std::string& ip, const std::string& port)
{
	std::lock_guard<std::mutex> lk(_mutex);
	for (auto&& dev : _devices)
	{
		if (dev.second->GetIP() == ip && dev.second->GetPort() == port)
		{
			return dev.second;
		}
	}
	return nullptr;
}

void DeviceManager::RemoveDevice(const std::string& device_id)
{
	std::lock_guard<std::mutex> lk(_mutex);
	_devices.erase(device_id);
}

std::vector<Device::Ptr> DeviceManager::GetDeviceList()
{
	std::lock_guard<std::mutex> lk(_mutex);
	std::vector<Device::Ptr> devices;
	for (auto&& dev : _devices)
	{
		devices.push_back(dev.second);
	}
	return devices;
}

int DeviceManager::GetDeviceCount()
{
	std::lock_guard<std::mutex> lk(_mutex);
	return (int)_devices.size();
}

void DeviceManager::UpdateDeviceStatus(const std::string& device_id, int status)
{
	std::lock_guard<std::mutex> lk(_mutex);
	auto iter = _devices.find(device_id);
	if (iter != _devices.end())
	{
		iter->second->SetStatus(status);
	}
}

void DeviceManager::UpdateDeviceLastTime(const std::string& device_id, time_t time)
{
	std::lock_guard<std::mutex> lk(_mutex);
	auto iter = _devices.find(device_id);
	if (iter != _devices.end())
	{
		iter->second->UpdateLastTime(time);
	}
}

void DeviceManager::UpdateDeviceChannelCount(const std::string& device_id, int count)
{
	std::lock_guard<std::mutex> lk(_mutex);
	auto iter = _devices.find(device_id);
	if (iter != _devices.end())
	{
		iter->second->SetChannelCount(count);
	}
}

void DeviceManager::Start()
{
	CheckDeviceStatus();
}

void DeviceManager::CheckDeviceStatus()
{
	std::weak_ptr<DeviceManager> wSelf = shared_from_this();
	auto loop = EventLoopPool::instance()->getLoopByCircle();
	loop->addTimerTask(5000, [wSelf](){
		auto self = wSelf.lock();
		if (!self) {
			return 0;
		}
		std::lock_guard<std::mutex> lk(self->_mutex);
		time_t now = time(nullptr);

		//TODO： 心跳超时判断
		for (auto&& dev : self->_devices)
		{
			if (now - dev.second->GetLastTime() > 60 * 3)
			{
				dev.second->SetStatus(0);
			}
		}
		return 5000;
	}, nullptr);
}