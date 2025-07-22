﻿#ifndef HlsMuxer_H
#define HlsMuxer_H

#include "Common/Frame.h"
#include "Mpeg/TsMuxer.h"
#include "Mp4/Fmp4Muxer.h"
#include "Util/TimeClock.h"
#include "EventPoller/Timer.h"
#include "Common/UrlParser.h"

#include <map>
#include <string>
#include <memory>
#include <mutex>

using namespace std;

class HlsPlayerInfo
{
public:
	uint64_t lastTime_;
	void* key_;
};

class HlsMuxer : public enable_shared_from_this<HlsMuxer>
{
public:
	using Ptr = shared_ptr<HlsMuxer>;

	HlsMuxer(const UrlParser& parse);
	~HlsMuxer();

public:
	void init();
	virtual void start();
	void stop() {_muxer = false;}
	void release();
	void onFrame(const FrameBuffer::Ptr& frame);
	void onTsPacket(const Buffer::Ptr &pkt, int pts, int dts, bool keyframe);
	void addTrackInfo(const shared_ptr<TrackInfo>& track);
	void onManager();
	void onNoPLayer();
	void onDelConnection(void* key);
	int playerCount();
	
	void updateM3u8();
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
	UrlParser _parse;
	string _m3u8;
	TimeClock _tsClick;
	TimeClock _playClick;
	shared_ptr<TimerTask> _timeTask;
	TsMuxer::Ptr _tsMuxer;
	Fmp4Muxer::Ptr _fmp4Muxer;
	Buffer::Ptr _fmp4Header;
	FrameBuffer::Ptr _tsBuffer;
	mutex _tsMtx;
	map<string, FrameBuffer::Ptr> _mapTs;
	mutex _uidMtx;
	unordered_map<int, HlsPlayerInfo> _mapPlayer;
	// unordered_map<int, void*> _mapUid2key;
	unordered_map<int, shared_ptr<TrackInfo>> _mapTrackInfo;
	function<void()> _onNoPLayer;
	function<void()> _onReady;
	function<void(void* key)> _onDelConnection;
};

#endif
