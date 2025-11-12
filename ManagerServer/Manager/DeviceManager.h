#pragma once
#include "Device.h"
#include <memory>
#include <future>
// 信号类模板
template <typename... Args>
class Signal {
public:
    using Callback = std::function<void(Args...)>;

    // 连接回调函数
    void connect(const Callback& callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks_.push_back(callback);
    }

    // 异步发送信号
    void emitAsync(Args... args) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& callback : callbacks_) {
            auto task = [callback, args...]() {
                callback(args...);
                };
            auto future = std::async(std::launch::async, task);
        }
    }

private:
    std::vector<Callback> callbacks_;
    mutable std::mutex mutex_;
};

class DeviceManager : public std::enable_shared_from_this<DeviceManager>
{
public:
    using Callback = std::function<void(const std::string&, Device::Ptr)>;
	static std::shared_ptr<DeviceManager> instance();

	void Init();
	void AddDevice(Device::Ptr device);
	void UpdateDevice(Device::Ptr device);
	Device::Ptr GetDevice(const std::string& device_id);
	Device::Ptr GetDevice(const std::string& ip, const std::string& port);
	void RemoveDevice(const std::string& device_id);

	std::vector<Device::Ptr> GetDeviceList();
	int GetDeviceCount();
	void UpdateDeviceStatus(const std::string& device_id, int status);
	void UpdateDeviceLastTime(const std::string& device_id, time_t time = std::time(nullptr));
	void UpdateDeviceChannelCount(const std::string& device_id, int count);

	void Start();

    // 连接回调函数
    void connect(const Callback& callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks_.push_back(callback);
    }

    // 异步发送信号
    void emitAsync(const std::string& device_id, Device::Ptr ptr_device) {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& callback : callbacks_) {
            auto task = [callback, device_id,ptr_device]() {
                callback(device_id,ptr_device);
                };
            auto future = std::async(std::launch::async, task);
        }
    }

private:
	DeviceManager() = default;
	void CheckDeviceStatus();

private:

	std::unordered_map<std::string, Device::Ptr> _devices;
	// toolkit::Timer::Ptr _check_timer = nullptr;
	std::mutex _mutex;

private:
    std::vector<Callback> callbacks_;
    mutable std::mutex mutex_;
};

