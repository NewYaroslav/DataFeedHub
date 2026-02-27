#pragma once
#ifndef _DFH_COMPRESSION_UTILS_ZIG_ZAG_DELTA_PRICE_HPP_INCLUDED
#define _DFH_COMPRESSION_UTILS_ZIG_ZAG_DELTA_PRICE_HPP_INCLUDED

/// \file zig_zag_delta_price.hpp
/// \brief Price delta ZigZag codecs extracted from zig_zag_delta.hpp.
///
/// \warning Do NOT include this header directly.
/// Include only via \c include/DataFeedHub/compression/utils/zig_zag_delta.hpp
/// (or umbrella \c include/DataFeedHub/compression/utils.hpp) to guarantee
/// that all required dependencies and helper declarations are available.

    /// \defgroup dfh_tick_price_delta_zigzag Price delta ZigZag codecs
    /// \ingroup dfh_compression
    /// \brief Helpers for encoding/decoding tick price fields as delta + ZigZag integers.
    ///
    /// These helpers convert a selected floating-point price field into a scaled integer series:
    /// `scaled = llround(price * price_scale)`, then store per-element deltas relative to the previous
    /// scaled value and ZigZag-encode signed deltas into unsigned integers.
    ///
    /// Supported output formats:
    /// - int32 deltas packed into uint32 (`encode_price_delta_zig_zag_u32` / `decode_price_delta_zig_zag_u32`)
    /// - int64 deltas packed into uint64 (`encode_price_delta_zig_zag_u64` / `decode_price_delta_zig_zag_u64`)
    ///
    /// The price field is selected at compile time via a pointer-to-member: `double TickType::*`.
    ///
    /// \par Example
    /// \code
    /// struct MarketTick {
    ///     double last = 0.0;
    ///     double bid  = 0.0;
    ///     double ask  = 0.0;
    /// };
    ///
    /// std::vector<MarketTick> ticks = {
    ///     {1.23450, 1.23440, 1.23460},
    ///     {1.23455, 1.23445, 1.23465},
    ///     {1.23460, 1.23450, 1.23470}
    /// };
    ///
    /// const double price_scale = 1e5; // 5 digits
    /// const std::int64_t init_last = std::llround(ticks[0].last * price_scale);
    ///
    /// std::vector<std::uint32_t> deltas(ticks.size());
    /// dfh::compression::encode_price_delta_zig_zag_u32<MarketTick, &MarketTick::last>(
    ///     ticks.data(), deltas.data(), ticks.size(), price_scale, init_last);
    ///
    /// std::vector<MarketTick> restored(ticks.size());
    /// dfh::compression::decode_price_delta_zig_zag_u32<MarketTick, &MarketTick::last>(
    ///     deltas.data(), restored.data(), restored.size(), price_scale, init_last);
    /// \endcode
    ///
    /// \note \p initial_price is an integer in the scaled domain (same units as `llround(price * price_scale)`).
    /// \note Decode functions write only the selected field; other fields in ticks are untouched.
    /// \note The int32 variant validates that each delta fits into int32 range.

namespace detail {

#   if defined(__SSE2__)
    static inline bool llround_like_2pd_to_2i32_sse2(__m128d x, __m128d scale, __m128i& out2) noexcept {
        __m128d v = _mm_mul_pd(x, scale);

        // abs(v)
        const __m128d sign_mask = _mm_set1_pd(-0.0);
        __m128d av = _mm_andnot_pd(sign_mask, v);

        // range check for int32 after rounding: |v| <= INT32_MAX + 0.5, NaN fails compare
        const __m128d maxv = _mm_set1_pd(2147483647.5);
        __m128d ok = _mm_cmple_pd(av, maxv);
        if ((_mm_movemask_pd(ok) & 0b11) != 0b11) return false;

        // y = v + copysign(0.5, v)
        __m128d half = _mm_set1_pd(0.5);
        __m128d sign = _mm_and_pd(v, sign_mask);
        __m128d signed_half = _mm_or_pd(half, sign);
        __m128d y = _mm_add_pd(v, signed_half);

        out2 = _mm_cvttpd_epi32(y); // 2 int32 in low lanes
        return true;
    }

    // overflow check for int32 subtraction: overflow if ((a^b)&(a^d)) has sign bit.
    static inline bool sub_overflow_i32_sse2(__m128i a, __m128i b, __m128i d) noexcept {
        __m128i t1 = _mm_xor_si128(a, b);
        __m128i t2 = _mm_xor_si128(a, d);
        __m128i ov = _mm_and_si128(t1, t2);
        __m128i sign = _mm_srai_epi32(ov, 31);
        return _mm_movemask_epi8(sign) != 0;
    }
    
    static inline __m128i zigzag_i32_sse2(__m128i d) noexcept {
        return _mm_xor_si128(_mm_slli_epi32(d, 1), _mm_srai_epi32(d, 31));
    }
#   endif

#if defined(__AVX2__)
    static inline bool llround_like_4pd_to_4i32_avx2(__m256d x, __m256d scale, __m128i& out4) noexcept {
        __m256d v = _mm256_mul_pd(x, scale);

        const __m256d sign_mask = _mm256_set1_pd(-0.0);
        __m256d av = _mm256_andnot_pd(sign_mask, v);

        const __m256d maxv = _mm256_set1_pd(2147483647.5);
        __m256d ok = _mm256_cmp_pd(av, maxv, _CMP_LE_OQ);
        if (_mm256_movemask_pd(ok) != 0b1111) return false;

        __m256d half = _mm256_set1_pd(0.5);
        __m256d sign = _mm256_and_pd(v, sign_mask);
        __m256d signed_half = _mm256_or_pd(half, sign);
        __m256d y = _mm256_add_pd(v, signed_half);

        out4 = _mm256_cvttpd_epi32(y); // 4x i32
        return true;
    }

