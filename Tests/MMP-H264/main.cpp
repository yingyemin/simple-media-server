#include <memory.h>
#include <cstdint>
#include <iostream>
#include <fstream>
#include <cassert>
#include <memory>
#include <sstream>
#include <chrono>
#include <string>

#include "AbstractH26xByteReader.h"
#include "H26xBinaryReader.h"
#include "H264Deserialize.h"
#include "H265Deserialize.h"

namespace Mmp
{
namespace Codec
{

/**
 * @brief simple AbstractH26xByteReader implemention based on std::ifstream 
 */
class SimpleFileH264ByteReader : public Mmp::Codec::AbstractH26xByteReader
{
public:
    explicit SimpleFileH264ByteReader(const std::string& h264Path);
    ~SimpleFileH264ByteReader();
public:
    size_t Read(void* data, size_t bytes) override;
    bool Seek(size_t offset) override;
    size_t Tell() override;
    bool Eof() override;
private:
    std::ifstream _ifs;
};

SimpleFileH264ByteReader::SimpleFileH264ByteReader(const std::string& h264Path)
{
    _ifs.open(h264Path, std::ios::in | std::ios::binary);
    if (!_ifs.is_open())
    {
        assert(false);
        exit(255);
    }
}

SimpleFileH264ByteReader::~SimpleFileH264ByteReader()
{
    _ifs.close();
}

size_t SimpleFileH264ByteReader::Read(void* data, size_t bytes)
{
    _ifs.read((char*) data, bytes);
    return _ifs.gcount();
}

bool SimpleFileH264ByteReader::Seek(size_t offset)
{
    _ifs.seekg(offset);
    return _ifs.tellg() == offset;
}

size_t SimpleFileH264ByteReader::Tell()
{
    return _ifs.tellg();
}

bool SimpleFileH264ByteReader::Eof()
{
    return _ifs.eof();
}

} // namespace Codec
} // namespace Mmp

namespace Mmp
{
namespace Codec
{

constexpr uint32_t kBufSize = 1024 * 1024;

/**
 * @brief memory cache AbstractH26xByteReader implemention based on std::ifstream 
 */
class CacheFileH264ByteReader : public Mmp::Codec::AbstractH26xByteReader
{
public:
    explicit CacheFileH264ByteReader(const std::string& h264Path);
    ~CacheFileH264ByteReader();
public:
    size_t Read(void* data, size_t bytes) override;
    bool Seek(size_t offset) override;
    size_t Tell() override;
    bool Eof() override;
private:
    std::ifstream _ifs;
private:
    uint8_t* _buf;
    uint32_t _offset;
    uint32_t _cur;
    uint32_t _len;
};

CacheFileH264ByteReader::CacheFileH264ByteReader(const std::string& h264Path)
{
    _ifs.open(h264Path, std::ios::in | std::ios::binary);
    if (!_ifs.is_open())
    {
        assert(false);
        exit(255);
    }
    _buf = new uint8_t[kBufSize];
    _offset = _ifs.tellg();
    _ifs.read((char*)_buf, kBufSize);
    _cur = 0;
    _len = _ifs.gcount();
}

CacheFileH264ByteReader::~CacheFileH264ByteReader()
{
    delete[] _buf;
    _ifs.close();
}

size_t CacheFileH264ByteReader::Read(void* data, size_t bytes)
{
    if (_cur + bytes < _len)
    {
        memcpy(data, _buf + _cur, bytes);
        _cur += bytes;
        return bytes;
    }
    else if (_ifs.eof())
    {
        memcpy(data, _buf + _cur, _len -  _cur);
        return _len -  _cur;
    }
    else
    {
        _offset = _ifs.tellg();
        _ifs.readsome((char*)_buf, kBufSize);
        _cur = 0;
        _len = _ifs.gcount(); 
        if (_len == 0) /* eof */
        {
            return 0;
        }
        else
        {
            return Read(data, bytes);
        }
    }
}

bool CacheFileH264ByteReader::Seek(size_t offset)
{
    if (offset < _offset)
    {
        _ifs.seekg(offset);
        _offset = _ifs.tellg();
        _ifs.readsome((char*)_buf, kBufSize);
        _cur = 0;
        _len = _ifs.gcount(); 
        return _offset == offset;
    }
    else if (offset > _offset + kBufSize)
    {
        _ifs.seekg(offset);
        _offset = _ifs.tellg();
        _ifs.readsome((char*)_buf, kBufSize);
        _cur = 0;
        _len = _ifs.gcount(); 
        return _offset == offset;
    }
    else
    {
        _cur = offset - _offset;
        return true;
    }
}

size_t CacheFileH264ByteReader::Tell()
{
    return _offset + _cur;
}

bool CacheFileH264ByteReader::Eof()
{
    return _len == 0 || (_ifs.eof() && _cur == _len);
}

} // namespace Codec
} // namespace Mmp

