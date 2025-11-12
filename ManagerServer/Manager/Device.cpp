#include "Device.h"

void Channel::InsertSubChannel(const std::string parent_id, const std::string& channel_id, Channel::Ptr channel)
{
	std::lock_guard<std::mutex> lk(_mutex);
	if (parent_id == _channel_id)
	{
		_sub_channels[channel_id] = channel;
	}
	else
	{
		for (auto&& ch : _sub_channels)
		{
			ch.second->InsertSubChannel(parent_id, channel_id, channel);
		}
	}
}

Channel::Ptr Channel::GetSubChannel(const std::string& channel_id)
{
	std::lock_guard<std::mutex> lk(_mutex);
	auto iter = _sub_channels.find(channel_id);
	if (iter != _sub_channels.end())
	{
		return iter->second;
	}
	return nullptr;
}

void Channel::DeleteSubChannel(const std::string& channel_id)
{
	std::lock_guard<std::mutex> lk(_mutex);
	auto iter = _sub_channels.find(channel_id);
	if (iter != _sub_channels.end())
	{
		_sub_channels.erase(iter);
	}
	else
	{
		for (auto&& ch : _sub_channels)
		{
			ch.second->DeleteSubChannel(channel_id);
		}
	}
}

std::vector<Channel::Ptr> Channel::GetAllSubChannels()
{
	std::lock_guard<std::mutex> lk(_mutex);
	std::vector<Channel::Ptr> channels;
	for (auto&& ch : _sub_channels)
	{
		channels.push_back(ch.second);
	}
	return channels;
}

int Channel::GetChannelCount()
{
	std::lock_guard<std::mutex> lk(_mutex);
	return (int)_sub_channels.size();
}


void Channel::SetParentID(const std::string& parent_id)
{
	_parent_id = parent_id;
}

std::string Channel::GetParentID() const
{
	return _parent_id;
}

void Channel::SetChannelID(const std::string& channel_id)
{
	_channel_id = channel_id;
}

std::string Channel::GetChannelID() const
{
	return _channel_id;
}

void Channel::SetName(const std::string& name)
{
	_name = name;
}

std::string Channel::GetName() const
{
	return _name;
}

void Channel::SetNickName(const std::string& name)
{
	_nickname = name;
}

std::string Channel::GetNickName() const
{
	return _nickname.empty() ? _name : _nickname;
}

void Channel::SetManufacturer(const std::string& manufacturer)
{
	_manufacturer = manufacturer;
}

std::string Channel::GetManufacturer() const
{
	return _manufacturer;
}

void Channel::SetModel(const std::string& model)
{
	_model = model;
}

std::string Channel::GetModel() const
{
	return _model;
}

void Channel::SetOwner(const std::string& owner)
{
	_owner = owner;
}

std::string Channel::GetOwner() const
{
	return _owner;
}

void Channel::SetCivilCode(const std::string& civil_code)
{
	_civil_code = civil_code;
}

std::string Channel::GetCivilCode() const
{
	return _civil_code;
}

void Channel::SetAddress(const std::string& address)
{
	_address = address;
}

std::string Channel::GetAddress() const
{
	return _address;
}

void Channel::SetStatus(const std::string& status)
{
	_status = status;
}

std::string Channel::GetStatus() const
{
	return _status;
}

void Channel::SetParental(const std::string& parental)
{
	_parental = parental;
}

std::string Channel::GetParental() const
{
	return _parental;
}

void Channel::SetRegisterWay(const std::string& register_way)
{
	_register_way = register_way;
}

std::string Channel::GetRegisterWay() const
{
	return _register_way;
}

void Channel::SetSecrety(const std::string& secrety)
{
	_secrety = secrety;
}

std::string Channel::GetSecrety() const
{
	return _secrety;
}

void Channel::SetStreamNum(const std::string& stream_num)
{
	_stream_num = stream_num;
}

std::string Channel::GetStreamNum() const
{
	return _stream_num;
}

void Channel::SetIpAddress(const std::string& ip)
{
	_ip = ip;
}

std::string Channel::GetIpAddress() const
{
	return _ip;
}

void Channel::SetPtzType(const std::string& ptz_type)
{
	_ptz_type = ptz_type;
}

std::string Channel::GetPtzType() const
{
	return _ptz_type;
}

void Channel::SetDownloadSpeed(const std::string& speed)
{
	_download_speed = speed;
}

std::string Channel::GetDownloadSpeed() const
{
	return _download_speed;
}

void Channel::SetDefaultSSRC(const std::string& id)
{
	_ssrc = id;
	_stream_id = _ssrc;
}

std::string Channel::GetDefaultSSRC() const
{
	return _ssrc;
}

std::string Channel::GetDefaultStreamID() const
{
	return _stream_id;
}

std::string Channel::toString()
{
	return toJson().dump(4);
}


