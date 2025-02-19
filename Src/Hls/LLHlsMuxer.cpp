
#include <random>
#include <iomanip>

#include "LLHlsMuxer.h"
#include "Common/Config.h"
#include "LLHlsManager.h"
#include "EventPoller/EventLoop.h"
#include "Log/Logger.h"
// #include "Codec/AacTrack.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"

static int getUid()
{
	random_device rd; // 用于获得种子
    mt19937 gen(rd()); // 初始化随机数生成器，这里使用了Mersenne Twister算法
    uniform_int_distribution<> distrib(1, 100); // 定义分布范围[1, 100]
 
    int random_number = distrib(gen); // 生成随机数

	return random_number;
}

LLHlsMuxer::LLHlsMuxer(const UrlParser& parse)
	:_parse(parse)
{
	_fmp4Buffer = make_shared<FrameBuffer>();
}

LLHlsMuxer::~LLHlsMuxer()
{
	_timeTask->quit = true;
	// LLHlsManager::instance()->delMuxer(_parse.path_ + "_" + _parse.vhost_ + "_" + _parse.type_);
}

void LLHlsMuxer::init()
{
	
}

void LLHlsMuxer::start()
{
	LLHlsManager::instance()->addMuxer(_parse.path_ + "_" + _parse.vhost_ + "_" + _parse.type_, shared_from_this());

	weak_ptr<LLHlsMuxer> wSelf = dynamic_pointer_cast<LLHlsMuxer>(shared_from_this());
	_fmp4Muxer = make_shared<Fmp4Muxer>(MOV_FLAG_SEGMENT);
	_fmp4Muxer->init();
	// _fmp4Muxer->setOnFmp4Header([wSelf](const Buffer::Ptr &buffer){
	// 	auto self = wSelf.lock();
	// 	if (self) {
	// 		self->_fmp4Header = buffer;
	// 		self->_fmp4Buffer->_buffer.append(buffer->data(), buffer->size());
	// 	}
	// });
	_fmp4Muxer->setOnFmp4Segment([wSelf](const Buffer::Ptr &buffer, bool keyframe){
		auto self = wSelf.lock();
		if (self) {
			self->onFmp4Packet(buffer, keyframe);
		}
	});

	for (auto& track : _mapTrackInfo) {
		if (track.second->trackType_ == "audio") {
			_fmp4Muxer->addAudioTrack(track.second);
		} else {
			_fmp4Muxer->addVideoTrack(track.second);
		}
	}

	_fmp4Muxer->fmp4_writer_init_segment();
	_fmp4Muxer->fmp4_writer_save_segment();
	_fmp4Header = _fmp4Muxer->getFmp4Header();
	_fmp4Buffer->_buffer.append(_fmp4Header->data(), _fmp4Header->size());

	_fmp4Muxer->startEncode();
	_muxer = true;

	static int intervel = Config::instance()->getAndListen([](const json &config){
        intervel = config["Hls"]["Server"]["intervel"];
    }, "Hls", "Server", "intervel");

	if (intervel == 0) {
		intervel = 5000;
	}

	auto loop = EventLoop::getCurrentLoop();
	loop->addTimerTask(intervel, [wSelf](){
		auto self = wSelf.lock();
		if (!self) {
			return 0;
		}
		self->onManager();
		return intervel;
	}, [wSelf](bool success, const shared_ptr<TimerTask>& task){
		auto self = wSelf.lock();
		if (self) {
			self->_timeTask = task;
		}
	});
	_playClick.update();
}

void LLHlsMuxer::release()
{
	LLHlsManager::instance()->delMuxer(_parse.path_ + "_" + _parse.vhost_ + "_" + _parse.type_);
	// vector<int> vecUid;
	// {
	// 	lock_guard<mutex> lck(_uidMtx);
	// 	for (auto& pr : _mapUid2key) {
	// 		vecUid.push_back(pr.first);
	// 	}
	// }
	// for (auto& uid : vecUid) {
	// 	HlsManager::instance()->delMuxer(uid);
	// }
}