    static inline bool sub_overflow_i32_avx2(__m256i a, __m256i b, __m256i d) noexcept {
        __m256i t1 = _mm256_xor_si256(a, b);
        __m256i t2 = _mm256_xor_si256(a, d);
        __m256i ov = _mm256_and_si256(t1, t2);
        __m256i sign = _mm256_srai_epi32(ov, 31);
        return _mm256_movemask_epi8(sign) != 0;
    }

    static inline __m256i zigzag_i32_avx2(__m256i d) noexcept {
        return _mm256_xor_si256(_mm256_slli_epi32(d, 1), _mm256_srai_epi32(d, 31));
    }
#   endif

#if defined(__AVX512F__)
    static inline bool llround_like_8pd_to_8i32_avx512(__m512d x, double scale, __m256i& out8) noexcept {
        __m512d v = _mm512_mul_pd(x, _mm512_set1_pd(scale));

        // abs(v)
        const __m512d sign_mask = _mm512_set1_pd(-0.0);
        __m512d av = _mm512_andnot_pd(sign_mask, v);

        // mask: |v| <= INT32_MAX + 0.5 and ordered
        const __m512d maxv = _mm512_set1_pd(2147483647.5);
        __mmask8 ok = _mm512_cmp_pd_mask(av, maxv, _CMP_LE_OQ);
        if (ok != 0xFF) return false;

        // y = v + copysign(0.5, v)
        __m512d half = _mm512_set1_pd(0.5);
        __m512d sign = _mm512_and_pd(v, sign_mask);
        __m512d signed_half = _mm512_or_pd(half, sign);
        __m512d y = _mm512_add_pd(v, signed_half);

        // trunc -> i32
        out8 = _mm512_cvttpd_epi32(y); // returns __m256i with 8 int32
        return true;
    }
#   endif

} // namespace dfh::compression::detail

    /// \brief Scalar price-delta + ZigZag encoder to 32-bit code units.
    /// \tparam TickType Tick DTO type containing the price member.
    /// \tparam PriceMember Pointer to member with source price (`double`).
    /// \param ticks Pointer to input ticks.
    /// \param output Pointer to output encoded values (`uint32_t`).
    /// \param size Number of ticks.
    /// \param price_scale Multiplier applied before rounding to integer price space.
    /// \param initial_price Initial scaled reference price.
    /// \return `true` if all rounded deltas fit in `int32_t`; otherwise `false`.
    /// \pre `ticks`/`output` point to arrays of at least `size` elements.
    /// \note Use `u64` variant when the scaled-delta domain may exceed `int32_t`.
    /// \thread_safety Thread-safe (pure function over caller-provided buffers).
    template<class TickType, double TickType::* PriceMember>
    bool encode_price_delta_zig_zag_scalar_u32(
            const TickType* ticks,
            std::uint32_t* output,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) noexcept {
        if (size == 0) return true;
        std::int64_t scaled_price; 
        for (std::size_t i = 0; i < size; ++i) {
            scaled_price = static_cast<std::int64_t>(
                std::llround((ticks[i].*PriceMember) * price_scale)
            );
            if (!diff_fits_i32(scaled_price, initial_price)) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(scaled_price - initial_price));
            initial_price = scaled_price;
        }
        return true;
    }

#   if defined(__SSE2__)
    template<class TickType, double TickType::* PriceMember>
    bool encode_price_delta_zig_zag_sse2_u32(
            const TickType* ticks,
            std::uint32_t* output,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) noexcept {
        if (size == 0) return true;

        // SIMD u32 fast path requires scaled in int32 domain (otherwise return false => caller may use u64 variant)
        std::int64_t prev64 = initial_price;
        if (prev64 < std::numeric_limits<std::int32_t>::min() ||
            prev64 > std::numeric_limits<std::int32_t>::max()) {
            return false;
        }
        std::int32_t prev = static_cast<std::int32_t>(prev64);

        const __m128d scale = _mm_set1_pd(price_scale);

        std::size_t i = 0;

        // Prologue: make i multiple of 4 so output store can be aligned if desired (you said output aligned-32 always).
        // Here output[0] is at aligned address; i=0 already aligned. We'll write aligned stores starting at i=0 anyway.
        double p0, p1, p2, p3;
        __m128i s01, s23, cur, prevv, d, zz;
        for (; i + 4 <= size; i += 4) {
            // Load 4 doubles (AoS: scalar loads, then pack)
            p0 = ticks[i + 0].*PriceMember;
            p1 = ticks[i + 1].*PriceMember;
            p2 = ticks[i + 2].*PriceMember;
            p3 = ticks[i + 3].*PriceMember;

            if (!detail::llround_like_2pd_to_2i32_sse2(_mm_setr_pd(p0, p1), scale, s01)) break;
            if (!detail::llround_like_2pd_to_2i32_sse2(_mm_setr_pd(p2, p3), scale, s23)) break;

            // Combine into [s0 s1 s2 s3] in one __m128i
            cur = _mm_unpacklo_epi64(s01, s23);

            // Build prev-vector = [prev, cur0, cur1, cur2]
            prevv = _mm_slli_si128(cur, 4);
            prevv = _mm_or_si128(prevv, _mm_cvtsi32_si128(prev));

            d = _mm_sub_epi32(cur, prevv);

            // delta must fit in int32: detect overflow of subtraction
            if (detail::sub_overflow_i32_sse2(cur, prevv, d)) return false;

            zz = detail::zigzag_i32_sse2(d);

            // aligned store is ok if output aligned and i multiple of 4 (it is)
            _mm_store_si128(reinterpret_cast<__m128i*>(output + i), zz);

            // update prev = cur3
            prev = _mm_cvtsi128_si32(_mm_shuffle_epi32(cur, _MM_SHUFFLE(3,3,3,3)));
        }

        // Scalar tail / fallback: exact llround + diff_fits_i32
        prev64 = static_cast<std::int64_t>(prev);
        for (; i < size; ++i) {
            const std::int64_t cur64 = static_cast<std::int64_t>(
                std::llround((ticks[i].*PriceMember) * price_scale)
            );
            if (!diff_fits_i32(cur64, prev64)) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(cur64 - prev64));
            prev64 = cur64;
        }
        return true;
    }
