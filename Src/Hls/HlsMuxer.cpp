
#include <random>

#include "HlsMuxer.h"
#include "Common/Config.h"
#include "HlsManager.h"
#include "EventPoller/EventLoop.h"
#include "Log/Logger.h"
// #include "Codec/AacTrack.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"

using namespace std;

static int getUid()
{
	random_device rd; // 用于获得种子
    mt19937 gen(rd()); // 初始化随机数生成器，这里使用了Mersenne Twister算法
    uniform_int_distribution<> distrib(1, 100000000); // 定义分布范围[1, 100]
 
    int random_number = distrib(gen); // 生成随机数

	return random_number;
}

HlsMuxer::HlsMuxer(const UrlParser& parse)
	:_parse(parse)
{
	logTrace << "path: " << _parse.path_ + "_" + _parse.vhost_ + "_" + _parse.type_ << endl;
	_tsBuffer = make_shared<FrameBuffer>();
}

HlsMuxer::~HlsMuxer()
{
	_timeTask->quit = true;
}

void HlsMuxer::init()
{
	
}

void HlsMuxer::start()
{
	HlsManager::instance()->addMuxer(_parse.path_ + "_" + _parse.vhost_ + "_" + _parse.type_, shared_from_this());

	weak_ptr<HlsMuxer> wSelf = shared_from_this();
	if (_parse.type_ == "fmp4") {
		_fmp4Muxer = make_shared<Fmp4Muxer>(MOV_FLAG_SEGMENT);
		_fmp4Muxer->init();
		for (auto& track : _mapTrackInfo) {
			if (track.second->trackType_ == "video") {
				_fmp4Muxer->addVideoTrack(track.second);
			} else if (track.second->trackType_ == "audio") {
				_fmp4Muxer->addAudioTrack(track.second);
			}
		}
		_fmp4Muxer->fmp4_writer_init_segment();
        _fmp4Muxer->fmp4_writer_save_segment();
        _fmp4Header = _fmp4Muxer->getFmp4Header();
		_fmp4Muxer->setOnFmp4Segment([wSelf](const Buffer::Ptr& pkt, bool keyframe){
            auto self = wSelf.lock();
			if (self) {
				self->onTsPacket(pkt, 0, 0, keyframe);
			}
        });

		_fmp4Muxer->startEncode();
	} else {
		_tsMuxer = make_shared<TsMuxer>();
		_tsMuxer->setOnTsPacket([wSelf](const StreamBuffer::Ptr &pkt, int pts, int dts, bool keyframe){
			auto self = wSelf.lock();
			if (self) {
				self->onTsPacket(pkt, pts, dts, keyframe);
			}
		});

		for (auto& track : _mapTrackInfo) {
			_tsMuxer->addTrackInfo(track.second);
		}

		_tsMuxer->startEncode();
	}
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

void HlsMuxer::release()
{
	HlsManager::instance()->delMuxer(_parse.path_ + "_" + _parse.vhost_ + "_" + _parse.type_);
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

void HlsMuxer::onFrame(const FrameBuffer::Ptr& frame)
{
	if (!_muxer) {
		_tsMuxer = nullptr;
		_fmp4Muxer = nullptr;
		return ;
	}
	if (_tsMuxer) {
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
                _tsMuxer->onFrame(trackinfo->_sps);

                trackinfo->_pps->_dts = frame->_dts;
                trackinfo->_pps->_pts = frame->_pts;
                trackinfo->_pps->_index = frame->_index;
                _tsMuxer->onFrame(trackinfo->_pps);
            } else if (_mapTrackInfo[frame->_index]->codec_ == "h265") {
                auto trackinfo = dynamic_pointer_cast<H265Track>(_mapTrackInfo[frame->_index]);
                trackinfo->_vps->_dts = frame->_dts;
                trackinfo->_vps->_pts = frame->_pts;
                trackinfo->_vps->_index = frame->_index;
                _tsMuxer->onFrame(trackinfo->_vps);

                trackinfo->_sps->_dts = frame->_dts;
                trackinfo->_sps->_pts = frame->_pts;
                trackinfo->_sps->_index = frame->_index;
                _tsMuxer->onFrame(trackinfo->_sps);

                trackinfo->_pps->_dts = frame->_dts;
                trackinfo->_pps->_pts = frame->_pts;
                trackinfo->_pps->_index = frame->_index;
                _tsMuxer->onFrame(trackinfo->_pps);
            }
            // FILE* fp = fopen("psmuxer.sps", "ab+");
            // fwrite(trackinfo->_sps->data(), trackinfo->_sps->size(), 1, fp);
            // fclose(fp);
        }

		if (_hasKeyframe) {
			_tsMuxer->onFrame(frame);
		}
	} else if (_fmp4Muxer) {
		_fmp4Muxer->inputFrame_l(frame->getTrackIndex(), frame->data() + frame->startSize(), frame->size() - frame->startSize(), frame->pts(), frame->dts(), frame->keyFrame(), 0);
	}
}

