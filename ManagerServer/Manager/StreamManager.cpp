#include "StreamManager.h"


MediaStream::MediaStream(const std::string& app, const std::string& stream_id, STREAM_TYPE type)
{
	_app = app;
	_stream_id = stream_id;
	_type = type;
}

std::string MediaStream::GetApp() const
{
	return _app;
}

std::string MediaStream::GetStreamID() const
{
	return _stream_id;
}

std::string MediaStream::GetStatus() const
{
	return _status;
}

void MediaStream::setStatus(const std::string& status)
{
	_status = status;
}

std::string MediaStream::getProtocol() const
{
	return _protocol;
}

void MediaStream::setProtocol(const std::string& protocol)
{
	_protocol = protocol;
}

STREAM_TYPE MediaStream::GetType()
{
	return _type;
}

nlohmann::json MediaStream::toJson()
{
	return nlohmann::json{
		{"app",_app},
		{"stream_id",_stream_id},
		{"type",_type},
	};
}

int CallSession::GetCallID()
{
	return _call_id;
}
void CallSession::SetCallID(int id)
{
	_call_id = id;
}

std::string CallSession::getServerId()
{
	return _server_id;
}

void CallSession::setServerId(const std::string& id)
{
	_server_id = id;
}

int CallSession::GetDialogID()
{
	return _dialog_id;
}
int CallSession::GetCSeqID()
{
	return _ceq_id++;
}
void CallSession::SetDialogID(int id)
{
	_dialog_id = id;
}
void CallSession::SetConnected(bool flag)
{
	_is_connected = flag;
}
bool CallSession::IsConnected()
{
	return _is_connected;
}


bool CallSession::WaitForStreamReady(int seconds)
{
	std::unique_lock<std::mutex> lk(_mutex);
	auto status = _cv.wait_for(lk, std::chrono::seconds(seconds));
	if (status == std::cv_status::timeout)
	{
		return false;
	}
	return true;
}

void CallSession::NotifyStreamReady()
{
	_cv.notify_all();
}

std::shared_ptr<StreamManager> StreamManager::instance()
{ 
    static std::shared_ptr<StreamManager> manager(new StreamManager()); 
    return manager; 
}

void StreamManager::AddStream(MediaStream::Ptr stream)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_streams[stream->GetStreamID()] = stream;
}

void  StreamManager::RemoveStream(const std::string& id)
{
	std::lock_guard<std::mutex> lock(_mutex);
	_streams.erase(id);
}

MediaStream::Ptr StreamManager::GetStream(const std::string& id)
{
	std::lock_guard<std::mutex> lock(_mutex);
	auto iter = _streams.find(id);
	if (iter != _streams.end())
	{
		return iter->second;
	}
	return nullptr;
}

MediaStream::Ptr StreamManager::GetStreamByCallID(int id)
{
	std::lock_guard<std::mutex> lock(_mutex);
	for (auto&& s : _streams)
	{
		if (s.second->GetType() == STREAM_TYPE_GB)
		{
			auto session = std::dynamic_pointer_cast<CallSession>(s.second);
			if (session && session->GetCallID() == id)
			{
				return s.second;
			}
		}
	}
	return nullptr;
}


std::vector<MediaStream::Ptr> StreamManager::GetAllStream()
{
	std::vector<MediaStream::Ptr> streams;
	for (auto&& s : _streams)
	{
		streams.push_back(s.second);
	}
	return streams;
}

std::vector<MediaStream::Ptr> StreamManager::GetStreamByType(STREAM_TYPE type)
{
	std::vector<MediaStream::Ptr> streams;
	for (auto&& s : _streams)
	{
		if (s.second->GetType() == type)
			streams.push_back(s.second);
	}
	return streams;
}

MediaStream::Ptr StreamManager::MakeStream(const std::string& stream_id, const std::string& app, STREAM_TYPE type)
{
	MediaStream::Ptr stream = nullptr;
	if (type == STREAM_TYPE_PROXY)
	{
		stream = std::make_shared<MediaStreamProxy>(app, stream_id);
	}
	else if (type == STREAM_TYPE_PUSH)
	{
		stream = std::make_shared<MediaStreamPushed>(app, stream_id);
	}

	if (stream)
	{
		std::lock_guard<std::mutex> lock(_mutex);
		_streams[stream->GetStreamID()] = stream;
	}
	return stream;
}

void StreamManager::ClearStreams()
{
	std::lock_guard<std::mutex> lock(_mutex);
	_streams.clear();
}