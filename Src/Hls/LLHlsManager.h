#ifndef LLHlsManager_H
#define LLHlsManager_H

#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

#include "LLHlsMuxer.h"

// using namespace std;

class LLHlsManager
{
public:
	using Ptr = std::shared_ptr<LLHlsManager>;
	
	LLHlsManager();
	~LLHlsManager();

public:
	static LLHlsManager::Ptr& instance();

	void addMuxer(const std::string& key, const LLHlsMuxer::Ptr& muxer);
	LLHlsMuxer::Ptr getMuxer(const std::string& key);
	void delMuxer(const std::string& key);

	// void addMuxer(int uid, const LLHlsMuxer::Ptr& muxer);
	// LLHlsMuxer::Ptr getMuxer(int uid);
	// void delMuxer(int uid);

private:
	// std::mutex _muxerMtx;
	// std::unordered_map<int, LLHlsMuxer::Ptr> _mapMuxer;

	std::mutex _muxerStrMtx;
	std::unordered_map<std::string, LLHlsMuxer::Ptr> _mapStrMuxer;
};

#endif
