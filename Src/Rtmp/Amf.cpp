#include "Amf.h"
#include "Util/String.h"
#include "Log/Logger.h"

int AmfDecoder::decode(const char *data, int size, int n)
{
	if (!data || size <= 0) {
		return 0;
	}

	int bytes_used = 0;
	while (size - bytes_used > 0)
	{
		int ret = 0;
		char type = data[bytes_used];
		bytes_used += 1;

		logInfo << "type: " << (int)type;

		switch (type)
		{
		case AMF0_NUMBER:
			_obj.type_ = AMF_NUMBER;
			ret = decodeNumber(data + bytes_used, size - bytes_used, _obj.amfNumber_);
			break;

		case AMF0_BOOLEAN:
			_obj.type_ = AMF_BOOLEAN;
			ret = decodeBoolean(data + bytes_used, size - bytes_used, _obj.amfBoolean_);
			break;

		case AMF0_STRING:
		// logInfo << "decode a string";
			_obj.type_ = AMF_STRING;
			ret = decodeString(data + bytes_used, size - bytes_used, _obj.amfString_);
			break;

		case AMF0_OBJECT:
			// logInfo << "decode a object";
			ret = decodeObject(data + bytes_used, size - bytes_used, _objs);
			break;

		case AMF0_OBJECT_END:
			break;

		case AMF0_ECMA_ARRAY:
			ret = decodeObject(data + bytes_used + 4, size - bytes_used - 4, _objs);
			break;

		case AMF0_NULL:
			break;

		default:
			break;
		}

		if (ret < 0) {
			break;
		}

		bytes_used += ret;
		n--;
		if (n == 0) {
			break;
		}
	}

	return bytes_used > size ? size : bytes_used;
}

int AmfDecoder::decodeNumber(const char *data, int size, double& amf_number)
{
	if (!data || size < 8) {
		return 0;
	}

	char *ci = (char*)data;
	char *co = (char*)&amf_number;
	co[0] = ci[7];
	co[1] = ci[6];
	co[2] = ci[5];
	co[3] = ci[4];
	co[4] = ci[3];
	co[5] = ci[2];
	co[6] = ci[1];
	co[7] = ci[0];

	return 8;
}

int AmfDecoder::decodeString(const char *data, int size, std::string& amf_string)
{
	if (!data || size < 2) {
		return 0;
	}

	int bytes_used = 0;
	int strSize = decodeInt16(data, size);
	bytes_used += 2;
	if (strSize > (size - bytes_used)) {
		return -1;
	}

	if (strSize == 0) {
		return bytes_used;
	}
	// logInfo << "bytes_used: " << bytes_used;
	// logInfo << "strSize: " << strSize;
	// logInfo << "size: " << size;

	amf_string.assign(data + bytes_used, strSize);
	// logInfo << "amf_string: " << amf_string;
	bytes_used += strSize;
	return bytes_used;
}

int AmfDecoder::decodeObject(const char *data, int size, AmfObjects& amf_objs)
{
	if (!data || size <= 0) {
		return 0;
	}

	amf_objs.clear();
	int bytes_used = 0;
	// logInfo << "size: " << size;
	while (size > 0)
	{
		// logInfo << "size: " << size;
		int strLen = decodeInt16(data + bytes_used, size);
		// logInfo << "strLen: " << strLen;
		size -= 2;
		if (size < strLen || strLen == 0) {
			return bytes_used + 2 + 1;
		}

		// logInfo << "bytes_used + 2 + strLen: " << (bytes_used + 2 + strLen);
		// logInfo << "size: " << size;
		// logInfo << "strLen: " << strLen;
		std::string key(data + bytes_used + 2, 0, strLen);
		size -= strLen;

		AmfDecoder dec;
		int ret = dec.decode(data + bytes_used + 2 + strLen, size, 1);
		bytes_used += 2 + strLen + ret;
		size -= ret;
		if (ret <= 1) {
			break;
		}

		// logInfo << "key: " << key;
		amf_objs.emplace(key, dec.getObject());
	}

	return bytes_used > size ? size : bytes_used;
}

int AmfDecoder::decodeBoolean(const char *data, int size, bool& amf_boolean)
{
	if (!data || size < 1) {
		return 0;
	}

	amf_boolean = (data[0] != 0);
	return 1;
}

uint16_t AmfDecoder::decodeInt16(const char *data, int size)
{
	if (!data || size < 2) {
		return 0;
	}

	uint16_t val = readUint16BE((char*)data);
	return val;
}

uint32_t AmfDecoder::decodeInt24(const char *data, int size)
{
	if (!data || size < 3) {
		return 0;
	}
	uint32_t val = readUint24BE((char*)data);
	return val;
}

