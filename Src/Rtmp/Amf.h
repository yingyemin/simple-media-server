#ifndef AMF_OBJECT_H
#define AMF_OBJECT_H

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <memory>
#include <map>
#include <unordered_map>

#include "Net/Buffer.h"

#pragma pack(1)

typedef enum
{ 
	AMF0_NUMBER = 0, 
	AMF0_BOOLEAN, 
	AMF0_STRING, 
	AMF0_OBJECT,
	AMF0_MOVIECLIP,		/* reserved, not used */
	AMF0_NULL, 
	AMF0_UNDEFINED, 
	AMF0_REFERENCE, 
	AMF0_ECMA_ARRAY, 
	AMF0_OBJECT_END,
	AMF0_STRICT_ARRAY, 
	AMF0_DATE, 
	AMF0_LONG_STRING, 
	AMF0_UNSUPPORTED,
	AMF0_RECORDSET,		/* reserved, not used */
	AMF0_XML_DOC, 
	AMF0_TYPED_OBJECT,
	AMF0_AVMPLUS,		/* switch to AMF3 */
	AMF0_INVALID = 0xff
} AMF0DataType;

typedef enum
{
	AMF3_INVALID = 0,
	AMF3_NULL, 
    AMF3_FALSE,
    AMF3_TRUE,
    AMF3_INTEGER,
	AMF3_NUMBER, 
	AMF3_STRING, 
    AMF3_LEGACY_XML,
	AMF3_DATE, 
	AMF3_ARRAY, 
	AMF3_OBJECT,
    AMF3_XML,
    AMF3_BYTE_ARRAY
} AMF3DataType;
    
typedef enum
{
	AMF_NUMBER,
	AMF_BOOLEAN,
	AMF_STRING,
} AmfObjectType;

class AmfObject
{
public:  
	AmfObject(){}

	AmfObject(std::string str)
	{
		this->type_ = AMF_STRING; 
		this->amfString_ = str; 
	}

	AmfObject(double number)
	{
		this->type_ = AMF_NUMBER; 
		this->amfNumber_ = number; 
	}

	AmfObject(int number)
	{
		this->type_ = AMF_NUMBER; 
		this->amfNumber_ = number; 
	}

	AmfObject(bool number)
	{
		this->type_ = AMF_BOOLEAN; 
		this->amfBoolean_ = number; 
	}

public:
	AmfObjectType type_;

	std::string amfString_;
	double amfNumber_ = 0;
	bool amfBoolean_;  
};

typedef std::unordered_map<std::string, AmfObject> AmfObjects;

class AmfDecoder
{
public:    
	/* n: 解码次数 */
    int decode(const char *data, int size, int n=-1);

	void setVersion(int version) 
	{_version = version;}

    void reset()
    {
        _obj.amfString_ = "";
        _obj.amfNumber_ = 0;
        _obj.amfBoolean_ = false;
        _objs.clear();
    }

    std::string getString() const
    { return _obj.amfString_; }

    double getNumber() const
    { return _obj.amfNumber_; }

    bool hasObject(std::string key) const
    { return (_objs.find(key) != _objs.end()); }

    AmfObject getObject(std::string key) 
    { return _objs[key]; }

    AmfObject getObject() 
    { return _obj; }

    AmfObjects getObjects() 
    { return _objs; }
    
private:    
    static int decodeBoolean(const char *data, int size, bool& amf_boolean);
    static int decodeNumber(const char *data, int size, double& amf_number);
    static int decodeString(const char *data, int size, std::string& amf_string);
    static int decodeObject(const char *data, int size, AmfObjects& amf_objs);
    static uint16_t decodeInt16(const char *data, int size);
    static uint32_t decodeInt24(const char *data, int size);
    static uint32_t decodeInt32(const char *data, int size);

private:
	int _version;
    AmfObject _obj;
    AmfObjects _objs;    
};

class AmfEncoder
{
public:
	AmfEncoder(uint32_t size = 1024);
	virtual ~AmfEncoder();
     
	void reset()
	{
		_index = 0;
	}
     
	StreamBuffer::Ptr data()
	{
		return _data;
	}

	uint32_t size() const 
	{
		return _index;
	}
     
	void encodeString(const char* str, int len, bool isObject=true);
	void encodeNumber(double value);
	void encodeBoolean(int value);
	void encodeObjects(AmfObjects& objs);
	void encodeECMA(AmfObjects& objs);
     
private:
	void encodeInt8(int8_t value);
	void encodeInt16(int16_t value);
	void encodeInt24(int32_t value);
	void encodeInt32(int32_t value); 
	void realloc(uint32_t size);

private:
	StreamBuffer::Ptr _data;    
	uint32_t _size  = 0;
	uint32_t _index = 0;
};

#pragma pack()

#endif
