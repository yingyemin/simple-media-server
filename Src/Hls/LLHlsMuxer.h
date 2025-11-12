#ifndef LLHlsMuxer_H
#define LLHlsMuxer_H

#include "Common/Frame.h"
#include "Mp4/Fmp4Muxer.h"
#include "Util/TimeClock.h"
#include "EventPoller/Timer.h"
#include "Common/UrlParser.h"
// #include "HlsMuxer.h"

#include <map>
#include <string>
#include <memory>
#include <mutex>

// using namespace std;

class LLHlsPlayerInfo
{
public:
	uint64_t lastTime_;
	void* key_;
};

class Mp4SegInfo
{
public:
	int _duration = 0;
	int _index = 0;
	std::string _sysTime;
	std::map<std::string, FrameBuffer::Ptr> _mapSeg;
};

class LLHlsMuxer : public std::enable_shared_from_this<LLHlsMuxer>
{
public:
	using Ptr = std::shared_ptr<LLHlsMuxer>;

	LLHlsMuxer(const UrlParser& parse);
	~LLHlsMuxer();

public:
	void init();
	void start();
	void stop() {_muxer = false;}
	void release();
	void onFrame(const FrameBuffer::Ptr& frame);
	void onFmp4Packet(const Buffer::Ptr &pkt, bool keyframe);
	void addTrackInfo(const std::shared_ptr<TrackInfo>& track);
	void onManager();
	void onNoPLayer();
	void onDelConnection(void* key);
	
	void updateM3u8();
	void updateSeg();
	void setOnNoPlayer(const std::function<void()>& cb) { _onNoPLayer = cb; }
	void setOnReady(const std::function<void()>& cb) { _onReady = cb; }
	void setOnDelConnection(const std::function<void(void* key)>& cb) { _onDelConnection = cb; }

	// 两级m3u8
	std::string getM3u8(void* key);
	std::string getM3u8WithUid(int uid);
	FrameBuffer::Ptr getTsBuffer(const std::string& key);

private:
	bool _first = true;
	bool _muxer = false;
	bool _hasKeyframe = false;
	uint64_t _firstTsSeq = 0;
	uint64_t _lastPts = 0;
	std::string _lastSysTime;
	UrlParser _parse;
	std::string _m3u8;
	TimeClock _tsClick;
	TimeClock _segClick;
	TimeClock _playClick;
	std::shared_ptr<TimerTask> _timeTask;
	Fmp4Muxer::Ptr _fmp4Muxer;
	FrameBuffer::Ptr _fmp4Buffer;
	Buffer::Ptr _fmp4Header;
	std::mutex _tsMtx;
	std::map<std::string, Mp4SegInfo> _mapFmp4;
	std::mutex _uidMtx;
	std::unordered_map<int, LLHlsPlayerInfo> _mapPlayer;
	// std::unordered_map<int, void*> _mapUid2key;
	std::unordered_map<int, std::shared_ptr<TrackInfo>> _mapTrackInfo;
	std::function<void()> _onNoPLayer;
	std::function<void()> _onReady;
	std::function<void(void* key)> _onDelConnection;
};

#endif