uint32_t AmfDecoder::decodeInt32(const char *data, int size)
{
	if (!data || size < 4) {
		return 0;
	}
	uint32_t val = readUint32BE((char*)data);
	return val;
}

AmfEncoder::AmfEncoder(uint32_t size)
	: _data(make_shared<StreamBuffer>(size + 1))
	, _size(size)
{
}

AmfEncoder::~AmfEncoder()
{
}

void AmfEncoder::encodeInt8(int8_t value)
{
	if ((_size - _index) < 1) {
		this->realloc(_size + 1024);
	}

	_data->data()[_index++] = value;
}

void AmfEncoder::encodeInt16(int16_t value)
{
	if ((_size - _index) < 2) {
		this->realloc(_size + 1024);
	}

	writeUint16BE(_data->data() + _index, value);
	_index += 2;
}

void AmfEncoder::encodeInt24(int32_t value)
{
	if ((_size - _index) < 3) {
		this->realloc(_size + 1024);
	}

	writeUint24BE(_data->data() + _index, value);
	_index += 3;
}

void AmfEncoder::encodeInt32(int32_t value)
{
	if ((_size - _index) < 4) {
		this->realloc(_size + 1024);
	}

	writeUint32BE(_data->data() + _index, value);
	_index += 4;
}

void AmfEncoder::encodeString(const char *str, int len, bool isObject)
{
	if ((int)(_size - _index) < (len + 1 + 2 + 2)) {
		this->realloc(_size + len + 5);
	}

	if (len < 65536) {
		if (isObject) {
			_data->data()[_index++] = AMF0_STRING;
		}
		encodeInt16(len);
	}
	else {
		if (isObject) {
			_data->data()[_index++] = AMF0_LONG_STRING;
		}
		encodeInt32(len);
	}

	memcpy(_data->data() + _index, str, len);
	_index += len;
}

void AmfEncoder::encodeNumber(double value)
{
	if ((_size - _index) < 9) {
		this->realloc(_size + 1024);
	}

	_data->data()[_index++] = AMF0_NUMBER;

	char* ci = (char*)&value;
	char* co = _data->data();
	co[_index++] = ci[7];
	co[_index++] = ci[6];
	co[_index++] = ci[5];
	co[_index++] = ci[4];
	co[_index++] = ci[3];
	co[_index++] = ci[2];
	co[_index++] = ci[1];
	co[_index++] = ci[0];
}

void AmfEncoder::encodeBoolean(int value)
{
	if ((_size - _index) < 2) {
		this->realloc(_size + 1024);
	}

	_data->data()[_index++] = AMF0_BOOLEAN;
	_data->data()[_index++] = value ? 0x01 : 0x00;
}

void AmfEncoder::encodeObjects(AmfObjects& objs)
{
	if (objs.size() == 0) {
		encodeInt8(AMF0_NULL);
		return;
	}

	encodeInt8(AMF0_OBJECT);

	for (auto iter : objs) {
		encodeString(iter.first.c_str(), (int)iter.first.size(), false);
		switch (iter.second.type_)
		{
		case AMF_NUMBER:
			encodeNumber(iter.second.amfNumber_);
			break;
		case AMF_STRING:
			encodeString(iter.second.amfString_.c_str(), (int)iter.second.amfString_.size());
			break;
		case AMF_BOOLEAN:
			encodeBoolean(iter.second.amfBoolean_);
			break;
		default:
			break;
		}
	}

	encodeString("", 0, false);
	encodeInt8(AMF0_OBJECT_END);
}

void AmfEncoder::encodeECMA(AmfObjects& objs)
{
	encodeInt8(AMF0_ECMA_ARRAY);
	encodeInt32(0);

	for (auto iter : objs) {
		encodeString(iter.first.c_str(), (int)iter.first.size(), false);
		switch (iter.second.type_)
		{
		case AMF_NUMBER:
			encodeNumber(iter.second.amfNumber_);
			break;
		case AMF_STRING:
			encodeString(iter.second.amfString_.c_str(), (int)iter.second.amfString_.size());
			break;
		case AMF_BOOLEAN:
			encodeBoolean(iter.second.amfBoolean_);
			break;
		default:
			break;
		}
	}

	encodeString("", 0, false);
	encodeInt8(AMF0_OBJECT_END);
}

void AmfEncoder::realloc(uint32_t size)
{
	if (size <= _size) {
		return;
	}

	StreamBuffer::Ptr data = make_shared<StreamBuffer>(size + 1);
	memcpy(data->data(), _data->data(), _index);
	_size = size;
	_data = data;
}