void LLHlsMuxer::onFrame(const FrameBuffer::Ptr& frame)
{
	if (!_muxer) {
		_fmp4Muxer = nullptr;
		return ;
	}
	if (_fmp4Muxer) {
		if (frame->metaFrame() || frame->getNalType() == 6) {
			return ;
		}
		if (frame->keyFrame()) {
			_hasKeyframe = true;
            if (_mapTrackInfo[frame->_index]->codec_ == "h264") {
                auto trackinfo = dynamic_pointer_cast<H264Track>(_mapTrackInfo[frame->_index]);
                trackinfo->_sps->_dts = frame->_dts;
                trackinfo->_sps->_pts = frame->_pts;
                trackinfo->_sps->_index = frame->_index;
                _fmp4Muxer->inputFrame(trackinfo->index_, trackinfo->_sps->data(), trackinfo->_sps->size(), frame->_dts, frame->_pts, true);

                trackinfo->_pps->_dts = frame->_dts;
                trackinfo->_pps->_pts = frame->_pts;
                trackinfo->_pps->_index = frame->_index;
                _fmp4Muxer->inputFrame(trackinfo->index_, trackinfo->_pps->data(), trackinfo->_pps->size(), frame->_dts, frame->_pts, true);
            } else if (_mapTrackInfo[frame->_index]->codec_ == "h265") {
                auto trackinfo = dynamic_pointer_cast<H265Track>(_mapTrackInfo[frame->_index]);
                trackinfo->_vps->_dts = frame->_dts;
                trackinfo->_vps->_pts = frame->_pts;
                trackinfo->_vps->_index = frame->_index;
                _fmp4Muxer->inputFrame(trackinfo->index_, trackinfo->_vps->data(), trackinfo->_vps->size(), frame->_dts, frame->_pts, true);

                trackinfo->_sps->_dts = frame->_dts;
                trackinfo->_sps->_pts = frame->_pts;
                trackinfo->_sps->_index = frame->_index;
                _fmp4Muxer->inputFrame(trackinfo->index_, trackinfo->_sps->data(), trackinfo->_sps->size(), frame->_dts, frame->_pts, true);

                trackinfo->_pps->_dts = frame->_dts;
                trackinfo->_pps->_pts = frame->_pts;
                trackinfo->_pps->_index = frame->_index;
                _fmp4Muxer->inputFrame(trackinfo->index_, trackinfo->_pps->data(), trackinfo->_pps->size(), frame->_dts, frame->_pts, true);
            }
            // FILE* fp = fopen("psmuxer.sps", "ab+");
            // fwrite(trackinfo->_sps->data(), trackinfo->_sps->size(), 1, fp);
            // fclose(fp);
        }

		if (_hasKeyframe) {
			_fmp4Muxer->inputFrame(frame->_index, frame->data(), frame->size(), frame->_dts, frame->_pts, true);
		}
	}
}

void LLHlsMuxer::onFmp4Packet(const Buffer::Ptr &pkt, bool keyframe)
{
	if (_first) {
		_tsClick.update();
		_segClick.update();
		_first = false;
		// _lastPts = pts;

		std::stringstream ss;
		std::tm now = std::tm();
		ss << std::put_time(&now, "%Y-%m-%d %H:%M:%S");
		_lastSysTime = ss.str();

		string key = to_string(time(NULL)) + ".ts";
		string mkey = _parse.path_ + "_hls-" + key;
		_mapFmp4[mkey]._sysTime = _lastSysTime;
	}
	static int tsDuration = Config::instance()->getAndListen([](const json &config){
		tsDuration = config["LLHls"]["Server"]["duration"];
	}, "LLHls", "Server", "duration");

	if (tsDuration == 0) {
		tsDuration = 4000;
	}
	static int segDuration = Config::instance()->getAndListen([](const json &config){
		segDuration = config["LLHls"]["Server"]["segDuration"];
	}, "LLHls", "Server", "segDuration");

	if (segDuration == 0) {
		segDuration = 500;
	}

	// logInfo << "get a ts packet: " << pkt->size();

	// FILE* fp = fopen("testmuxer.ts", "ab+");
	// fwrite(pkt->data(), 1, pkt->size(), fp);
	// fclose(fp);

	// logInfo << "keyframe : " << keyframe << ", _lastPts: " << _lastPts << ", pts: " << pts;

	if (keyframe && /*_lastPts != pts &&*/ _tsClick.startToNow() > tsDuration) {
		// _fmp4Buffer->_dts = _tsClick.startToNow();
		updateSeg();
		updateM3u8();
		_tsClick.update();
		_segClick.update();

		std::stringstream ss;
		std::tm now = std::tm();
		ss << std::put_time(&now, "%Y-%m-%d %H:%M:%S");
		_lastSysTime = ss.str();

		string key = to_string(time(NULL)) + ".ts";
		string mkey = _parse.path_ + "_hls-" + key;
		_mapFmp4[mkey]._sysTime = _lastSysTime;
	}

	if (_segClick.startToNow() > segDuration) {
		_fmp4Buffer->_dts = _segClick.startToNow();
		updateSeg();
		updateM3u8();
		_segClick.update();
	}

	_fmp4Buffer->_buffer.append(pkt->data(), pkt->size());
	// _lastPts = pts;
}

void LLHlsMuxer::addTrackInfo(const shared_ptr<TrackInfo>& track)
{
	_mapTrackInfo[track->index_] = track;
}

void LLHlsMuxer::updateSeg()
{
	auto iter = _mapFmp4.rbegin();
	string key = iter->first + "." + to_string(iter->second._index++) + ".mp4";
	iter->second._mapSeg[key] = _fmp4Buffer;

	_fmp4Buffer = make_shared<FrameBuffer>();
	_fmp4Buffer->_buffer.append(_fmp4Header->data(), _fmp4Header->size());
}