#   endif

#if defined(__AVX2__)
    template<class TickType, double TickType::* PriceMember>
    bool encode_price_delta_zig_zag_avx2_u32(
            const TickType* ticks,
            std::uint32_t* output,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) noexcept {
        if (size == 0) return true;

        std::int64_t prev64 = initial_price;
        if (prev64 < std::numeric_limits<std::int32_t>::min() ||
            prev64 > std::numeric_limits<std::int32_t>::max()) {
            return false;
        }
        std::int32_t prev = static_cast<std::int32_t>(prev64);

        const __m256d scale = _mm256_set1_pd(price_scale);

        std::size_t i = 0;
        for (; i + 8 <= size; i += 8) {
            // Load 8 doubles scalar and pack into two __m256d
            const double p0 = ticks[i + 0].*PriceMember;
            const double p1 = ticks[i + 1].*PriceMember;
            const double p2 = ticks[i + 2].*PriceMember;
            const double p3 = ticks[i + 3].*PriceMember;
            const double p4 = ticks[i + 4].*PriceMember;
            const double p5 = ticks[i + 5].*PriceMember;
            const double p6 = ticks[i + 6].*PriceMember;
            const double p7 = ticks[i + 7].*PriceMember;

            __m128i s0, s1;
            if (!detail::llround_like_4pd_to_4i32_avx2(_mm256_setr_pd(p0,p1,p2,p3), scale, s0)) break;
            if (!detail::llround_like_4pd_to_4i32_avx2(_mm256_setr_pd(p4,p5,p6,p7), scale, s1)) break;

            __m256i cur = _mm256_castsi128_si256(s0);
            cur = _mm256_inserti128_si256(cur, s1, 1); // [c0..c7] as i32

            // prev vector: [prev, c0, c1, c2, c3, c4, c5, c6]
            const __m256i idx = _mm256_setr_epi32(0,0,1,2,3,4,5,6);
            __m256i prevv = _mm256_permutevar8x32_epi32(cur, idx);
            prevv = _mm256_blend_epi32(prevv, _mm256_set1_epi32(prev), 0x01); // lane0 = prev

            __m256i d = _mm256_sub_epi32(cur, prevv);
            if (detail::sub_overflow_i32_avx2(cur, prevv, d)) return false;

            __m256i zz = detail::zigzag_i32_avx2(d);

            // aligned store (output aligned-32 and i multiple of 8)
            _mm256_store_si256(reinterpret_cast<__m256i*>(output + i), zz);

            // update prev = c7
            prev = _mm256_extract_epi32(cur, 7);
        }

        // Scalar tail / fallback (exact semantics)
        prev64 = static_cast<std::int64_t>(prev);
        for (; i < size; ++i) {
            const std::int64_t cur64 = static_cast<std::int64_t>(
                std::llround((ticks[i].*PriceMember) * price_scale)
            );
            if (!diff_fits_i32(cur64, prev64)) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(cur64 - prev64));
            prev64 = cur64;
        }
        return true;
    }
#   endif

#   if defined(__AVX512F__)
    template<class TickType, double TickType::* PriceMember>
    bool encode_price_delta_zig_zag_avx512_u32(
            const TickType* ticks,
            std::uint32_t* output,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) noexcept {
        if (size == 0) return true;
        if (!ticks || !output) return true;

        std::int64_t prev64 = initial_price;
        if (prev64 < std::numeric_limits<std::int32_t>::min() ||
            prev64 > std::numeric_limits<std::int32_t>::max()) {
            return false;
        }
        std::int32_t prev = static_cast<std::int32_t>(prev64);

        std::size_t i = 0;
        for (; i + 8 <= size; i += 8) {
            // pack 8 doubles
            __m512d x = _mm512_setr_pd(
                ticks[i+0].*PriceMember, ticks[i+1].*PriceMember, ticks[i+2].*PriceMember, ticks[i+3].*PriceMember,
                ticks[i+4].*PriceMember, ticks[i+5].*PriceMember, ticks[i+6].*PriceMember, ticks[i+7].*PriceMember
            );

            __m256i cur;
            if (!detail::llround_like_8pd_to_8i32_avx512(x, price_scale, cur)) break;

            // Same AVX2 i32 pipeline for delta+zigzag:
            const __m256i idx = _mm256_setr_epi32(0,0,1,2,3,4,5,6);
            __m256i prevv = _mm256_permutevar8x32_epi32(cur, idx);
            prevv = _mm256_blend_epi32(prevv, _mm256_set1_epi32(prev), 0x01);

            __m256i d = _mm256_sub_epi32(cur, prevv);
            if (detail::sub_overflow_i32_avx2(cur, prevv, d)) return false;

            __m256i zz = detail::zigzag_i32_avx2(d);

            _mm256_store_si256(reinterpret_cast<__m256i*>(output + i), zz);
            prev = _mm256_extract_epi32(cur, 7);
        }

        // Scalar tail/fallback
        prev64 = static_cast<std::int64_t>(prev);
        for (; i < size; ++i) {
            const std::int64_t cur64 = static_cast<std::int64_t>(
                std::llround((ticks[i].*PriceMember) * price_scale)
            );
            if (!diff_fits_i32(cur64, prev64)) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(cur64 - prev64));
            prev64 = cur64;
        }
        return true;
    }
