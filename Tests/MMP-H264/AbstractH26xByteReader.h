//
// AbstractH26xByteReader.h
//
// Library: Codec
// Package: H26x
// Module:  H26x
// 

#pragma once

#include "H264Common.h"

namespace Mmp
{
namespace Codec
{

class AbstractH26xByteReader
{
public:
    using ptr = std::shared_ptr<AbstractH26xByteReader>;
public:
    AbstractH26xByteReader() = default;
    virtual ~AbstractH26xByteReader() = default;
public:
    /**
     * @brief     read raw data from the bytes stream
     * @param[out] data
     * @param[in]  bytes
     * @return     actual read bytes
     */
    virtual size_t Read(void* data, size_t bytes) = 0;
    /**
     * @brief     go to the bytes position of the bytes stream
     * @param[in] offset
     */
    virtual bool Seek(size_t offset) = 0;
    /**
     * @brief get current bytes postion of the bytes stream
     */
    virtual size_t Tell() = 0;
    /**
     * @brief reach the end of the stream
     */
    virtual bool Eof() = 0;
};

} // namespace Codec
} // namespace Mmp