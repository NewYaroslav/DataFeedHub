#pragma once
#ifndef _DFH_COMPRESSION_UTILS_HPP_INCLUDED
#define _DFH_COMPRESSION_UTILS_HPP_INCLUDED

/// \file utils.hpp
/// \brief Compression utility.

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <cmath>
#include <limits>
#include <vector>
#include <unordered_map>
#include <utility>
#include <algorithm>
#include <stdexcept>

#ifdef __SSE2__
#include <emmintrin.h>
#endif

#ifdef __SSE4_1__
#include <smmintrin.h>
#endif

#ifdef defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__) || defined(__AVX512DQ__)
#include <immintrin.h>
#endif

#include <zdict.h>
#include <zstd.h>

#include "utils/frequency_encoding.hpp"
#include "utils/repeat_encoding.hpp"
#include "utils/volume_scaling.hpp"
#include "utils/zig_zag.hpp"
#include "utils/zig_zag_delta.hpp"
#include "utils/zstd_utils.hpp"

#endif // _DFH_COMPRESSION_UTILS_HPP_INCLUDED