void LLHlsMuxer::updateM3u8()
{
	std::stringstream ssTime;
	std::tm now = std::tm();
	ssTime << std::put_time(&now, "%Y-%m-%d %H:%M:%S");
	_lastSysTime = ssTime.str();

	static int tsNum = Config::instance()->getAndListen([](const json &config){
		tsNum = config["Hls"]["Server"]["segNum"];
	}, "Hls", "Server", "segNum");

	if (tsNum == 0) {
		tsNum = 5;
	}

	stringstream ss;
	int maxDuration = 0;
	{
		lock_guard<mutex> lck(_tsMtx);
		while (_mapFmp4.size() > tsNum) {
			logInfo << "erase mp4 : " << _mapFmp4.begin()->first;

			_mapFmp4.erase(_mapFmp4.begin());
			++_firstTsSeq;
		}

		for (auto& iter : _mapFmp4) {
			int curDuration = 0;
			ss << "#EXT-X-PROGRAM-DATE-TIME:" << iter.second._sysTime << "\n";
			for (auto &mp4It: iter.second._mapSeg) {
				curDuration += mp4It.second->dts();
				ss << "#EXT-X-PART:DURATION=" << mp4It.second->dts() << ",URI=\"" << mp4It.first << "\",INDEPENDENT=YES\n";
			}
			// if (ts.second->dts() > maxDuration) {
			// 	maxDuration = ts.second->dts();
			// }
			// auto pos = ts.first.find_last_of("/");
			// ss << "#EXTINF:" << ts.second->dts() / 1000.0 << "\n"
			//    << ts.first.substr(pos + 1) + "\n";
		}

	}

	// FILE* fp = fopen(key.data(), "wb");
	// fwrite(_fmp4Buffer->data(), _fmp4Buffer->size(), 1, fp);
	// fclose(fp);
	
	stringstream ssHeader;
	
	ssHeader << "#EXTM3U\n"
	   << "#EXT-X-VERSION:3\n"
       << "#EXT-X-ALLOW-CACHE:NO\n"
	   << "#EXT-X-TARGETDURATION:" << 20 /*maxDuration / 1000.0*/ << "\n"
	   << "#EXT-X-MEDIA-SEQUENCE:" << _firstTsSeq << "\n";

	lock_guard<mutex> lck(_tsMtx);
	_m3u8 = ssHeader.str() + ss.str();

	
	// FILE* mfp = fopen("test.m3u8", "wb");
	// fwrite(_m3u8.data(), _m3u8.size(), 1, mfp);
	// fclose(mfp);

	if (_onReady) {
		logInfo << "hls onready";
		_onReady();
		_onReady = nullptr;
	}
}

string LLHlsMuxer::getM3u8(void* key)
{
	int uid = getUid();
	_playClick.update();

	// LLHlsManager::instance()->addMuxer(uid, shared_from_this());
	{
		lock_guard<mutex> lck(_uidMtx);
		LLHlsPlayerInfo info;
		info.key_ = key;
		info.lastTime_ = time(NULL);
		_mapPlayer[uid] = info;
	}

	stringstream ss;
	ss << "#EXTM3U\n"
	   << "#EXT-X-STREAM-INF:BANDWIDTH=1280000\n";
	{
		// lock_guard<mutex> lck(_tsMtx);
		auto pos = _parse.path_.find_last_of("/");
		ss << _parse.path_.substr(pos + 1) << ".ll.m3u8?uid=" << uid;
	}

	return ss.str();
}

string LLHlsMuxer::getM3u8WithUid(int uid)
{
	_playClick.update();

	{
		lock_guard<mutex> lck(_uidMtx);
		_mapPlayer[uid].lastTime_ = time(NULL);
	}

	lock_guard<mutex> lck(_tsMtx);
	return _m3u8;
}

FrameBuffer::Ptr LLHlsMuxer::getTsBuffer(const string& key)
{
	_playClick.update();

	lock_guard<mutex> lck(_tsMtx);
	auto it = _mapFmp4.find(key);
	return it == _mapFmp4.end() ? nullptr : it->second._mapSeg.begin()->second;
}

void LLHlsMuxer::onManager()
{
    static int playTimeout = Config::instance()->getAndListen([](const json &config){
        playTimeout = config["Hls"]["Server"]["PlayTimeout"];
    }, "Hls", "Server", "PlayTimeout");

	if (playTimeout == 0) {
		playTimeout = 5;
	}

    lock_guard<mutex> lck(_uidMtx);
    for (auto it = _mapPlayer.begin(); it != _mapPlayer.end();) {
        int now = time(NULL);
        if (now - it->second.lastTime_ > playTimeout) {
			auto uid = it->first;
			auto key = it->second.key_;
			// LLHlsManager::instance()->delMuxer(uid);
            it = _mapPlayer.erase(it);
			
			// auto key = _mapUid2key[uid];
			onDelConnection(key);
			// _mapUid2key.erase(uid);
        } else {
            ++it;
        }
    }

	if (_mapPlayer.size() == 0 && _playClick.startToNow() > playTimeout) {
		onNoPLayer();
	}
}

void LLHlsMuxer::onNoPLayer()
{
	if (_onNoPLayer) {
		_onNoPLayer();
	}
}

void LLHlsMuxer::onDelConnection(void* key)
{
	if (_onDelConnection) {
		_onDelConnection(key);
	}
}