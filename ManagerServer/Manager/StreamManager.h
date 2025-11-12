#pragma once

#include <memory>
#include <string>
#include <vector>
#include <condition_variable>
#include <atomic>

#include "Common/json.hpp"

enum STREAM_TYPE {
	STREAM_TYPE_NONE = 0,
	STREAM_TYPE_GB = 1,         // GB28181方式推流
	STREAM_TYPE_PROXY = 2,      // 拉流代理
	STREAM_TYPE_PUSH = 3,       // 设备端主动推流
	STREAM_TYPE_MAX
};

class MediaStream
{
public:
	typedef std::shared_ptr<MediaStream> Ptr;
	MediaStream() = default;
	MediaStream(const std::string& app, const std::string& stream_id, STREAM_TYPE type);

	//MARK: 基类必须要有一个虚函数
	virtual ~MediaStream() {}

	std::string GetApp() const;
	std::string GetStreamID() const;
	STREAM_TYPE GetType();
	std::string GetStatus() const;
	void setStatus(const std::string& status);

	std::string getProtocol() const;
	void setProtocol(const std::string& protocol);

	std::string getServerId() {return _server_id;}
	void setServerId(const std::string& id) {_server_id = id;}

	nlohmann::json toJson();

private:
	std::string _protocol;
	std::string _app;
	std::string _stream_id;
	std::string _server_id;
	std::string _status;
	STREAM_TYPE _type = STREAM_TYPE::STREAM_TYPE_NONE;
};


class MediaStreamProxy :public MediaStream
{
public:
	MediaStreamProxy(const std::string& app, const std::string& stream_id)
		:MediaStream(app, stream_id, STREAM_TYPE::STREAM_TYPE_PROXY) {};
};

class MediaStreamPushed :public MediaStream
{
public:
	MediaStreamPushed(const std::string& app, const std::string& stream_id)
		:MediaStream(app, stream_id, STREAM_TYPE::STREAM_TYPE_PUSH) {};
};

// 播放的session
class CallSession :public MediaStream
{
public:
	typedef std::shared_ptr<CallSession> Ptr;
	CallSession(const std::string& app, const std::string& stream_id)
		:MediaStream(app, stream_id, STREAM_TYPE::STREAM_TYPE_GB)
	{}

	virtual ~CallSession() {}

	int GetCallID();
	void SetCallID(int id);
	int GetDialogID();
	int GetCSeqID();
	std::string getServerId();
	void setServerId(const std::string& id);
	void SetDialogID(int id);
	void SetConnected(bool flag);
	bool IsConnected();

	bool WaitForStreamReady(int seconds = 6);
	void NotifyStreamReady();

private:
	bool _is_connected = false;
	//其实没什么用
	int _call_id = 0;
	//会话ID，重要，每次的媒体控制需要通过此ID来找到对应的会话
	int _dialog_id = 0;
	std::atomic_uint32_t _ceq_id{0};
	std::string _server_id;

	std::mutex _mutex;
	std::condition_variable _cv;
};

class StreamManager
{
public:
	static std::shared_ptr<StreamManager> instance();

	void AddStream(MediaStream::Ptr stream);
	void RemoveStream(const std::string& id);

	MediaStream::Ptr GetStream(const std::string& id);
	MediaStream::Ptr GetStreamByCallID(int id);
	std::vector<MediaStream::Ptr> GetAllStream();
	std::vector<MediaStream::Ptr> GetStreamByType(STREAM_TYPE type);
	MediaStream::Ptr MakeStream(const std::string& stream_id, const std::string& app, STREAM_TYPE type);
	void ClearStreams();

private:
	StreamManager() = default;

private:
	std::mutex _mutex;
	std::map<std::string, MediaStream::Ptr> _streams;
};