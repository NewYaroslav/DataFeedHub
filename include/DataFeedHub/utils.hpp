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
#include <simdcomp/simdcomp.h>
#include <libvbyte/vbyte.h>
#include <fast_double_parser/fast_double_parser.h>
#include <time_shield_cpp/time_shield.hpp>


//------------------------------------------------------------------------------
// Internal data structures
//------------------------------------------------------------------------------

#include "data/common.hpp"
#include "data/ticks.hpp"

//------------------------------------------------------------------------------
// Utility domain
//------------------------------------------------------------------------------

#include "utils/aligned_allocator.hpp"
#include "utils/binance_parser.hpp"
#include "utils/bybit_parser.hpp"
#include "utils/fixed_point.hpp"
#include "utils/math_utils.hpp"
#include "utils/simdcomp.hpp"
#include "utils/sse_double_int64_utils.hpp"
#include "utils/string_utils.hpp"
#include "utils/symbol_key_utils.hpp"
#include "utils/vbyte.hpp"
#include "utils/zip_utils.hpp"

#endif // _DFH_UTILS_HPP_INCLUDED