#   endif

    /// \ingroup dfh_tick_price_delta_zigzag
    /// \brief Encodes a selected price field as ZigZag-encoded int32 deltas stored in uint32.
    /// \details Returns \c false if a delta does not fit into int32 range. Intended for "try u32, fallback to u64".
    /// \tparam TickType Tick structure type.
    /// \tparam PriceMember Pointer-to-member selecting the price field (double TickType::*).
    /// \param ticks Pointer to the first tick.
    /// \param output Output buffer. Must contain at least \p size elements.
    /// \param size Number of ticks to encode.
    /// \param price_scale Scale factor, typically 10^digits.
    /// \param initial_price Initial scaled price value used as the delta base.
    /// \return \c true if all deltas fit in int32_t; otherwise \c false.
    template<class TickType, double TickType::* PriceMember>
    bool encode_price_delta_zig_zag_u32(
            const TickType* ticks,
            std::uint32_t* output,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) noexcept {
#       if defined(__AVX512F__)
        return encode_price_delta_zig_zag_avx512_u32<TickType, PriceMember>(ticks, output, size, price_scale, initial_price);
#       elif defined(__AVX2__)
        return encode_price_delta_zig_zag_avx2_u32<TickType, PriceMember>(ticks, output, size, price_scale, initial_price);
#       elif defined(__SSE2__)
        return encode_price_delta_zig_zag_sse2_u32<TickType, PriceMember>(ticks, output, size, price_scale, initial_price);
#       else
        return encode_price_delta_zig_zag_scalar_u32<TickType, PriceMember>(ticks, output, size, price_scale, initial_price);
#       endif
    }
    
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

    template<class TickType, double TickType::* PriceMember>
    void decode_price_delta_zig_zag_scalar_u32(
            const std::uint32_t* deltas,
            TickType* ticks,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) noexcept {
        const double inv_scale = 1.0 / price_scale;
        std::int64_t prev64 = initial_price;

        for (std::size_t i = 0; i < size; ++i) {
            prev64 += static_cast<std::int64_t>(zigzag_decode_u32(deltas[i]));
            ticks[i].*PriceMember = static_cast<double>(prev64) * inv_scale;
        }
    }
    
#   if defined(__SSE2__)
    template<class TickType, double TickType::* PriceMember>
    void decode_price_delta_zig_zag_sse2_u32(
            const std::uint32_t* deltas,
            TickType* ticks,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price) noexcept
    {
        const double inv_scale = 1.0 / price_scale;
        std::int64_t prev64 = initial_price;

        std::size_t i = 0;

        for (; i < size && reinterpret_cast<std::uintptr_t>(&deltas[i]) % 16 != 0; ++i) {
            prev64 += static_cast<std::int64_t>(zigzag_decode_u32(deltas[i]));
            ticks[i].*PriceMember = static_cast<double>(prev64) * inv_scale;
        }

        const std::size_t aligned_end = size - (size % 4);
        alignas(16) std::int32_t tmp[4];
        for (; i < aligned_end; i += 4) {
            __m128i cur = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&deltas[i]));
            __m128i decoded = zigzag_decode_u32_sse2(cur);
            __m128i delta = _mm_add_epi32(decoded, _mm_set1_epi32(static_cast<std::int32_t>(prev64)));
            _mm_store_si128(reinterpret_cast<__m128i*>(tmp), delta);
            for (std::size_t j = 0; j < 4; ++j) {
                ticks[i + j].*PriceMember = static_cast<double>(tmp[j]) * inv_scale;
            }
            prev64 = static_cast<std::int64_t>(tmp[3]);
        }

        for (; i < size; ++i) {
            prev64 += static_cast<std::int64_t>(zigzag_decode_u32(deltas[i]));
            ticks[i].*PriceMember = static_cast<double>(prev64) * inv_scale;
        }
    }
#   endif

#   if defined(__AVX2__)
    template<class TickType, double TickType::* PriceMember>
    void decode_price_delta_zig_zag_avx2_u32(
            const std::uint32_t* deltas,
            TickType* ticks,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) noexcept {
        const double inv_scale = 1.0 / price_scale;
        std::int64_t prev64 = initial_price;
        std::size_t i = 0;

        for (; i < size && reinterpret_cast<std::uintptr_t>(&deltas[i]) % 32 != 0; ++i) {
            prev64 += static_cast<std::int64_t>(zigzag_decode_u32(deltas[i]));
            ticks[i].*PriceMember = static_cast<double>(prev64) * inv_scale;
        }

        const std::size_t aligned_end = size - (size % 8);
        alignas(32) std::int32_t tmp[8];
        for (; i < aligned_end; i += 8) {
            __m256i cur = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&deltas[i]));
            __m256i decoded = zigzag_decode_u32_avx2(cur);
            __m256i delta = _mm256_add_epi32(decoded, _mm256_set1_epi32(static_cast<std::int32_t>(prev64)));
            _mm256_store_si256(reinterpret_cast<__m256i*>(tmp), delta);
            for (std::size_t j = 0; j < 8; ++j) {
                ticks[i + j].*PriceMember = static_cast<double>(tmp[j]) * inv_scale;
            }
            prev64 = static_cast<std::int64_t>(tmp[7]);
        }

        for (; i < size; ++i) {
            prev64 += static_cast<std::int64_t>(zigzag_decode_u32(deltas[i]));
            ticks[i].*PriceMember = static_cast<double>(prev64) * inv_scale;
        }
    }
