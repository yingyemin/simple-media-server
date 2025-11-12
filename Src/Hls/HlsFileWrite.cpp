
#include <random>
#include "HlsFileWrite.h"
#include "Log/Logger.h"
#include "Common/Config.h"
#include "EventPoller/EventLoop.h"
#include "Log/Logger.h"
// #include "Codec/AacTrack.h"
#include "Codec/H264Track.h"
#include "Codec/H265Track.h"

using namespace std;

HlsFileWriter::HlsFileWriter(const UrlParser& parse, const std::string& path_, const std::string& fileName_)
	:_parse(parse), _filepath(path_),_fileName(fileName_)
{
    logTrace << "path: " << _parse.path_ + "_" + _parse.vhost_ + "_" + _parse.type_ << endl;
    _tsBuffer = make_shared<FrameBuffer>();
}

HlsFileWriter::~HlsFileWriter()
{
	
}

void HlsFileWriter::write(const char* data, int size)
{
    _file.write(data, size);
}

void HlsFileWriter::read(char* data, int size)
{
    //_file.read(data, size);
}

void HlsFileWriter::seek(uint64_t offset)
{
    
}

size_t HlsFileWriter::tell()
{
    return _file.tell();
}

bool HlsFileWriter::open()
{
    if (!_file.open(_filepath, "wb+")) {
        logWarn << "open m3u8 file failed: " << _filepath;
        return false;
    }
    return true;
}

void HlsFileWriter::start()
{
	weak_ptr<HlsFileWriter> wSelf = shared_from_this();
	_tsMuxer = make_shared<TsMuxer>();
	_tsMuxer->setOnTsPacket([wSelf](const StreamBuffer::Ptr& pkt, int pts, int dts, bool keyframe) {
		auto self = wSelf.lock();
		if (self) {
			self->onTsPacket(pkt, pts, dts, keyframe);
		}
		});

	for (auto& track : _mapTrackInfo) {
		_tsMuxer->addTrackInfo(track.second);
	}
	strHeader = "";
	strContent = "";
	_tsMuxer->startEncode();
	_muxer = true;

}

void HlsFileWriter::onFrame(const FrameBuffer::Ptr& frame)
{
	if (!_muxer) {
		_tsMuxer = nullptr;
		return;
	}
	if (_tsMuxer) {
		if (frame->metaFrame() || frame->getNalType() == 6) {
			return;
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
			}
			else if (_mapTrackInfo[frame->_index]->codec_ == "h265") {
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
	}
}

void HlsFileWriter::onTsPacket(const StreamBuffer::Ptr& pkt, int pts, int dts, bool keyframe)
{
	if (_first) {
		_tsClick.update();
		_first = false;
		_lastPts = pts;
	}
	static int tsDuration = Config::instance()->getAndListen([](const json& config) {
		tsDuration = config["Hls"]["Server"]["duration"];
		}, "Hls", "Server", "duration");

	static int force = Config::instance()->getAndListen([](const json& config) {
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
	// 如果是关键帧更新m3u8
	if ((keyframe || force) /*&& _lastPts != pts*/ && _tsClick.startToNow() > tsDuration) {
		_tsBuffer->_dts = _tsClick.startToNow();
		updateM3u8();
		_tsClick.update();
	}

	_tsBuffer->_buffer->append(pkt->data(), pkt->size());
	_lastPts = pts;
}

void HlsFileWriter::addTrackInfo(const shared_ptr<TrackInfo>& track)
{
	_mapTrackInfo[track->index_] = track;
}

std::string HlsFileWriter::writeTs()
{
	string key = to_string(time(NULL)) + ".ts";
	//string mkey = _fileName + key;
	

	size_t pos_ = _filepath.rfind("/");
	std::string str_dir = _filepath.substr(0, pos_+1);
	std::string tmp_path = str_dir + _fileName + "/" + key;


	File fp;
	if (fp.open(tmp_path, "wb"))
	{
		fp.write(_tsBuffer->data(), _tsBuffer->size());
		fp.close();
	}
	return _fileName + "/" + key;
}

void HlsFileWriter::updateM3u8()
{
	static int tsNum = Config::instance()->getAndListen([](const json& config) {
		tsNum = config["Hls"]["Server"]["segNum"];
		}, "Hls", "Server", "segNum");

	if (tsNum == 0) {
		tsNum = 5;
	}

	
	std::string file_path = writeTs();
	if (_tsBuffer->dts() > max_duration) {
		max_duration = _tsBuffer->dts();

		stringstream ssHeader;
		ssHeader << "#EXTM3U\n"
			<< "#EXT-X-VERSION:3\n"
			<< "#EXT-X-ALLOW-CACHE:NO\n"
			<< "#EXT-X-TARGETDURATION:" << max_duration/1000.0 << "\n"
			<< "#EXT-X-MEDIA-SEQUENCE:" << _firstTsSeq << "\n";

		lock_guard<mutex> lck(_tsMtx);
		strHeader = ssHeader.str();
	}
	stringstream ss;
	//auto pos = mkey.find_last_of("/");
	ss << "#EXTINF:" << _tsBuffer->dts() / 1000.0 << "\n"
		<< file_path + "\n";

	_tsBuffer = make_shared<FrameBuffer>();

	
	lock_guard<mutex> lck(_tsMtx);
	strContent += ss.str();

}

void HlsFileWriter::flushBuffer()
{
	if (_tsBuffer.get())
	{
		//string key = to_string(time(NULL)) + ".ts";
		//string mkey = _parse.path_ + "_hls-" + key;
		stringstream ss;

		//size_t pos_ = _filepath.rfind("/");
		//std::string str_dir = _filepath.substr(0, pos_);
		//std::string tmp_path = str_dir + mkey;

		//File fp;
		//if (fp.open(tmp_path, "wb"))
		//{
		//	fp.write(_tsBuffer->data(), _tsBuffer->size());
		//	fp.close();
		//}
		_tsBuffer->_dts = _tsClick.startToNow();
		std::string file_path = writeTs();
		if (_tsBuffer->dts() > max_duration) {
			max_duration = _tsBuffer->dts();
			stringstream ssHeader;
			ssHeader << "#EXTM3U\n"
				<< "#EXT-X-VERSION:3\n"
				<< "#EXT-X-ALLOW-CACHE:NO\n"
				<< "#EXT-X-TARGETDURATION:" << max_duration << "\n"
				<< "#EXT-X-MEDIA-SEQUENCE:" << _firstTsSeq << "\n";

			lock_guard<mutex> lck(_tsMtx);
			strHeader = ssHeader.str();
		}
		//auto pos = mkey.find_last_of("/");
		ss << "#EXTINF:" << _tsBuffer->dts() / 1000.0 << "\n"
			<< file_path + "\n";

		ss << "#EXT-X-ENDLIST";
		lock_guard<mutex> lck(_tsMtx);
		strContent += ss.str();
	}
}

void HlsFileWriter::close()
{
	flushBuffer();
	write(strHeader.c_str(), strHeader.length());
	write(strContent.c_str(), strContent.length());
	_file.close();
}