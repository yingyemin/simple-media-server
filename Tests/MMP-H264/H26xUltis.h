//
// H26xUltis.h
//
// Library: Codec
// Package: H26x
// Module:  H26x
// 

#pragma once

#include <iostream>
#include <cassert>

#include "H264Common.h"

#include "H265Common.h"

#ifdef MMP_H26x_EXTERN_HEADER
#include MMP_H26x_EXTERN_HEADER
#endif /* MMP_H26x_EXTERN_HEADER */

#ifndef H26x_LOG_TERMINATOR
#define H26x_LOG_TERMINATOR std::endl
#endif 

#ifndef H26x_LOG_INFO
#define H26x_LOG_INFO std::cout 
#endif

#ifndef H26x_LOG_ERROR
#define H26x_LOG_ERROR std::cerr
#endif /* H26x_LOG_ERROR */

namespace Mmp
{
namespace Codec
{

#ifdef MMP_H26X_DEBUG_MODE
#define MPP_H26X_SYNTAXT_STRICT_CHECK(cond, msg, exp)               if (!(cond))\
                                                                    {\
                                                                        H26x_LOG_ERROR << "[H26X] " << "[" << __FILE__ << ":" << __LINE__ << "]" << msg << H26x_LOG_TERMINATOR;\
                                                                        assert(false);\
                                                                        exp;\
                                                                    }
#define MPP_H26X_SYNTAXT_NORMAL_CHECK(cond, msg, exp)               if (!(cond))\
                                                                    {\
                                                                        H26x_LOG_INFO << "[H26X] " << "[" << __FILE__ << ":" << __LINE__ << "]" << msg << H26x_LOG_TERMINATOR;\
                                                                        exp;\
                                                                    }
#else
#define MPP_H26X_SYNTAXT_STRICT_CHECK(cond, msg, exp)               if (!(cond))\
                                                                    {\
                                                                        exp;\
                                                                    }
#define MPP_H26X_SYNTAXT_NORMAL_CHECK(cond, msg, exp)               if (!(cond))\
                                                                    {\
                                                                        exp;\
                                                                    }
#endif /* MMP_H26X_DEBUG_MODE */

std::string H264NaluTypeToStr(uint8_t nal_unit_type);

std::string H264SliceTypeToStr(uint8_t slice_type);

void FillH264SpsContext(H264SpsSyntax::ptr sps);

/**
 * @sa ITU-T H.265 (2021) - 7.4.3.2 Sequence parameter set RBSP semantics
 */
void FillH265SpsContext(H265SpsSyntax::ptr sps);


} // namespace Codec
} // namespace Mmp