#   endif

#   if defined(__AVX512F__)
    /// \brief AVX512 backend for u32 price-delta decode.
    /// \note Not selected by dispatcher; kept for micro-benchmarks / ISA studies.
    template<class TickType, double TickType::* PriceMember>
    void decode_price_delta_zig_zag_avx512_u32(
            const std::uint32_t* deltas,
            TickType* ticks,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) noexcept {
        const double inv_scale = 1.0 / price_scale;
        std::int64_t prev64 = initial_price;
        std::size_t i = 0;

        for (; i < size && reinterpret_cast<std::uintptr_t>(&deltas[i]) % 64 != 0; ++i) {
            prev64 += static_cast<std::int64_t>(zigzag_decode_u32(deltas[i]));
            ticks[i].*PriceMember = static_cast<double>(prev64) * inv_scale;
        }

        const std::size_t aligned_end = size - (size % 16);
        alignas(64) std::int32_t tmp[16];
        for (; i < aligned_end; i += 16) {
            __m512i cur = _mm512_loadu_si512(reinterpret_cast<const __m512i*>(&deltas[i]));
            __m512i decoded = zigzag_decode_u32_avx512(cur);
            __m512i delta = _mm512_add_epi32(decoded, _mm512_set1_epi32(static_cast<std::int32_t>(prev64)));
            _mm512_store_si512(reinterpret_cast<__m512i*>(tmp), delta);
            for (std::size_t j = 0; j < 16; ++j) {
                ticks[i + j].*PriceMember = static_cast<double>(tmp[j]) * inv_scale;
            }
            prev64 = static_cast<std::int64_t>(tmp[15]);
        }

        for (; i < size; ++i) {
            prev64 += static_cast<std::int64_t>(zigzag_decode_u32(deltas[i]));
            ticks[i].*PriceMember = static_cast<double>(prev64) * inv_scale;
        }
    }
#   endif


    /// \ingroup dfh_tick_price_delta_zigzag
    /// \brief Decodes ZigZag-encoded int32 deltas stored in uint32 into a selected price field.
    /// \tparam TickType Tick structure type.
    /// \tparam PriceMember Pointer-to-member selecting the price field (double TickType::*).
    /// \param deltas Input deltas array. Must contain \p size elements.
    /// \param ticks Output tick array. Must contain \p size elements. Only the selected field is written.
    /// \param size Number of elements to decode.
    /// \param price_scale Scale factor used in encoding, typically 10^digits.
    /// \param initial_price Initial scaled price value used to reconstruct the series.
    /// \note Writes only \p PriceMember in each tick; other fields remain unchanged.
    template<class TickType, double TickType::* PriceMember>
    void decode_price_delta_zig_zag_u32(
            const std::uint32_t* deltas,
            TickType* ticks,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) noexcept {
#       if defined(__AVX2__)
        decode_price_delta_zig_zag_avx2_u32<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
#       elif defined(__SSE2__)
        decode_price_delta_zig_zag_sse2_u32<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
#       else
        decode_price_delta_zig_zag_scalar_u32<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
#       endif
    }
    
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

    template<class TickType, double TickType::* PriceMember>
    void encode_price_delta_zig_zag_scalar_u64(
            const TickType* ticks,
            std::uint64_t* output,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) {
        if (size == 0) return;
        if (!ticks || !output) return;

        std::int64_t cur;
        for (std::size_t i = 0; i < size; ++i) {
            cur = static_cast<std::int64_t>(
                std::llround((ticks[i].*PriceMember) * price_scale)
            );
            if (!safe_diff_check_i64(cur, initial_price))
                throw std::overflow_error("encode_price_delta_zig_zag_u64: int64 delta overflow");
            output[i] = zigzag_encode_u64(cur - initial_price);
            initial_price = cur;
        }
    }
    
#   if defined(__AVX2__)
    template<class TickType, double TickType::* PriceMember>
    void encode_price_delta_zig_zag_avx2_u64(
            const TickType* ticks,
            std::uint64_t* output,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) {
        if (size == 0) return;
        if (!ticks || !output) return;

        std::int64_t prev = initial_price;
        const __m256d scale_v = _mm256_set1_pd(price_scale);
        const __m256d sign_mask_v = _mm256_set1_pd(-0.0);
        const __m256d half_v = _mm256_set1_pd(0.5);

        std::size_t i = 0;
        for (; i + 4 <= size; i += 4) {
            // Cheaper round+scale stage: llround-like SIMD path.
            const __m256d x = _mm256_setr_pd(
                ticks[i + 0].*PriceMember,
                ticks[i + 1].*PriceMember,
                ticks[i + 2].*PriceMember,
                ticks[i + 3].*PriceMember
            );
            const __m256d v = _mm256_mul_pd(x, scale_v);
            const __m256d sign = _mm256_and_pd(v, sign_mask_v);
            const __m256d signed_half = _mm256_or_pd(half_v, sign);
            const __m256d y = _mm256_add_pd(v, signed_half);

            alignas(32) double y_arr[4];
            _mm256_storeu_pd(y_arr, y);
            alignas(32) std::int64_t c[4] = {
                static_cast<std::int64_t>(y_arr[0]),
                static_cast<std::int64_t>(y_arr[1]),
                static_cast<std::int64_t>(y_arr[2]),
                static_cast<std::int64_t>(y_arr[3])
            };
            const __m256i cur = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(c));

            // Keep strict overflow checks identical to scalar semantics.
            if (!safe_diff_check_i64(c[0], prev )) throw std::overflow_error("encode_price_delta_zig_zag_u64: int64 delta overflow");
            if (!safe_diff_check_i64(c[1], c[0])) throw std::overflow_error("encode_price_delta_zig_zag_u64: int64 delta overflow");
            if (!safe_diff_check_i64(c[2], c[1])) throw std::overflow_error("encode_price_delta_zig_zag_u64: int64 delta overflow");
            if (!safe_diff_check_i64(c[3], c[2])) throw std::overflow_error("encode_price_delta_zig_zag_u64: int64 delta overflow");

            const __m256i prevv = _mm256_setr_epi64x(prev, c[0], c[1], c[2]);
            const __m256i d = _mm256_sub_epi64(cur, prevv);
            const __m256i sign_mask = _mm256_cmpgt_epi64(_mm256_setzero_si256(), d);
            const __m256i zz = _mm256_xor_si256(_mm256_slli_epi64(d, 1), sign_mask);

            _mm256_storeu_si256(reinterpret_cast<__m256i*>(output + i), zz);
            prev = c[3];
        }

        for (; i < size; ++i) {
            const std::int64_t cur = static_cast<std::int64_t>(
                std::llround((ticks[i].*PriceMember) * price_scale)
            );
            if (!safe_diff_check_i64(cur, prev))
                throw std::overflow_error("encode_price_delta_zig_zag_u64: int64 delta overflow");
            output[i] = zigzag_encode_u64(cur - prev);
            prev = cur;
        }
    }
