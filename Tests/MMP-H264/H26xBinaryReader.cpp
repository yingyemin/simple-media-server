#include "H26xBinaryReader.h"

#include <cassert>
#include <cstdint>
#include <exception>
#include <stdexcept>
#include <string>

#include "H26xUltis.h"

namespace Mmp
{
namespace Codec
{

// Quick Note:
//
// See also : ISO 14496/10(2020) - 7.2 Specification of syntax functions, categories, and descriptors
//
//  - ae(v): context-adaptive arithmetic entropy-coded syntax element. The parsing process for this descriptor is specified 
//  in subclause 9.3.
//  - b(8): byte having any pattern of bit string (8 bits). The parsing process for this descriptor is specified by the return 
//  value of the function read_bits( 8 ).
//  - ce(v): context-adaptive variable-length entropy-coded syntax element with the left bit first. The parsing process for 
//  this descriptor is specified in subclause 9.2.
//  - f(n): fixed-pattern bit string using n bits written (from left to right) with the left bit first. The parsing process for this 
//  descriptor is specified by the return value of the function read_bits( n ).
//  - i(n): signed integer using n bits. When n is "v" in the syntax table, the number of bits varies in a manner dependent 
//  on the value of other syntax elements. The parsing process for this descriptor is specified by the return value of the 
//  function read_bits( n ) interpreted as a two's complement integer representation with most significant bit written first.
//  - me(v): mapped Exp-Golomb-coded syntax element with the left bit first. The parsing process for this descriptor is 
//  specified in subclause 9.1.
//  - se(v): signed integer Exp-Golomb-coded syntax element with the left bit first. The parsing process for this descriptor 
//  is specified in subclause 9.1.
//  - te(v): truncated Exp-Golomb-coded syntax element with left bit first. The parsing process for this descriptor is 
//  specified in subclause 9.1.
//  - u(n): unsigned integer using n bits. When n is "v" in the syntax table, the number of bits varies in a manner 
//  dependent on the value of other syntax elements. The parsing process for this descriptor is specified by the return 
//  value of the function read_bits( n ) interpreted as a binary representation of an unsigned integer with most significant 
//  bit written first.
//  - ue(v): unsigned integer Exp-Golomb-coded syntax element with the left bit first. The parsing process for this
//  descriptor is specified in subclause 9.1.
//

static uint8_t kLeftAndLookUp[8] = {0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01};
static uint8_t kRightAndLookUp[8] = {0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, 0xFF};


H26xBinaryReader::H26xBinaryReader(AbstractH26xByteReader::ptr reader)
{
    _rbspEndByte = 0;
    _curBitPos = 8;
    _curValue = 0;
    _reader = reader;
    _inNalUnit = false;
    _zeroCount = 0;
}

H26xBinaryReader::~H26xBinaryReader()
{

}

void H26xBinaryReader::UE(uint32_t& value)
{
    // See also : ISO 14496/10(2020) - 9.1 Parsing process for Exp-Golomb codes
    int32_t leadingZeroBits = -1;
    uint64_t tmp = 0;
    for (uint8_t b = 0; b == 0; leadingZeroBits++)
    {
        U(1, b);
        assert(b == 0 || b == 1);
    }
    U(leadingZeroBits, tmp);
    value = (uint32_t)((1 << leadingZeroBits) - 1 + tmp);
}

void H26xBinaryReader::SE(int32_t& value)
{
    // See also : ISO 14496/10(2020) -  Table 9-3 – Assignment of syntax element to codeNum for signed Exp-Golomb coded syntax elements se(v)}
    uint32_t codeNum = 0;
    UE(codeNum);
    if (codeNum % 2 == 0)
    {
        value = -(codeNum >> 1);
    }
    else
    {
        value = (codeNum >> 1) + 1;
    }
}

#define MMP_U_OPERATION(bits, value)            value = 0;\
                                                bool firstFlag = false;\
                                                do\
                                                {\
                                                    ReadOneByteAuto();\
                                                    size_t readBits = (size_t)bits <= (8 - (size_t)_curBitPos)? bits : (8 - (size_t)_curBitPos);\
                                                    bits = (size_t)(bits - readBits);\
                                                    value <<= readBits;\
                                                    if (readBits < 8 && !firstFlag)\
                                                    {\
                                                        value |= (_curValue & kLeftAndLookUp[_curBitPos]) >> (8 - _curBitPos - readBits);\
                                                    }\
                                                    else if (readBits < 8)\
                                                    {\
                                                        value |= (_curValue & kRightAndLookUp[readBits - 1]) >> (8 - _curBitPos - readBits);\
                                                    }\
                                                    else\
                                                    {\
                                                        value |= _curValue;\
                                                    }\
                                                    _curBitPos = _curBitPos + readBits;\
                                                    firstFlag = true;\
                                                } while (bits != 0);

#define MMP_U_PRED_OPERATION(bits, value)       uint8_t curBitPos = _curBitPos;\
                                                size_t curPosByte = curBitPos == 8 ? _reader->Tell() : _reader->Tell() - 1;\
                                                MMP_U_OPERATION(bits, value);\
                                                _reader->Seek(curPosByte);\
                                                if (curBitPos != 8)\
                                                {\
                                                    ReadOneByteAuto(true);\
                                                }\
                                                _curBitPos = curBitPos;

#define MMP_I_OPERATION(bits, value)            MMP_U_OPERATION(bits, value)

#define MMP_I_PRED_OPERATION(bits, value)       MMP_U_PRED_OPERATION(bits, value)

void H26xBinaryReader::U(size_t bits, uint64_t& value)
{
    if (bits > 64)
    {
        throw std::out_of_range(std::string());
    }
    MMP_U_OPERATION(bits, value);
}

void H26xBinaryReader::U(size_t bits, uint32_t& value, bool probe)
{
    if (bits > 32)
    {
        throw std::out_of_range(std::string());
    }
    if (!probe)
    {
        MMP_U_OPERATION(bits, value);
    }
    else
    {
        MMP_U_PRED_OPERATION(bits, value);
    }
}

void H26xBinaryReader::U(size_t bits, uint16_t& value)
{
    if (bits > 16)
    {
        throw std::out_of_range(std::string());
    }
    MMP_U_OPERATION(bits, value);
}

void H26xBinaryReader::U(size_t bits, uint8_t& value, bool probe)
{
    if (bits > 8)
    {
        throw std::out_of_range(std::string());
    }
    if (!probe)
    {
        MMP_U_OPERATION(bits, value);
    }
    else
    {
        MMP_U_PRED_OPERATION(bits, value);
    }
}

void H26xBinaryReader::I(size_t bits, int64_t& value)
{
    if (bits > 64)
    {
        throw std::out_of_range(std::string());
    }
    MMP_I_OPERATION(bits, value);
}

void H26xBinaryReader::I(size_t bits, int32_t& value)
{
    if (bits > 32)
    {
        throw std::out_of_range(std::string());
    }
    MMP_I_OPERATION(bits, value);
}

void H26xBinaryReader::I(size_t bits, int16_t& value)
{
    if (bits > 16)
    {
        throw std::out_of_range(std::string());
    }
    MMP_I_OPERATION(bits, value);
}

void H26xBinaryReader::I(size_t bits, int8_t& value)
{
    if (bits > 8)
    {
        throw std::out_of_range(std::string());
    }
    MMP_I_OPERATION(bits, value);
}

#undef MMP_I_PRED_OPERATION
#undef MMP_I_OPERATION
#undef MMP_U_PRED_OPERATION
#undef MMP_U_OPERATION

void H26xBinaryReader::B8(uint8_t& value)
{
    U(8, value);
}

void H26xBinaryReader::Skip(size_t bits)
{
    if (bits + _curBitPos < 8) // 不需要跳转至下一个字节
    {
        _curBitPos = uint8_t(bits + _curBitPos);
        return;
    }
    else // 需要跳转的 bits 先用消费当前 byte 剩余的 bits
    {
        bits = bits - (8 - _curBitPos);
        _curBitPos = 8;
    }
    size_t skipByte = bits / 8; // 计算需要跳转的字节数
    if (skipByte)
    {
        _reader->Seek(_reader->Tell() + skipByte);
    }
    ReadOneByteAuto(true);
    _curBitPos = bits % 8;
}

void H26xBinaryReader::MoveNextByte()
{
    _curBitPos = 8;
    ReadOneByteAuto();
}

bool H26xBinaryReader::Eof()
{
    return _reader->Eof();
}

void H26xBinaryReader::BeginNalUnit()
{
    _inNalUnit = true;
}


void H26xBinaryReader::EndNalUnit()
{
    _inNalUnit = false;
}

size_t H26xBinaryReader::CurBits()
{
    return _reader->Tell() * 8 + (_curBitPos % 8);
}

bool H26xBinaryReader::more_rbsp_data()
{
    // Hint :
    // more_rbsp_data( ) is specified as follows:
    // - If there is no more data in the RBSP, the return value of more_rbsp_data( ) is equal to FALSE.
    // - Otherwise, the RBSP data is searched for the last (least significant, right-most) bit equal to 1 that is present in the 
    //   RBSP. Given the position of this bit, which is the first bit (rbsp_stop_one_bit) of the rbsp_trailing_bits( ) syntax 
    //   structure, the following applies:
    // - If there is more data in an RBSP before the rbsp_trailing_bits( ) syntax structure, the return value of 
    //   more_rbsp_data( ) is equal to TRUE.
    // - Otherwise, the return value of more_rbsp_data( ) is equal to FALSE.
    //
    // Hint : 寻找下一个 RBSP 起点 NAL START CODE (0x000003),
    //        根据 ISO 描述, 在 rbsp_trailing_bits() 后可能存在几个字节的 zero, 此部分不算做 RBSP 范畴内
    //        
    if (_rbspEndByte > _reader->Tell()) // cache hint
    {
        return true;
    }
    else if (_reader->Tell() == _rbspEndByte) // reach end of rbsp
    {
        return false;
    }
    else // update _rbspEndByte and try once again
    {
        bool inNalUnit = _inNalUnit;
        _inNalUnit = false;
        uint8_t curBitPos = _curBitPos;
        size_t curPosByte = curBitPos == 8 ? _reader->Tell() : _reader->Tell() - 1;
        // 1 - 寻找下一个 RBSP 起点 NAL START CODE (0x000003)
        {
            try 
            {
                uint32_t next_24_bits = 0;
                U(24, next_24_bits, true);
                while (next_24_bits != 0x000001)
                {
                    if ((next_24_bits & 0xFFFF) == 0)
                    {
                        Skip(8);
                    }
                    else if ((next_24_bits & 0xFF) == 0)
                    {
                        Skip(16);
                    }
                    else
                    {
                        Skip(32);
                    }
                    _rbspEndByte = _reader->Tell();
                    U(24, next_24_bits, true);
                }
            }
            catch (...)
            {
                // Hint : 剩余长度不足 3 时可以进入此异常
                _rbspEndByte = _reader->Tell();
            }
        }
        // 2 - 移除 rbsp_trailing_bits() 后的几个 zero byte (,如果存在的话)
        {
            try
            {
                uint8_t zeroByte = 0;
                do
                {
                    U(8, zeroByte, true);
                    if (zeroByte == 0)
                    {
                        _reader->Seek(_reader->Tell() - 1);
                        _rbspEndByte = _reader->Tell();
                    }
                } while (zeroByte == 0);   
            }
            catch (...)
            {
                // Hint : 无可读数据进入此异常
                // nothing to do
            }
        }
        _reader->Seek(curPosByte);
        if (curBitPos != 8)
        {
            ReadOneByteAuto(true);
        }
        _curBitPos = curBitPos;
        _inNalUnit = inNalUnit;
        return more_rbsp_data();
    }

    return true;
}

void H26xBinaryReader::rbsp_trailing_bits()
{
    // See also : ISO 14496/10(2020) - 7.3.2.11 RBSP trailing bits syntax
    uint8_t rbsp_stop_one_bit = 0;
    uint8_t rbsp_alignment_zero_bit;
    U(1, rbsp_stop_one_bit);
    assert(rbsp_stop_one_bit == 1);
    while (!(_curBitPos == 0 || _curBitPos == 8))
    {
        U(1, rbsp_alignment_zero_bit);
        // assert(rbsp_alignment_zero_bit == 0);
    }
}

bool H26xBinaryReader::more_data_in_byte_stream()
{
    // Hint :
    // more_data_in_byte_stream( ), which is used only in the byte stream NAL unit syntax structure specified in Annex B, is 
    // specified as follows:
    // - If more data follow in the byte stream, the return value of more_data_in_byte_stream( ) is equal to TRUE.
    // - Otherwise, the return value of more_data_in_byte_stream( ) is equal to FALSE.
    return !_reader->Eof();
}

void H26xBinaryReader::byte_alignment()
{
    assert(false);
    // TODO
}

bool H26xBinaryReader::End()
{
    assert(false);
    return true;
}

bool H26xBinaryReader::ReadBytes(size_t byte, uint8_t* value)
{
    size_t readByte = _reader->Read(value, byte);
    if (readByte != byte)
    {
        throw std::out_of_range("");
    }
    return readByte == byte;
}

void H26xBinaryReader::ReadOneByteAuto(bool force)
{
    if (_curBitPos == 8 || force)
    {
        ReadBytes(1, &_curValue);
        //  Hint : 0x0000030x -> 0x00000x
        //  The RBSP data is searched for byte-aligned bits of the following binary patterns:
        //  '00000000 00000000 000000xx' (where xx represents any 2 bit pattern: 00, 01, 10, or 11),
        //  and a byte equal to 0x03 is inserted to replace these bit patterns with the patterns:
        //  '00000000 00000000 00000011 000000xx',
        //  and finally, when the last byte of the RBSP data is equal to 0x00 (which can only occur when the RBSP ends in a 
        //  cabac_zero_word), a final byte equal to 0x03 is appended to the end of the data. The last zero byte of a byte-aligned 
        //  three-byte sequence 0x000000 in the RBSP (which is replaced by the four-byte sequence 0x00000300) is taken into 
        //  account when searching the RBSP data for the next occurrence of byte-aligned bits with the binary patterns 
        //  specified above.
        //
        if (_inNalUnit && _zeroCount == 2 && _curValue == 3) 
        {
            ReadBytes(1, &_curValue);
            _zeroCount = 0;
        }
        if (_curValue == 0)
        {
            _zeroCount = (_zeroCount + 1) % 3;
        }
        else
        {
            _zeroCount = 0;
        }
        _curBitPos = 0;
    }
}

} // namespace Codec
} // namespace Mmp