using namespace Mmp::Codec;

static std::string H264NalUintTypeToStr(uint32_t nal_unit_type)
{
    switch (nal_unit_type)
    {
        case H264NaluType::MMP_H264_NALU_TYPE_NULL: return "null";
        case H264NaluType::MMP_H264_NALU_TYPE_SLICE: return "slice";
        case H264NaluType::MMP_H264_NALU_TYPE_DPA: return "dpa";
        case H264NaluType::MMP_H264_NALU_TYPE_DPB: return "dpb";
        case H264NaluType::MMP_H264_NALU_TYPE_DPC: return "dpc";
        case H264NaluType::MMP_H264_NALU_TYPE_IDR: return "idr";
        case H264NaluType::MMP_H264_NALU_TYPE_SEI: return "sei";
        case H264NaluType::MMP_H264_NALU_TYPE_SPS: return "sps";
        case H264NaluType::MMP_H264_NALU_TYPE_PPS: return "pps";
        case H264NaluType::MMP_H264_NALU_TYPE_AUD: return "aud";
        case H264NaluType::MMP_H264_NALU_TYPE_EOSEQ: return "eoseq";
        case H264NaluType::MMP_H264_NALU_TYPE_EOSTREAM: return "eostream";
        case H264NaluType::MMP_H264_NALU_TYPE_FILL: return "fill";
        case H264NaluType::MMP_H264_NALU_TYPE_SPSEXT: return "spsext";
        case H264NaluType::MMP_H264_NALU_TYPE_PREFIX: return "prefix";
        case H264NaluType::MMP_H264_NALU_TYPE_SUB_SPS: return "subsps";
        case H264NaluType::MMP_H264_NALU_TYPE_SLC_EXT: return "slcext";
        case H264NaluType::MMP_H264_NALU_TYPE_VDRD: return "vord";
        default:
            return "unkonwn";
    }
}

static std::string H265NalUintTypeToStr(uint32_t nal_unit_type)
{
    switch (nal_unit_type) 
    {
        case H265NaluType::MMP_H265_NALU_TYPE_TRAIL_N: return "trail n";
        case H265NaluType::MMP_H265_NALU_TYPE_TRAIL_R: return "trail r";
        case H265NaluType::MMP_H265_NALU_TYPE_TSA_N: return "tsa n";
        case H265NaluType::MMP_H265_NALU_TYPE_TSA_R: return "tsa r";
        case H265NaluType::MMP_H265_NALU_TYPE_STSA_N: return "stsa n";
        case H265NaluType::MMP_H265_NALU_TYPE_STSA_R: return "stsa r";
        case H265NaluType::MMP_H265_NALU_TYPE_RADL_R: return "radl r";
        case H265NaluType::MMP_H265_NALU_TYPE_RASL_R: return "rasl r";
        case H265NaluType::MMP_H265_NALU_TYPE_RSV_VCL_N10: return "rsv vcl n10";
        case H265NaluType::MMP_H265_NALU_TYPE_RSV_VCL_N12: return "rsv vcl n12";
        case H265NaluType::MMP_H265_NALU_TYPE_RSV_VCL_N14T: return "src vcl n14t";
        case H265NaluType::MMP_H265_NALU_TYPE_RSV_VCL_R11: return "rsv vcl r11";
        case H265NaluType::MMP_H265_NALU_TYPE_RSV_VCL_R13: return "rsv vcl r13";
        case H265NaluType::MMP_H265_NALU_TYPE_RSV_VCL_R15: return "rsv vcl r15";
        case H265NaluType::MMP_H265_NALU_TYPE_BLA_W_LP: return "bla w lp";
        case H265NaluType::MMP_H265_NALU_TYPE_BLA_W_RADL: return "bla w radl";
        case H265NaluType::MMP_H265_NALU_TYPE_BLA_N_LP: return "bla n lp";
        case H265NaluType::MMP_H265_NALU_TYPE_IDR_W_RADL: return "idr w rald";
        case H265NaluType::MMP_H265_NALU_TYPE_IDR_N_LP: return "idr n lp";
        case H265NaluType::MMP_H265_NALU_TYPE_RSV_IRAP_VCL22: return "rsv irap vcl22";
        case H265NaluType::MMP_H265_NALU_TYPE_RSV_IRAP_VCL23: return "rsv irap vcl23";
        case H265NaluType::MMP_H265_NALU_TYPE_VPS_NUT: return "vps nut";
        case H265NaluType::MMP_H265_NALU_TYPE_SPS_NUT: return "sps nut";
        case H265NaluType::MMP_H265_NALU_TYPE_PPS_NUT: return "pps nut";
        case H265NaluType::MMP_H265_NALU_TYPE_AUD_NUT: return "aud nut";
        case H265NaluType::MMP_H265_NALU_TYPE_EOS_NUT: return "eos nut";
        case H265NaluType::MMP_H265_NALU_TYPE_EOB_NUT: return "eob nut";
        case H265NaluType::MMP_H265_NALU_TYPE_FD_NUT: return "fd nut";
        case H265NaluType::MMP_H265_NALU_TYPE_PREFIX_SEI_NUT: return "prefix sei nut";
        case H265NaluType::MMP_H265_NALU_TYPE_SUFFIX_SEI_NUT: return "suffix sei nut";
        default:
            return "unkonwn";
    }
}