#   endif

#   if defined(__AVX512DQ__) && defined(__AVX512F__)
    template<class TickType, double TickType::* PriceMember>
    void encode_price_delta_zig_zag_avx512_u64(
            const TickType* ticks,
            std::uint64_t* output,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price)
    {
        if (size == 0) return;
        if (!ticks || !output) return;

        std::int64_t prev = initial_price;

        std::size_t i = 0;
        for (; i + 8 <= size; i += 8) {
            __m512d x = _mm512_setr_pd(
                ticks[i+0].*PriceMember, ticks[i+1].*PriceMember, ticks[i+2].*PriceMember, ticks[i+3].*PriceMember,
                ticks[i+4].*PriceMember, ticks[i+5].*PriceMember, ticks[i+6].*PriceMember, ticks[i+7].*PriceMember
            );

            __m512d v = _mm512_mul_pd(x, _mm512_set1_pd(price_scale));

            // llround-like: trunc(v + copysign(0.5, v))
            const __m512d sign_mask = _mm512_set1_pd(-0.0);
            __m512d half = _mm512_set1_pd(0.5);
            __m512d sign = _mm512_and_pd(v, sign_mask);
            __m512d signed_half = _mm512_or_pd(half, sign);
            __m512d y = _mm512_add_pd(v, signed_half);

            // Convert trunc(y) -> i64
            // cvt with trunc: cvttpd_epi64 exists in AVX512DQ
            __m512i cur = _mm512_cvttpd_epi64(y);

            // Build prev vector: [prev, c0, c1, ... c6]
            __m512i prevv = _mm512_alignr_epi64(cur, cur, 7); // rotates; weвЂ™ll overwrite lane0 anyway
            // after alignr, lane0 contains c7; fix by shifting:
            // simpler: make index permute
            const __m512i idx = _mm512_setr_epi64(0,0,1,2,3,4,5,6);
            prevv = _mm512_permutexvar_epi64(idx, cur);
            prevv = _mm512_mask_mov_epi64(prevv, 0x01, _mm512_set1_epi64(prev)); // lane0 = prev

            __m512i d = _mm512_sub_epi64(cur, prevv);

            // safe_diff_check_i64 for each lane? full SIMD overflow check is bulky.
            // Here: follow your contract: throw if intermediate subtraction overflows int64.
            // In practice prices wonвЂ™t overflow int64; we do scalar check per element cheaply.
            alignas(64) std::int64_t c[8];
            _mm512_store_si512(reinterpret_cast<void*>(c), cur);

            if (!safe_diff_check_i64(c[0], prev)) throw std::overflow_error("encode_price_delta_zig_zag_u64: int64 delta overflow");
            for (int k = 1; k < 8; ++k) {
                if (!safe_diff_check_i64(c[k], c[k-1])) throw std::overflow_error("encode_price_delta_zig_zag_u64: int64 delta overflow");
            }

            // ZigZag: (d<<1) ^ (d>>63)
            __m512i signm = _mm512_cmpgt_epi64_mask(_mm512_setzero_si512(), d) ? _mm512_set1_epi64(-1) : _mm512_setzero_si512();
            // better: generate sign per lane:
            __mmask8 neg = _mm512_cmplt_epi64_mask(d, _mm512_setzero_si512());
            __m512i signv = _mm512_mask_set1_epi64(_mm512_setzero_si512(), neg, -1);

            __m512i zz = _mm512_xor_si512(_mm512_slli_epi64(d, 1), signv);

            _mm512_store_si512(reinterpret_cast<void*>(output + i), zz);

            prev = c[7];
        }

        for (; i < size; ++i) {
            const std::int64_t cur = static_cast<std::int64_t>(
                std::llround((ticks[i].*PriceMember) * price_scale)
            );
            if (!safe_diff_check_i64(cur, prev))
                throw std::overflow_error("encode_price_delta_zig_zag_u64: int64 delta overflow");
            output[i] = zigzag_encode_u64(cur - prev);
            prev = cur;
        }
    }