void HlsMuxer::onTsPacket(const Buffer::Ptr &pkt, int pts, int dts, bool keyframe)
{
	if (_first) {
		_tsClick.update();
		_first = false;
		_lastPts = pts;
	}
	static int tsDuration = Config::instance()->getAndListen([](const json &config){
		tsDuration = config["Hls"]["Server"]["duration"];
	}, "Hls", "Server", "duration");

	static int force = Config::instance()->getAndListen([](const json &config){
		force = config["Hls"]["Server"]["force"];
	}, "Hls", "Server", "force");

	if (tsDuration == 0) {
		tsDuration = 4000;
	}

	// logInfo << "get a ts packet: " << pkt->size();

	// FILE* fp = fopen("testmuxer.ts", "ab+");
	// fwrite(pkt->data(), 1, pkt->size(), fp);
	// fclose(fp);

	// logInfo << "keyframe : " << keyframe << ", _lastPts: " << _lastPts << ", pts: " << pts
	// 		<< ", _tsClick.startToNow(): " << _tsClick.startToNow() << ", tsDuration : " << tsDuration;

	// if (keyframe) {
	// 	logInfo << "keyframe : " << keyframe;
	// }
	// if (_lastPts != pts) {
	// 	logInfo << ", _lastPts: " << _lastPts << ", pts: " << pts;
	// }
	// if (_tsClick.startToNow() > tsDuration) {
	// 	logInfo << ", _tsClick.startToNow(): " << _tsClick.startToNow() << ", tsDuration : " << tsDuration;
	// }
	if ((keyframe || force) /*&& _lastPts != pts*/ && _tsClick.startToNow() > tsDuration) {
		_tsBuffer->_dts = _tsClick.startToNow();
		updateM3u8();
		_tsClick.update();
	}

	if (_tsBuffer->size() == 0 && _fmp4Header) {
		_tsBuffer->_buffer->append(_fmp4Header->data(), _fmp4Header->size());
	}
	_tsBuffer->_buffer->append(pkt->data(), pkt->size());
	_lastPts = pts;
}

void HlsMuxer::addTrackInfo(const shared_ptr<TrackInfo>& track)
{
	_mapTrackInfo[track->index_] = track;
}