void Usage()
{
    std::stringstream ss;
    ss << "[usage] ./Sample [xxx.h264 | xxx.h265]" << std::endl;
    std::cout << ss.str() << std::endl;
}

int main(int argc, char* argv[])
{
    if (argc != 2 || (std::string(argv[1]).find(".h264") == std::string::npos && std::string(argv[1]).find(".h265") == std::string::npos))
    {
        Usage();
        return -1;
    }
#if 0 /* slow but simple */
    AbstractH26xByteReader::ptr byteReader = std::make_shared<SimpleFileH264ByteReader>(std::string(argv[1]));
#else /* fast but a bit complicated  */
    AbstractH26xByteReader::ptr byteReader = std::make_shared<CacheFileH264ByteReader>(std::string(argv[1]));
#endif
    if (std::string(argv[1]).find(".h264") != std::string::npos)
    {
        H26xBinaryReader::ptr binaryReader = std::make_shared<H26xBinaryReader>(byteReader);
        H264Deserialize::ptr deserialize = std::make_shared<H264Deserialize>();
        std::vector<H264NalSyntax::ptr> nals;
        bool res = true;
        int num = 0;
        auto begin = std::chrono::system_clock::now();
        do
        {
            num++;
            H264NalSyntax::ptr nal = std::make_shared<H264NalSyntax>();
            auto start = std::chrono::system_clock::now();
            res = deserialize->DeserializeByteStreamNalUnit(binaryReader, nal);
            std::cout << "(" << num << ")" << "  "  << "[" << H264NalUintTypeToStr(nal->nal_unit_type) << "]" 
                    << " cost time :" << (std::chrono::system_clock::now() - start).count() / (1000 * 1000) << " ms"
                    << std::endl;
            if (res)
            {
                nals.push_back(nal);
            }
        } while (res && !binaryReader->Eof());
        std::cout << "total cost time : " << (std::chrono::system_clock::now() - begin).count() / (1000 * 1000) << "ms";
    }
    else if (std::string::npos && std::string(argv[1]).find(".h265") != std::string::npos)
    {
        H26xBinaryReader::ptr binaryReader = std::make_shared<H26xBinaryReader>(byteReader);
        H265Deserialize::ptr deserialize = std::make_shared<H265Deserialize>();
        std::vector<H265NalSyntax::ptr> nals;
        bool res = true;
        int num = 0;
        auto begin = std::chrono::system_clock::now();
        do
        {
            num++;
            H265NalSyntax::ptr nal = std::make_shared<H265NalSyntax>();
            auto start = std::chrono::system_clock::now();
            res = deserialize->DeserializeByteStreamNalUnit(binaryReader, nal);
            std::cout << "(" << num << ")" << "  "  << "[" << H265NalUintTypeToStr(nal->header->nal_unit_type) << "]" 
                    << " cost time :" << (std::chrono::system_clock::now() - start).count() / (1000 * 1000) << " ms"
                    << std::endl;
            if (res)
            {
                nals.push_back(nal);
            }
        } while (res && !binaryReader->Eof());
        std::cout << "total cost time : " << (std::chrono::system_clock::now() - begin).count() / (1000 * 1000) << "ms";
    }

    return 0;
}