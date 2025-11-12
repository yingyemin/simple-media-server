#ifndef GB28181Channel_H
#define GB28181Channel_H

#include <memory>
#include <string>
#include <vector>
#include <mutex>
#include <unordered_map>

#include "Common/json.hpp"

class Channel
{
public:
	typedef std::shared_ptr<Channel> Ptr;
	
	Channel() = default;
	void InsertSubChannel(const std::string parent_id, const std::string& channel_id, Channel::Ptr channel);
	Channel::Ptr GetSubChannel(const std::string& channel_id);
	void DeleteSubChannel(const std::string& channel_id);
	std::vector<Channel::Ptr> GetAllSubChannels();

	int GetChannelCount();

	void SetParentID(const std::string& parent_id);
	std::string GetParentID() const;

	void SetChannelID(const std::string& channel_id);
	std::string GetChannelID() const;

	void SetName(const std::string& name);
	std::string GetName() const;

	void SetNickName(const std::string& name);
	std::string GetNickName() const;

	void SetManufacturer(const std::string& manufacturer);
	std::string GetManufacturer() const;

	void SetModel(const std::string& model);
	std::string GetModel() const;

	void SetOwner(const std::string& owner);
	std::string GetOwner() const;

	void SetCivilCode(const std::string& civil_code);
	std::string GetCivilCode() const;

	void SetAddress(const std::string& address);
	std::string GetAddress() const;

	void SetStatus(const std::string& status);
	std::string GetStatus() const;

	void SetParental(const std::string& parental);
	std::string GetParental() const;

	void SetRegisterWay(const std::string& register_way);
	std::string GetRegisterWay() const;

	void SetSecrety(const std::string& secrety);
	std::string GetSecrety() const;

	void SetStreamNum(const std::string& stream_num);
	std::string GetStreamNum() const;

	void SetIpAddress(const std::string& ip);
	std::string GetIpAddress() const;

	void SetPtzType(const std::string& ptz_type);
	std::string GetPtzType() const;

	void SetDownloadSpeed(const std::string& speed);
	std::string GetDownloadSpeed() const;

	void SetDefaultSSRC(const std::string& id);
	std::string GetDefaultSSRC() const;
	std::string GetDefaultStreamID() const;

	std::string toString();
	nlohmann::json toJson();

private:
	std::string _channel_id;
	std::string _parent_id;
	std::string _name;
	std::string _manufacturer;
	std::string _model;
	std::string _owner;
	std::string _civil_code;
	std::string _address;
	std::string _parental;
	std::string _register_way;
	std::string _secrety;
	std::string _stream_num;
	std::string _ip;
	std::string _status;
	std::string _nickname;

	std::string _ssrc;
	std::string _stream_id;

	std::string _ptz_type;
	std::string _download_speed;

	std::unordered_map<std::string, Channel::Ptr> _sub_channels;

private:
	std::mutex _mutex;
};

#endif