void HlsMuxer::updateM3u8()
{
	static int tsNum = Config::instance()->getAndListen([](const json &config){
		tsNum = config["Hls"]["Server"]["segNum"];
	}, "Hls", "Server", "segNum");

	if (tsNum == 0) {
		tsNum = 5;
	}

	string suff = _parse.type_ == "fmp4" ? ".m4s" : ".ts";

	string key = to_string(time(NULL)) + suff;
	string mkey = _parse.path_ + "_hls-" + key;
	stringstream ss;
	int maxDuration = 0;
	{
		lock_guard<mutex> lck(_tsMtx);
		_mapTs.emplace(mkey, _tsBuffer);

		// string name = "." + mkey;
		// auto fp = fopen(name.data(), "wb");
		// fwrite(_tsBuffer->data(), _tsBuffer->size(), 1, fp);
		// fclose(fp);
		
		logDebug << "add ts: " << mkey;
		while (_mapTs.size() > tsNum) {
			logDebug << "erase ts : " << _mapTs.begin()->first;

			_mapTs.erase(_mapTs.begin());
			++_firstTsSeq;
		}

		for (auto& ts : _mapTs) {
			if (ts.second->dts() > maxDuration) {
				maxDuration = ts.second->dts();
			}
			auto pos = ts.first.find_last_of("/");
			ss << "#EXTINF:" << ts.second->dts() / 1000.0 << "\n"
			   << ts.first.substr(pos + 1) << "?uid=" << _parse.vecParam_["uid"];

			if (_parse.type_ == "fmp4") {
				ss << "&type=fmp4";
			}

			ss << "\n";
		}

	}

	// FILE* fp = fopen(key.data(), "wb");
	// fwrite(_tsBuffer->data(), _tsBuffer->size(), 1, fp);
	// fclose(fp);
	
	stringstream ssHeader;

	_tsBuffer = make_shared<FrameBuffer>();
	
	ssHeader << "#EXTM3U\n"
	   << "#EXT-X-VERSION:7\n"
       << "#EXT-X-ALLOW-CACHE:NO\n"
	   << "#EXT-X-TARGETDURATION:" << 2 /*maxDuration / 1000.0*/ << "\n"
	   << "#EXT-X-MEDIA-SEQUENCE:" << _firstTsSeq << "\n";

	lock_guard<mutex> lck(_tsMtx);
	_m3u8 = ssHeader.str() + ss.str();

	
	// FILE* mfp = fopen("test.m3u8", "wb");
	// fwrite(_m3u8.data(), _m3u8.size(), 1, mfp);
	// fclose(mfp);

	if (_onReady) {
		logDebug << "hls onready";
		_onReady();
		_onReady = nullptr;
	}
}

string HlsMuxer::getM3u8(void* key)
{
	int uid = getUid();
	_playClick.update();

	// HlsManager::instance()->addMuxer(uid, shared_from_this());
	{
		lock_guard<mutex> lck(_uidMtx);
		HlsPlayerInfo info;
		info.key_ = key;
		logDebug << "update info.lastTime_";
		info.lastTime_ = time(NULL);
		_mapPlayer[uid] = info;
	}

	stringstream ss;
	ss << "#EXTM3U\n"
	   << "#EXT-X-STREAM-INF:BANDWIDTH=1280000\n";
	{
		// lock_guard<mutex> lck(_tsMtx);
		auto pos = _parse.path_.find_last_of("/");
		ss << _parse.path_.substr(pos + 1) << ".m3u8?uid=" << uid;
		if (_parse.type_ == "fmp4") {
			ss << "&type=fmp4";
		}
	}

	return ss.str();
}

string HlsMuxer::getM3u8WithUid(int uid)
{
	_playClick.update();

	{
		lock_guard<mutex> lck(_uidMtx);
		logDebug << "update info.lastTime_";
		_mapPlayer[uid].lastTime_ = time(NULL);
	}

	lock_guard<mutex> lck(_tsMtx);
	return _m3u8;
}

FrameBuffer::Ptr HlsMuxer::getTsBuffer(const string& key)
{
	_playClick.update();

	lock_guard<mutex> lck(_tsMtx);
	auto it = _mapTs.find(key);
	return it == _mapTs.end() ? nullptr : it->second;
}

void HlsMuxer::onManager()
{
    static int playTimeout = Config::instance()->getAndListen([](const json &config){
        playTimeout = config["Hls"]["Server"]["playTimeout"];
    }, "Hls", "Server", "playTimeout");

	if (playTimeout == 0) {
		playTimeout = 5;
	}

    lock_guard<mutex> lck(_uidMtx);
    for (auto it = _mapPlayer.begin(); it != _mapPlayer.end();) {
        int now = time(NULL);
		logDebug << "uid: " << it->first << ", now: " << now << ", it->second.lastTime_: " << it->second.lastTime_;
        if (now - it->second.lastTime_ > playTimeout) {
			auto uid = it->first;
			auto key = it->second.key_;
			// HlsManager::instance()->delMuxer(uid);
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

void HlsMuxer::onNoPLayer()
{
	if (_onNoPLayer) {
		_onNoPLayer();
	}
}

void HlsMuxer::onDelConnection(void* key)
{
	if (_onDelConnection) {
		_onDelConnection(key);
	}
}

int HlsMuxer::playerCount()
{
	lock_guard<mutex> lck(_uidMtx);
	return _mapPlayer.size();
}