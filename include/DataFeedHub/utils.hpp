#pragma once
#ifndef _DFH_UTILS_HPP_INCLUDED
#define _DFH_UTILS_HPP_INCLUDED

/// \file utils.hpp
/// \brief Aggregated include file for all utility modules used in DataFeedHub.
///
/// This header includes various low-level utility components such as symbol parsing,
/// memory alignment, SIMD optimizations, fixed-point math, compression, string helpers,
/// and exchange-specific parsers.

//------------------------------------------------------------------------------
// Utility domain
//------------------------------------------------------------------------------

#include "utils/DynamicBitset.hpp"
#include "utils/aligned_allocator.hpp"
#include "utils/binance_parser.hpp"
#include "utils/bybit_parser.hpp"
#include "utils/enum_utils.hpp"
#include "utils/fixed_point.hpp"
#include "utils/math_utils.hpp"
#include "utils/simdcomp.hpp"
#include "utils/sse_double_int64_utils.hpp"
#include "utils/string_utils.hpp"
#include "utils/symbol_key_utils.hpp"
#include "utils/vbyte.hpp"
#include "utils/zip_utils.hpp"

#endif // _DFH_UTILS_HPP_INCLUDED
