#ifndef LLHlsManager_H
#define LLHlsManager_H

#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

#include "LLHlsMuxer.h"

using namespace std;

class LLHlsManager
{
public:
	using Ptr = shared_ptr<LLHlsManager>;
	
	LLHlsManager();
	~LLHlsManager();

public:
	static LLHlsManager::Ptr& instance();

	void addMuxer(const string& key, const LLHlsMuxer::Ptr& muxer);
	LLHlsMuxer::Ptr getMuxer(const string& key);
	void delMuxer(const string& key);

	// void addMuxer(int uid, const LLHlsMuxer::Ptr& muxer);
	// LLHlsMuxer::Ptr getMuxer(int uid);
	// void delMuxer(int uid);

private:
	// mutex _muxerMtx;
	// unordered_map<int, LLHlsMuxer::Ptr> _mapMuxer;

	mutex _muxerStrMtx;
	unordered_map<string, LLHlsMuxer::Ptr> _mapStrMuxer;
};

#endif
