#ifndef HlsManager_H
#define HlsManager_H

#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

#include "HlsMuxer.h"

using namespace std;

class HlsManager
{
public:
	using Ptr = shared_ptr<HlsManager>;
	
	HlsManager();
	~HlsManager();

public:
	static HlsManager::Ptr& instance();

	void addMuxer(const string& key, const HlsMuxer::Ptr& muxer);
	HlsMuxer::Ptr getMuxer(const string& key);
	void delMuxer(const string& key);

	void addMuxer(int uid, const HlsMuxer::Ptr& muxer);
	HlsMuxer::Ptr getMuxer(int uid);
	void delMuxer(int uid);

private:
	mutex _muxerMtx;
	unordered_map<int, HlsMuxer::Ptr> _mapMuxer;

	mutex _muxerStrMtx;
	unordered_map<string, HlsMuxer::Ptr> _mapStrMuxer;
};

#endif