#   endif

    /// \ingroup dfh_tick_price_delta_zigzag
    /// \brief Encodes a selected price field as ZigZag-encoded int64 deltas stored in uint64.
    /// \details Throws if an intermediate subtraction would overflow int64_t.
    /// \tparam TickType Tick structure type.
    /// \tparam PriceMember Pointer-to-member selecting the price field (double TickType::*).
    /// \param ticks Pointer to the first tick.
    /// \param output Output buffer. Must contain at least \p size elements.
    /// \param size Number of ticks to encode.
    /// \param price_scale Scale factor, typically 10^digits.
    /// \param initial_price Initial scaled price value used as the delta base.
    /// \throw std::overflow_error If (scaled_price - initial_price) would overflow int64_t.
    template<class TickType, double TickType::* PriceMember>
    void encode_price_delta_zig_zag_u64(
            const TickType* ticks,
            std::uint64_t* output,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) {
#       if defined(__AVX512DQ__) && defined(__AVX512F__)
        encode_price_delta_zig_zag_avx512_u64<TickType, PriceMember>(ticks, output, size, price_scale, initial_price);
#       elif defined(__AVX2__)
        encode_price_delta_zig_zag_avx2_u64<TickType, PriceMember>(ticks, output, size, price_scale, initial_price);
#       else
        // SSE2: no good vector ZigZag(i64); keep scalar
        encode_price_delta_zig_zag_scalar_u64<TickType, PriceMember>(ticks, output, size, price_scale, initial_price);
#       endif
    }

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

    template<class TickType, double TickType::* PriceMember>
    /// \brief Decodes an encoded delta stream into destination ticks (backend helper).
    /// \note Function: \c decode_price_delta_zig_zag_scalar_u64.
    /// \thread_safety Thread-safe (no shared mutable state).


    inline void decode_price_delta_zig_zag_scalar_u64(
            const std::uint64_t* deltas,
            TickType* ticks,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) noexcept {
        if (size == 0) return;
        const double inv_scale = 1.0 / price_scale;
        std::int64_t cur = initial_price;
        for (std::size_t i = 0; i < size; ++i) {
            cur += zigzag_decode_u64_scalar(deltas[i]);
            ticks[i].*PriceMember = static_cast<double>(cur) * inv_scale;
        }
    }
    
#   if defined(__SSE2__)
    template<class TickType, double TickType::* PriceMember>
    /// \brief Decodes an encoded delta stream into destination ticks (backend helper).
    /// \note Function: \c decode_price_delta_zig_zag_sse2_u64.
    /// \thread_safety Thread-safe (no shared mutable state).


    inline void decode_price_delta_zig_zag_sse2_u64(
            const std::uint64_t* deltas,
            TickType* ticks,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) noexcept {
        if (size == 0) return;
        const double inv_scale = 1.0 / price_scale;
        std::int64_t cur = initial_price;
        std::size_t i = 0;

        // Align for aligned loads (16 bytes).
        constexpr std::size_t align = 16;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(deltas + i) % align) == 0) break;
            cur += zigzag_decode_u64_scalar(deltas[i]);
            ticks[i].*PriceMember = static_cast<double>(cur) * inv_scale;
        }

        constexpr std::size_t simd_width = 2;
        const std::size_t simd_end = i + ((size - i) / simd_width) * simd_width;

        alignas(16) std::int64_t prefix[simd_width];

        for (; i < simd_end; i += simd_width) {
            const __m128i z = _mm_load_si128(reinterpret_cast<const __m128i*>(deltas + i));
            __m128i d = zigzag_decode_u64_sse2(z);

            // Prefix sum within 128: [a, b] -> [a, a+b]
            __m128i t = d;
            t = _mm_add_epi64(t, _mm_slli_si128(t, 8));

            _mm_store_si128(reinterpret_cast<__m128i*>(prefix), t);

            // AoS store: scalar loop (РєР°Рє Сѓ С‚РµР±СЏ РІ РїСЂРёРјРµСЂРµ)
            ticks[i + 0].*PriceMember = static_cast<double>(cur + prefix[0]) * inv_scale;
            ticks[i + 1].*PriceMember = static_cast<double>(cur + prefix[1]) * inv_scale;

            cur += prefix[simd_width - 1];
        }

        for (; i < size; ++i) {
            cur += zigzag_decode_u64_scalar(deltas[i]);
            ticks[i].*PriceMember = static_cast<double>(cur) * inv_scale;
        }
    }
#   endif

#   if defined(__AVX2__)
    template<class TickType, double TickType::* PriceMember>
    /// \brief Decodes an encoded delta stream into destination ticks (backend helper).
    /// \note Function: \c decode_price_delta_zig_zag_avx2_u64.
    /// \note Not selected by dispatchers when SSE2 is also available; kept for micro-benchmarks.
    /// \thread_safety Thread-safe (no shared mutable state).


    inline void decode_price_delta_zig_zag_avx2_u64(
            const std::uint64_t* deltas,
            TickType* ticks,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price) noexcept {
        if (size == 0) return;
        const double inv_scale = 1.0 / price_scale;
        std::int64_t cur = initial_price;
        std::size_t i = 0;

        constexpr std::size_t simd_width = 4;
        const std::size_t simd_end = (size / simd_width) * simd_width;

        alignas(32) std::int64_t prefix[simd_width];

        for (; i < simd_end; i += simd_width) {
            const __m256i z = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(deltas + i));
            const __m256i d = zigzag_decode_u64_avx2(z);

            // Prefix sum inside each 128-bit lane: [a,b] | [c,d] -> [a,a+b] | [c,c+d]
            __m256i t = _mm256_add_epi64(d, _mm256_slli_si256(d, 8));

            // Fully vector carry propagation across 128-bit lanes: add low-lane last to high lane.
            const __m256i carry_all = _mm256_permute4x64_epi64(t, 0x55); // broadcast element1
            const __m256i carry_hi = _mm256_blend_epi32(_mm256_setzero_si256(), carry_all, 0xF0);
            t = _mm256_add_epi64(t, carry_hi); // [a, a+b, a+b+c, a+b+c+d]

            _mm256_storeu_si256(reinterpret_cast<__m256i*>(prefix), t);

            ticks[i + 0].*PriceMember = static_cast<double>(cur + prefix[0]) * inv_scale;
            ticks[i + 1].*PriceMember = static_cast<double>(cur + prefix[1]) * inv_scale;
            ticks[i + 2].*PriceMember = static_cast<double>(cur + prefix[2]) * inv_scale;
            ticks[i + 3].*PriceMember = static_cast<double>(cur + prefix[3]) * inv_scale;

            cur += prefix[simd_width - 1];
        }

        for (; i < size; ++i) {
            cur += zigzag_decode_u64_scalar(deltas[i]);
            ticks[i].*PriceMember = static_cast<double>(cur) * inv_scale;
        }
    }
