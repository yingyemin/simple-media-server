#ifndef FlvDemuxer_H
#define FlvDemuxer_H

#include <memory>
#include <string>
#include <functional>

#include "Net/Buffer.h"

using namespace std;

enum FlvParseState
{
	FLV_PARSE_HEADER,
	FLV_PARSE_Metadata,
	FLV_PARSE_Media
};

class FlvDemuxer : public enable_shared_from_this<FlvDemuxer>
{
public:
	FlvDemuxer();
	virtual ~FlvDemuxer();

public:
	void input(const char* data, int len);

	void setOnFlvHeader(const function<void(const char* data, int len)>& cb);
	void setOnFlvMetadata(const function<void(const char* data, int len)>& cb);
	void setOnFlvMedia(const function<void(const char* data, int len)>& cb);
	void setOnError(const function<void(const string& err)>& cb);

private:
	bool parseFlvHeader(const char* data, int len);
	bool parseFlvMetadata(const char* data, int len);
	bool parseFlvMedia(const char* data, int len);
	
	void onFlvHeader(const char* data, int len);
	void onFlvMetadata(const char* data, int len);
	void onFlvMedia(const char* data, int len);
	void onError(const string& err);

private:
	FlvParseState _state = FLV_PARSE_HEADER;
	StringBuffer _remainData;
	function<void(const char* data, int len)> _onFlvHeader;
	function<void(const char* data, int len)> _onFlvMetadata;
	function<void(const char* data, int len)> _onFlvMedia;
	function<void(const string& err)> _onError;
};

#endif //FlvDemuxer_H
