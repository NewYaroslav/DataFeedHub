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
// Standard headers
//------------------------------------------------------------------------------

#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <list>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>
#include <vector>

//------------------------------------------------------------------------------
// Third-party libraries
//------------------------------------------------------------------------------

#include <nlohmann/json.hpp>
#include <simdcomp.h>
#include <vbyte.h>
#include <fast_double_parser.h>
#include <time_shield.hpp>


//------------------------------------------------------------------------------
// Internal data structures
//------------------------------------------------------------------------------

#include <DataFeedHub/data/common.hpp>
#include <DataFeedHub/data/ticks.hpp>

//------------------------------------------------------------------------------
// Utility domain
//------------------------------------------------------------------------------

#include <DataFeedHub/utils/DynamicBitset.hpp>
#include <DataFeedHub/utils/aligned_allocator.hpp>
#include <DataFeedHub/utils/binance_parser.hpp>
#include <DataFeedHub/utils/bybit_parser.hpp>
#include <DataFeedHub/utils/enum_utils.hpp>
#include <DataFeedHub/utils/fixed_point.hpp>
#include <DataFeedHub/utils/math_utils.hpp>
#include <DataFeedHub/utils/simdcomp.hpp>
#include <DataFeedHub/utils/sse_double_int64_utils.hpp>
#include <DataFeedHub/utils/string_utils.hpp>
#include <DataFeedHub/utils/symbol_key_utils.hpp>
#include <DataFeedHub/utils/vbyte.hpp>
#include <DataFeedHub/utils/zip_utils.hpp>

#endif // _DFH_UTILS_HPP_INCLUDED
