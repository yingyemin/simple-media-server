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

using namespace std;

class Mp4SegInfo
{
public:
	int _duration = 0;
	int _index = 0;
	string _sysTime;
	map<string, FrameBuffer::Ptr> _mapSeg;
};

class LLHlsMuxer : public enable_shared_from_this<LLHlsMuxer>
{
public:
	using Ptr = shared_ptr<LLHlsMuxer>;

	LLHlsMuxer(const UrlParser& parse);
	~LLHlsMuxer();

public:
	void init();
	void start();
	void stop() {_muxer = false;}
	void onFrame(const FrameBuffer::Ptr& frame);
	void onFmp4Packet(const Buffer::Ptr &pkt, bool keyframe);
	void addTrackInfo(const shared_ptr<TrackInfo>& track);
	void onManager();
	void onNoPLayer();
	void onDelConnection(void* key);
	
	void updateM3u8();
	void updateSeg();
	void setOnNoPlayer(const function<void()>& cb) { _onNoPLayer = cb; }
	void setOnReady(const function<void()>& cb) { _onReady = cb; }
	void setOnDelConnection(const function<void(void* key)>& cb) { _onDelConnection = cb; }

	// 两级m3u8
	string getM3u8(void* key);
	string getM3u8WithUid(int uid);
	FrameBuffer::Ptr getTsBuffer(const string& key);

private:
	bool _first = true;
	bool _muxer = false;
	bool _hasKeyframe = false;
	uint64_t _firstTsSeq = 0;
	uint64_t _lastPts = 0;
	string _lastSysTime;
	UrlParser _parse;
	string _m3u8;
	TimeClock _tsClick;
	TimeClock _segClick;
	TimeClock _playClick;
	shared_ptr<TimerTask> _timeTask;
	Fmp4Muxer::Ptr _fmp4Muxer;
	FrameBuffer::Ptr _fmp4Buffer;
	Buffer::Ptr _fmp4Header;
	mutex _tsMtx;
	map<string, Mp4SegInfo> _mapFmp4;
	mutex _uidMtx;
	unordered_map<int, uint64_t> _mapUid2Time;
	unordered_map<int, void*> _mapUid2key;
	unordered_map<int, shared_ptr<TrackInfo>> _mapTrackInfo;
	function<void()> _onNoPLayer;
	function<void()> _onReady;
	function<void(void* key)> _onDelConnection;
};

#endif
