#ifndef HlsFileWriter_H
#define HlsFileWriter_H

#include "Common/Frame.h"
#include "Mpeg/TsMuxer.h"
#include "Util/TimeClock.h"
#include "EventPoller/Timer.h"
#include "Common/UrlParser.h"

#include <map>
#include <string>
#include <memory>
#include <mutex>
#include "Util/File.h"


// using namespace std;

class HlsFileWriter :public std::enable_shared_from_this<HlsFileWriter>
{
public:
	using Ptr = std::shared_ptr<HlsFileWriter>;
	HlsFileWriter(const UrlParser& parse, const std::string& path_ = "",const std::string& fileName_="");
	~HlsFileWriter();

public:
    void write(const char* data, int size);
    void read(char* data, int size);
    void seek(uint64_t offset);
    size_t tell();

    bool open();
	void close();
	virtual void start();
	void stop() { _muxer = false; }
	void onFrame(const FrameBuffer::Ptr& frame);
	void onTsPacket(const StreamBuffer::Ptr& pkt, int pts, int dts, bool keyframe);
	void addTrackInfo(const std::shared_ptr<TrackInfo>& track);
	void updateM3u8();
	void flushBuffer();
private:
	std::string writeTs();
private:
    std::string _filepath;
	std::string _fileName;
    File _file;

	bool _first = true;
	bool _muxer = false;
	bool _hasKeyframe = false;
	uint64_t _firstTsSeq = 0;
	uint64_t _lastPts = 0;
	UrlParser _parse;
	//string _m3u8;
	TimeClock _tsClick;

	int max_duration = 0;
	std::shared_ptr<TimerTask> _timeTask;
	TsMuxer::Ptr _tsMuxer;
	FrameBuffer::Ptr _tsBuffer;
	std::mutex _tsMtx;

	std::string strHeader;
	std::string strContent;

	std::unordered_map<int, std::shared_ptr<TrackInfo>> _mapTrackInfo;
};

#endif
