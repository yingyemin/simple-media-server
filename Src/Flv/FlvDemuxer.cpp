#include "FlvDemuxer.h"
#include "Util/String.hpp"
#include "Log/Logger.h"

using namespace std;

FlvDemuxer::FlvDemuxer()
{}

FlvDemuxer::~FlvDemuxer()
{}

void FlvDemuxer::input(const char* data, int len)
{
	// 从配置中获取
    static int maxRemainSize = 4 * 1024 * 1024;
	bool ret = true;

	int remainSize = _remainData.size();
	if (remainSize > maxRemainSize) {
		logError << "remain cache is too large";
		_remainData.clear();
		return ;
	}

	if (remainSize > 0) {
		logInfo << "remainSize: " << remainSize;
		_remainData.append(data, len);
		data = _remainData.data();
		len += remainSize;
	}

	do {
		logInfo << "_state is: " << (int)_state << ", len is: " << len;

		if (_state == FLV_PARSE_HEADER) {
			ret = parseFlvHeader(data, len);
		} else if (_state == FLV_PARSE_Metadata) {
			ret = parseFlvMetadata(data, len);
		} else if (_state == FLV_PARSE_Media) {
			ret = parseFlvMedia(data, len);
		}
		
		data = _remainData.data();
		len = _remainData.size();
	} while (ret && _remainData.size() > 0);
}

bool FlvDemuxer::parseFlvHeader(const char* data, int len)
{
	if (len < 9 + 4) {
		_remainData.assign(data, len);
		return false;
	}

	auto header = data;
	onFlvHeader(header, 9);

	len -= 9 + 4;
	if (len > 0) {
		_remainData.assign(data + 9 + 4, len);
	} else {
		_remainData.clear();
	}

	_state = FLV_PARSE_Metadata;

	return true;
}

bool FlvDemuxer::parseFlvMetadata(const char* data, int len)
{
	if (len < 11) {
		_remainData.assign(data, len);
		return false;
	}

	if (data[0] == 8 || data[0] == 9) {
		_state = FLV_PARSE_Media;
		return parseFlvMedia(data, len);;
	} else if (data[0] != 18) {
		logError << "invalid flv tag header type: " << (int)data[0];
		onError("invalid flv tag header type");
		return false;
	}

	int tagLen = readUint24BE(data + 1);
	if (tagLen + 11 + 4> len) {
		_remainData.assign(data, len);
		return false;
	}

	onFlvMetadata(data, tagLen + 11);

	len -= tagLen + 11 + 4;
	if (len > 0) {
		_remainData.assign(data + 11 + tagLen + 4, len);
	} else {
		_remainData.clear();
	}

	_state = FLV_PARSE_Media;

	return true;
}

bool FlvDemuxer::parseFlvMedia(const char* data, int len)
{
	if (len < 11) {
		_remainData.assign(data, len);
		return false;
	}

	int tagLen = readUint24BE(data + 1);
	logInfo << "tagLen: " << tagLen;
	if (tagLen + 11 + 4> len) {
		_remainData.assign(data, len);
		return false;
	}

	onFlvMedia(data, tagLen + 11);

	len -= tagLen + 11 + 4;
	if (len > 0) {
		_remainData.assign(data + 11 + tagLen + 4, len);
	} else {
		_remainData.clear();
	}

	return true;
}
	
void FlvDemuxer::onFlvHeader(const char* data, int len)
{
	if (_onFlvHeader) {
		_onFlvHeader(data, len);
	}
}

void FlvDemuxer::onFlvMetadata(const char* data, int len)
{
	if (_onFlvMetadata) {
		_onFlvMetadata(data, len);
	}
}

void FlvDemuxer::onFlvMedia(const char* data, int len)
{
	if (_onFlvMedia) {
		_onFlvMedia(data, len);
	}
}

void FlvDemuxer::onError(const string& err)
{
	if (_onError) {
		_onError(err);
	}
}

void FlvDemuxer::resetToSeek()
{
	_remainData.clear();
	_state = FLV_PARSE_Media;
}

void FlvDemuxer::setOnFlvHeader(const function<void(const char* data, int len)>& cb)
{
	_onFlvHeader = cb;
}

void FlvDemuxer::setOnFlvMetadata(const function<void(const char* data, int len)>& cb)
{
	_onFlvMetadata = cb;
}

void FlvDemuxer::setOnFlvMedia(const function<void(const char* data, int len)>& cb)
{
	_onFlvMedia = cb;
}

void FlvDemuxer::setOnError(const function<void(const string& err)>& cb)
{
	_onError = cb;
}
