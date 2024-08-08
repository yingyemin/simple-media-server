//
// H26xBinaryReader.h
//
// Library: Codec
// Package: H26x
// Module:  H26x
// 

#pragma once

#include "H264Common.h"
#include "AbstractH26xByteReader.h"

namespace Mmp
{
namespace Codec
{

class H26xBinaryReader
{
public:
    using ptr = std::shared_ptr<H26xBinaryReader>;
public:
    explicit H26xBinaryReader(AbstractH26xByteReader::ptr reader);
    virtual ~H26xBinaryReader();
public:
    void UE(uint32_t& value);
public:
    void SE(int32_t& value);
public:
    void U(size_t bits, uint64_t& value);
    void U(size_t bits, uint32_t& value, bool probe = false);
    void U(size_t bits, uint16_t& value);
    void U(size_t bits, uint8_t& value, bool probe = false);
public:
    void I(size_t bits, int64_t& value);
    void I(size_t bits, int32_t& value);
    void I(size_t bits, int16_t& value);
    void I(size_t bits, int8_t& value);
public:
    void B8(uint8_t& value);
public:
    void Skip(size_t bits);
public:
    void MoveNextByte();
    bool Eof();
public:
    void BeginNalUnit();
    void EndNalUnit();
public:
    size_t CurBits();
public:
    bool more_rbsp_data();
    void rbsp_trailing_bits();
    bool more_data_in_byte_stream();
public:
    void byte_alignment();
public:
    bool End();
public:
    void ReadOneByteAuto(bool force = false);
    bool ReadBytes(size_t byte, uint8_t* value);
private:
    uint8_t  _curBitPos;
    uint8_t  _curValue;
private:
    uint32_t _zeroCount;
    uint64_t _rbspEndByte;
private:
    bool _inNalUnit;
private:
    AbstractH26xByteReader::ptr _reader;
};

} // namespace Codec
} // namespace Mmp