#   endif

#   if defined(__AVX512F__)
    template<class TickType, double TickType::* PriceMember>
    /// \brief Decodes an encoded delta stream into destination ticks (backend helper).
    /// \note Function: \c decode_price_delta_zig_zag_avx512_u64.
    /// \thread_safety Thread-safe (no shared mutable state).


    inline void decode_price_delta_zig_zag_avx512_u64(
            const std::uint64_t* deltas,
            TickType* ticks,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) noexcept {
        if (size == 0 || !deltas || !ticks) return;

        const double inv_scale = 1.0 / price_scale;
        std::int64_t cur = initial_price;

        std::size_t i = 0;

        // Prefer aligned load if possible; otherwise use unaligned (still fast on many CPUs).
        constexpr std::size_t align = 64;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(deltas + i) % align) == 0) break;
            cur += zigzag_decode_u64_scalar(deltas[i]);
            ticks[i].*PriceMember = static_cast<double>(cur) * inv_scale;
        }

        constexpr std::size_t simd_width = 8;
        const std::size_t simd_end = i + ((size - i) / simd_width) * simd_width;

        alignas(64) std::int64_t d[simd_width];
        alignas(64) std::int64_t prefix[simd_width];

        for (; i < simd_end; i += simd_width) {
            __m512i z;
            if ((reinterpret_cast<std::uintptr_t>(deltas + i) % align) == 0) {
                z = _mm512_load_si512(reinterpret_cast<const void*>(deltas + i));
            } else {
                z = _mm512_loadu_si512(reinterpret_cast<const void*>(deltas + i));
            }

            __m512i dv = zigzag_decode_u64_avx512(z);
            _mm512_store_si512(reinterpret_cast<void*>(d), dv);

            // Small scalar prefix for the block (8 elems)
            std::int64_t run = 0;
            for (std::size_t k = 0; k < simd_width; ++k) {
                run += d[k];
                prefix[k] = run;
            }

            // AoS store
            for (std::size_t k = 0; k < simd_width; ++k) {
                ticks[i + k].*PriceMember = static_cast<double>(cur + prefix[k]) * inv_scale;
            }

            cur += prefix[simd_width - 1];
        }

        for (; i < size; ++i) {
            cur += zigzag_decode_u64_scalar(deltas[i]);
            ticks[i].*PriceMember = static_cast<double>(cur) * inv_scale;
        }
    }
#   endif


    /// \ingroup dfh_tick_price_delta_zigzag
    /// \brief Decodes ZigZag-encoded int64 deltas stored in uint64 into a selected price field.
    /// \tparam TickType Tick structure type.
    /// \tparam PriceMember Pointer-to-member selecting the price field (double TickType::*).
    /// \param deltas Input deltas array. Must contain \p size elements.
    /// \param ticks Output tick array. Must contain \p size elements. Only the selected field is written.
    /// \param size Number of elements to decode.
    /// \param price_scale Scale factor used in encoding, typically 10^digits.
    /// \param initial_price Initial scaled price value used to reconstruct the series.
    /// \note Writes only \p PriceMember in each tick; other fields remain unchanged.
    template<class TickType, double TickType::* PriceMember>
    inline void decode_price_delta_zig_zag_u64(
            const std::uint64_t* deltas,
            TickType* ticks,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price) noexcept {
#       if defined(__AVX512F__) && defined(__SSE2__)
        // Runtime policy for u64 price decode in AoS write-heavy paths:
        // - small/medium blocks: SSE2 is typically better on current targets;
        // - larger blocks: AVX512 may amortize prefix/carry overhead.
        constexpr std::size_t U64_PRICE_DECODE_AVX512_THRESHOLD = dispatcher_policy::PRICE_U64_DECODE_AVX512_THRESHOLD;
        if (size >= U64_PRICE_DECODE_AVX512_THRESHOLD) {
            decode_price_delta_zig_zag_avx512_u64<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
        } else {
            decode_price_delta_zig_zag_sse2_u64<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
        }
#       elif defined(__AVX512F__)
        decode_price_delta_zig_zag_avx512_u64<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
#       elif defined(__SSE2__) && defined(__AVX2__)
        // Current microbenchmarks show SSE2 consistently outperforming AVX2 on u64 price decode
        // in AoS write-heavy paths. Keep AVX2 backend available for explicit benchmarking.
        decode_price_delta_zig_zag_sse2_u64<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
#       elif defined(__AVX2__)
        decode_price_delta_zig_zag_avx2_u64<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
#       elif defined(__SSE2__)
        decode_price_delta_zig_zag_sse2_u64<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
#       else
        decode_price_delta_zig_zag_scalar_u64<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
#       endif
    }

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------


#endif // _DFH_COMPRESSION_UTILS_ZIG_ZAG_DELTA_PRICE_HPP_INCLUDED
