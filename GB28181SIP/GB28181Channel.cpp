#include "GB28181Channel.h"
#include "Utils.h"

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
	_name = ToMbcsString(name);
}

std::string Channel::GetName() const
{
	return _name;
}

void Channel::SetNickName(const std::string& name)
{
	_nickname = ToMbcsString(name);
}

std::string Channel::GetNickName() const
{
	return _nickname.empty() ? _name : _nickname;
}

void Channel::SetManufacturer(const std::string& manufacturer)
{
	_manufacturer = ToMbcsString(manufacturer);
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
	_address = ToMbcsString(address);
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
		{"name",ToUtf8String(_name)},
		{"nickname",ToUtf8String(_nickname.empty() ? _name : _nickname)},
		{"manufacturer",_manufacturer},
		{"model",_model},
		{"status",_status},
		{"ptz_type",_ptz_type},
		{"ssrc",_ssrc},
		{"stream_id",_stream_id},
		{"sub_channel_count",_sub_channels.size()}
	};
}