nlohmann::json Channel::toJson()
{
	return nlohmann::json
	{
		{"id",_channel_id},
		{"name",_name},
		{"nickname",_nickname.empty() ? _name : _nickname},
		{"manufacturer",_manufacturer},
		{"model",_model},
		{"status",_status},
		{"ptz_type",_ptz_type},
		{"ssrc",_ssrc},
		{"stream_id",_stream_id},
		{"sub_channel_count",_sub_channels.size()}
	};
}


//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------------------


Device::Device(const std::string& device_id, const std::string& ip, const std::string& port)
{
	_device_id = device_id;
	_ip = ip;
	_port = port;
}

void Device::InsertChannel(const std::string& parent_id, const std::string& channel_id, Channel::Ptr channel)
{
	std::lock_guard<std::mutex> lk(_mutex);
	if (_device_id == parent_id)
	{
		_channels[channel_id] = channel;
		_channel_count++;
	}
	else
	{
		for (auto&& ch : _channels)
		{
			ch.second->InsertSubChannel(parent_id, channel_id, channel);
		}
	}
}

void Device::DeleteChannel(const std::string& channel_id)
{
	std::lock_guard<std::mutex> lk(_mutex);
	auto iter = _channels.find(channel_id);
	if (iter != _channels.end())
	{
		_channels.erase(iter);
		_channel_count--;
	}
	else
	{
		for (auto&& ch : _channels)
		{
			ch.second->DeleteSubChannel(channel_id);
		}
	}
}

Channel::Ptr Device::GetChannel(const std::string& channel_id)
{
	std::lock_guard<std::mutex> lk(_mutex);
	auto iter = _channels.find(channel_id);
	if (iter != _channels.end())
	{
		return iter->second;
	}
	return nullptr;
}

std::vector<Channel::Ptr> Device::GetAllChannels()
{
	std::lock_guard<std::mutex> lk(_mutex);
	std::vector<Channel::Ptr> channels;
	for (auto&& ch : _channels)
	{
		channels.push_back(ch.second);
	}
	return channels;
}

std::string Device::GetDeviceID() const
{
	return _device_id;
}

void Device::SetDeviceID(const std::string& id)
{
	_device_id = id;
}

std::string Device::GetName() const
{
	return _name;
}

void Device::SetName(const std::string& name)
{
	_name = name;
}

std::string Device::GetNickName() const
{
	return _nickname.empty() ? _name : _nickname;
}

void Device::SetNickName(const std::string& name)
{
	_nickname = name;
}

std::string Device::GetIP() const
{
	return _ip;
}

void Device::SetIP(const std::string& ip)
{
	_ip = ip;
}

std::string Device::GetPort() const
{
	return _port;
}

void Device::SetPort(const std::string& port)
{
	_port = port;
}

std::string Device::GetTransport() const
{
	return _transport;
}

void Device::SetTransport(const std::string& transport)
{
	_transport = transport;
}

void Device::SetManufacturer(const std::string& manufacturer)
{
	_manufacturer = manufacturer;
}

std::string Device::GetManufacturer() const
{
	return _manufacturer;
}

void Device::SetModel(const std::string& model)
{
	_model = model;
}

std::string Device::GetModel() const
{
	return _model;
}

int Device::GetStatus() const
{
	return _status;
}

void Device::SetStatus(int status)
{
	_status = status;
	if (status == 0)
	{
		_registered = false;
	}
}

time_t Device::GetRegistTime()
{
	return _regist_time;
}

void Device::UpdateRegistTime(time_t t)
{
	_regist_time = t;
}

time_t Device::GetLastTime()
{
	return _last_time;
}

void Device::UpdateLastTime(time_t t)
{
	_last_time = t;
}

int Device::GetChannelCount()
{
	return _channel_count;
}

void Device::SetChannelCount(int count)
{
	_channel_count = count;
}

std::string Device::GetParentID() const
{
	return _parent_id;
}

void Device::SetParentID(const std::string& parent_id)
{
	_parent_id = parent_id;
}

std::string Device::GetStreamIP() const
{
	return _stream_ip;
}

void Device::SetStreamIP(const std::string& stream_ip)
{
	_stream_ip = stream_ip;
}

std::string Device::GetSignalServerId() const
{
	return _signalServerId;
}

void Device::SetSignalServerId(const std::string& signal_server_id)
{
	_signalServerId = signal_server_id;
}

bool Device::IsRegistered()
{
	return _registered;
}

void Device::SetRegistered(bool flag)
{
	_registered = flag;
}


nlohmann::json Device::toJson()
{
	return nlohmann::json
	{
		{"id",_device_id},
		{"name",_name},
		{"nickname",_nickname.empty() ? _name : _nickname},
		{"ip",_ip},
		{"port",_port},
		{"channel_count",_channel_count},
		{"protocol",_transport},
		{"status",_status},
		{"last_time",_last_time},
		{"regist_time",_regist_time},
		{"manufacturer",_manufacturer},
		{"stream_ip",_stream_ip}
	};
}


std::string Device::toString()
{
	return toJson().dump(4);
}
