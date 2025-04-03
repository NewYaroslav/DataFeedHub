#pragma once
#ifndef _DFH_UTILS_SSE_DOUBLE_INT64_UTILS_HPP_INCLUDED
#define _DFH_UTILS_SSE_DOUBLE_INT64_UTILS_HPP_INCLUDED

/// \file sse_double_int64_utils.hpp
/// \brief src: https://stackoverflow.com/questions/41144668/how-to-efficiently-perform-double-int64-conversions-with-sse-avx

#include <emmintrin.h>

namespace dfh::utils {

    /// \brief Converts a __m128d of doubles to a __m128i of uint64_t.
    /// \details Only works for inputs in the range [0, 2^52).
    /// \param x A __m128d containing double values.
    /// \return A __m128i containing the converted uint64_t values.
    inline __m128i double_to_uint64(__m128d x) {
        x = _mm_add_pd(x, _mm_set1_pd(0x0010000000000000));
        return _mm_xor_si128(
            _mm_castpd_si128(x),
            _mm_castpd_si128(_mm_set1_pd(0x0010000000000000))
        );
    }

    /// \brief Converts a __m128d of doubles to a __m128i of int64_t.
    /// \details Only works for inputs in the range [-2^51, 2^51].
    /// \param x A __m128d containing double values.
    /// \return A __m128i containing the converted int64_t values.
    inline __m128i double_to_int64(__m128d x) {
        x = _mm_add_pd(x, _mm_set1_pd(0x0018000000000000));
        return _mm_sub_epi64(
            _mm_castpd_si128(x),
            _mm_castpd_si128(_mm_set1_pd(0x0018000000000000))
        );
    }

    /// \brief Converts a __m128i of uint64_t to a __m128d of doubles.
    /// \details Only works for inputs in the range [0, 2^52).
    /// \param x A __m128i containing uint64_t values.
    /// \return A __m128d containing the converted double values.
    inline __m128d uint64_to_double(__m128i x) {
        x = _mm_or_si128(x, _mm_castpd_si128(_mm_set1_pd(0x0010000000000000)));
        return _mm_sub_pd(_mm_castsi128_pd(x), _mm_set1_pd(0x0010000000000000));
    }

    /// \brief Converts a __m128i of int64_t to a __m128d of doubles.
    /// \details Only works for inputs in the range [-2^51, 2^51].
    /// \param x A __m128i containing int64_t values.
    /// \return A __m128d containing the converted double values.
    inline __m128d int64_to_double(__m128i x) {
        x = _mm_add_epi64(x, _mm_castpd_si128(_mm_set1_pd(0x0018000000000000)));
        return _mm_sub_pd(_mm_castsi128_pd(x), _mm_set1_pd(0x0018000000000000));
    }
}

#endif // _DFH_UTILS_SSE_DOUBLE_INT64_UTILS_HPP_INCLUDED
