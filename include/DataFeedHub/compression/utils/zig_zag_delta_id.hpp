#pragma once
#ifndef _DFH_COMPRESSION_UTILS_ZIG_ZAG_DELTA_ID_HPP_INCLUDED
#define _DFH_COMPRESSION_UTILS_ZIG_ZAG_DELTA_ID_HPP_INCLUDED

/// \file zig_zag_delta_id.hpp
/// \brief Trade ID delta codecs extracted from zig_zag_delta.hpp.
///
/// \warning Do NOT include this header directly.
/// Include only via \c include/DataFeedHub/compression/utils/zig_zag_delta.hpp
/// (or umbrella \c include/DataFeedHub/compression/utils.hpp) to guarantee
/// that all required dependencies and helper declarations are available.

    /// \defgroup dfh_tick_id_delta Trade ID delta codecs
    /// \ingroup dfh_compression
    /// \brief Encoding/decoding of trade IDs using ZigZag deltas.

    /// \ingroup dfh_tick_id_delta
    /// \brief Tries to encode trade IDs as ZigZag-encoded int32 deltas stored in \c uint32_t.
    /// \details
    /// Stores deltas as: \c delta = (cur_id - prev_id - 1).
    /// This makes consecutive IDs (\c prev_id + 1) produce zero deltas, improving compression.
    /// The encoding works for any ID sequence (increasing, decreasing, repeating), but compression
    /// is typically best when IDs are close to consecutive.
    /// The function is intended for "try u32, fallback to u64": it returns \c false if a delta
    /// does not fit into \c int32_t.
    /// \tparam TickType Tick structure type. Must provide:
    /// - \c trade_id() returning an integer ID (expected to fit into \c int64_t),
    /// - \c set_trade_id(uint64_t) for decoding.
    /// \param ticks Pointer to input ticks.
    /// \param deltas Output buffer for encoded deltas. Must contain at least \p size elements.
    /// \param size Number of ticks to encode.
    /// \param initial_id Base ID used for the first delta.
    /// \return \c true if all deltas fit in \c int32_t; otherwise \c false.
    /// \throw std::invalid_argument If \p initial_id is out of range, or if an intermediate
    ///        signed 64-bit difference overflows.
    template<class TickType>
    bool encode_id_delta_u32(
            const TickType* ticks,
            std::uint32_t* deltas,
            std::size_t size,
            std::int64_t initial_id) {
        if (size == 0) return true;
        if (initial_id < 0 ||
            initial_id >= static_cast<std::int64_t>(TickType::max_trade_id())) {
            throw std::invalid_argument("encode_id_delta_u32: initial_id out of range");
        }

        const std::int64_t first_id = static_cast<std::int64_t>(ticks[0].trade_id());
        if (first_id <= initial_id) {
            throw std::invalid_argument("encode_id_delta_u32: ticks[0].trade_id() <= initial_id");
        }

        constexpr std::int64_t delta_offset = 1;
        std::int64_t cur_id, delta;
        for (std::size_t i = 0; i < size; ++i) {
            cur_id = static_cast<std::int64_t>(ticks[i].trade_id());
            if (!safe_diff_check_i64(cur_id, initial_id)) throw std::invalid_argument("encode_id_delta_u32: int64 diff overflow");
            delta = cur_id - delta_offset;
            if (!diff_fits_i32(delta, initial_id)) return false; // fallback to u64 path 
            delta -= initial_id;
            deltas[i] = zigzag_encode_u32(static_cast<std::int32_t>(delta));
            initial_id = cur_id;
        }
        return true;
    }
    
//------------------------------------------------------------------------------
// Writer policy: default uses TickType::set_trade_id(uint64_t)
// TradeTick specialization writes packed id_and_side preserving side bits.
//------------------------------------------------------------------------------

namespace detail {

    template<class TickType>
    struct trade_id_writer final {
        static inline void write(TickType& tick, std::uint64_t trade_id) noexcept {
            tick.set_trade_id(trade_id);
        }
    };

    template<>
    struct trade_id_writer<dfh::TradeTick> final {
        static inline void write(dfh::TradeTick& tick, std::uint64_t trade_id) noexcept {
            const std::uint64_t side_bits = tick.id_and_side & dfh::TradeTick::TRADE_SIDE_MASK;
            const std::uint64_t id_part =
                (trade_id & dfh::TradeTick::TRADE_ID_MASK) << dfh::TradeTick::TRADE_ID_SHIFT;
            tick.id_and_side = id_part | side_bits;
        }
    };
    
    template<class TickType>
    struct trade_id_reader final {
        static inline std::uint64_t read(const TickType& tick) noexcept {
            return tick.trade_id();
        }
    };

    template<>
    struct trade_id_reader<dfh::TradeTick> final {
        static inline std::uint64_t read(const dfh::TradeTick& tick) noexcept {
            return (tick.id_and_side >> dfh::TradeTick::TRADE_ID_SHIFT) & dfh::TradeTick::TRADE_ID_MASK;
        }
    };

    template<class TickType>
    static inline std::int64_t trade_id_i64(const TickType& tick) noexcept {
        return static_cast<std::int64_t>(trade_id_reader<TickType>::read(tick));
    }


    template<class TickType>
    static inline void write_trade_id_fast(TickType& tick, std::uint64_t trade_id) noexcept {
        if constexpr (std::is_same_v<TickType, dfh::TradeTick>) {
            const std::uint64_t side_bits = tick.id_and_side & dfh::TradeTick::TRADE_SIDE_MASK;
            const std::uint64_t id_part =
                (trade_id & dfh::TradeTick::TRADE_ID_MASK) << dfh::TradeTick::TRADE_ID_SHIFT;
            tick.id_and_side = id_part | side_bits;
        } else {
            tick.set_trade_id(trade_id);
        }
    }

} // namespace dfh::compression::detail
    
//------------------------------------------------------------------------------
// Scalar implementation for ZigZag decoding (no SIMD).
//------------------------------------------------------------------------------

    template<class TickType>
    /// \brief Decodes an encoded delta stream into destination ticks (backend helper).
    /// \note Function: \c decode_id_delta_scalar_u32.
    /// \thread_safety Thread-safe (no shared mutable state).


    inline void decode_id_delta_scalar_u32(
            const std::uint32_t* deltas,
            TickType* ticks,
            std::size_t size,
            std::int64_t initial_id
        ) noexcept {
        constexpr std::int64_t delta_offset = 1;
        std::int64_t cur = initial_id;

        for (std::size_t i = 0; i < size; ++i) {
            cur += static_cast<std::int64_t>(zigzag_decode_u32(deltas[i])) + delta_offset;
            detail::write_trade_id_fast(ticks[i], static_cast<std::uint64_t>(cur));
        }
    }
    
//------------------------------------------------------------------------------
// SSE2: SIMD ZigZag + SIMD prefix-sum of increments, then scalar expand to int64
// Notes:
//  - We keep cur in int64 (correct).
//  - We do NOT store SIMD results into uint64_t* (was a bug).
//  - We can align-load deltas after a scalar prologue.
//------------------------------------------------------------------------------

#   if defined(__SSE2__)
    template<class TickType>
    /// \brief Decodes an encoded delta stream into destination ticks (backend helper).
    /// \note Function: \c decode_id_delta_sse2_u32.
    /// \thread_safety Thread-safe (no shared mutable state).


    inline void decode_id_delta_sse2_u32(
            const std::uint32_t* deltas,
            TickType* ticks,
            std::size_t size,
            std::int64_t initial_id
        ) noexcept {
        constexpr std::int64_t delta_offset = 1;
        std::int64_t cur = initial_id;
        std::size_t i = 0;

        constexpr std::size_t simd_width = 4;
        const std::size_t simd_end = (size / simd_width) * simd_width;
        const __m128i ones = _mm_set1_epi32(static_cast<int>(delta_offset));

        for (; i < simd_end; i += simd_width) {
            const __m128i z = _mm_loadu_si128(reinterpret_cast<const __m128i*>(deltas + i));
            __m128i d = zigzag_decode_u32_sse2(z);
            d = _mm_add_epi32(d, ones);

            __m128i t = d;
            t = _mm_add_epi32(t, _mm_slli_si128(t, 4));
            t = _mm_add_epi32(t, _mm_slli_si128(t, 8));

            const std::int32_t p0 = _mm_cvtsi128_si32(t);
            const std::int32_t p1 = _mm_cvtsi128_si32(_mm_srli_si128(t, 4));
            const std::int32_t p2 = _mm_cvtsi128_si32(_mm_srli_si128(t, 8));
            const std::int32_t p3 = _mm_cvtsi128_si32(_mm_srli_si128(t, 12));

            const std::uint64_t id0 = static_cast<std::uint64_t>(cur + static_cast<std::int64_t>(p0));
            const std::uint64_t id1 = static_cast<std::uint64_t>(cur + static_cast<std::int64_t>(p1));
            const std::uint64_t id2 = static_cast<std::uint64_t>(cur + static_cast<std::int64_t>(p2));
            const std::uint64_t id3 = static_cast<std::uint64_t>(cur + static_cast<std::int64_t>(p3));

            detail::write_trade_id_fast(ticks[i + 0], id0);
            detail::write_trade_id_fast(ticks[i + 1], id1);
            detail::write_trade_id_fast(ticks[i + 2], id2);
            detail::write_trade_id_fast(ticks[i + 3], id3);

            cur += static_cast<std::int64_t>(p3);
        }

        for (; i < size; ++i) {
            cur += static_cast<std::int64_t>(zigzag_decode_u32(deltas[i])) + delta_offset;
            detail::write_trade_id_fast(ticks[i], static_cast<std::uint64_t>(cur));
        }
    }

#   endif

//------------------------------------------------------------------------------
// AVX2: same idea for 8 lanes
//------------------------------------------------------------------------------

#   if defined(__AVX2__)
    template<class TickType>
    /// \brief Decodes an encoded delta stream into destination ticks (backend helper).
    /// \note Function: \c decode_id_delta_avx2_u32.
    /// \thread_safety Thread-safe (no shared mutable state).


    inline void decode_id_delta_avx2_u32(
            const std::uint32_t* deltas,
            TickType* ticks,
            std::size_t size,
            std::int64_t initial_id
        ) noexcept {
        constexpr std::int64_t delta_offset = 1;
        std::int64_t cur = initial_id;
        std::size_t i = 0;

        constexpr std::size_t simd_width = 8;
        const std::size_t simd_end = (size / simd_width) * simd_width;

        alignas(32) std::int32_t prefix0[simd_width];
        alignas(32) std::int32_t prefix1[simd_width];
        const __m256i ones = _mm256_set1_epi32(static_cast<int>(delta_offset));

        auto prefix8 = [](__m256i d) noexcept {
            __m256i t = d;
            t = _mm256_add_epi32(t, _mm256_slli_si256(t, 4));
            t = _mm256_add_epi32(t, _mm256_slli_si256(t, 8));
            const __m128i low = _mm256_castsi256_si128(t);
            const std::int32_t low_last = _mm_extract_epi32(low, 3);
            const __m256i add_hi = _mm256_setr_epi32(0,0,0,0, low_last,low_last,low_last,low_last);
            return _mm256_add_epi32(t, add_hi);
        };

        for (; i + 2 * simd_width <= simd_end; i += 2 * simd_width) {
            __m256i d0 = zigzag_decode_u32_avx2(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(deltas + i)));
            __m256i d1 = zigzag_decode_u32_avx2(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(deltas + i + simd_width)));
            d0 = _mm256_add_epi32(d0, ones);
            d1 = _mm256_add_epi32(d1, ones);

            __m256i p0 = prefix8(d0);
            _mm256_store_si256(reinterpret_cast<__m256i*>(prefix0), p0);

            const std::int64_t base1 = cur + static_cast<std::int64_t>(prefix0[7]);
            __m256i p1 = prefix8(d1);
            _mm256_store_si256(reinterpret_cast<__m256i*>(prefix1), p1);

            detail::write_trade_id_fast(ticks[i + 0], static_cast<std::uint64_t>(cur + prefix0[0]));
            detail::write_trade_id_fast(ticks[i + 1], static_cast<std::uint64_t>(cur + prefix0[1]));
            detail::write_trade_id_fast(ticks[i + 2], static_cast<std::uint64_t>(cur + prefix0[2]));
            detail::write_trade_id_fast(ticks[i + 3], static_cast<std::uint64_t>(cur + prefix0[3]));
            detail::write_trade_id_fast(ticks[i + 4], static_cast<std::uint64_t>(cur + prefix0[4]));
            detail::write_trade_id_fast(ticks[i + 5], static_cast<std::uint64_t>(cur + prefix0[5]));
            detail::write_trade_id_fast(ticks[i + 6], static_cast<std::uint64_t>(cur + prefix0[6]));
            detail::write_trade_id_fast(ticks[i + 7], static_cast<std::uint64_t>(cur + prefix0[7]));

            detail::write_trade_id_fast(ticks[i + 8],  static_cast<std::uint64_t>(base1 + prefix1[0]));
            detail::write_trade_id_fast(ticks[i + 9],  static_cast<std::uint64_t>(base1 + prefix1[1]));
            detail::write_trade_id_fast(ticks[i + 10], static_cast<std::uint64_t>(base1 + prefix1[2]));
            detail::write_trade_id_fast(ticks[i + 11], static_cast<std::uint64_t>(base1 + prefix1[3]));
            detail::write_trade_id_fast(ticks[i + 12], static_cast<std::uint64_t>(base1 + prefix1[4]));
            detail::write_trade_id_fast(ticks[i + 13], static_cast<std::uint64_t>(base1 + prefix1[5]));
            detail::write_trade_id_fast(ticks[i + 14], static_cast<std::uint64_t>(base1 + prefix1[6]));
            detail::write_trade_id_fast(ticks[i + 15], static_cast<std::uint64_t>(base1 + prefix1[7]));

            cur = base1 + static_cast<std::int64_t>(prefix1[7]);
        }

        for (; i < simd_end; i += simd_width) {
            __m256i d = zigzag_decode_u32_avx2(_mm256_loadu_si256(reinterpret_cast<const __m256i*>(deltas + i)));
            d = _mm256_add_epi32(d, ones);
            __m256i t = prefix8(d);
            _mm256_store_si256(reinterpret_cast<__m256i*>(prefix0), t);

            detail::write_trade_id_fast(ticks[i + 0], static_cast<std::uint64_t>(cur + prefix0[0]));
            detail::write_trade_id_fast(ticks[i + 1], static_cast<std::uint64_t>(cur + prefix0[1]));
            detail::write_trade_id_fast(ticks[i + 2], static_cast<std::uint64_t>(cur + prefix0[2]));
            detail::write_trade_id_fast(ticks[i + 3], static_cast<std::uint64_t>(cur + prefix0[3]));
            detail::write_trade_id_fast(ticks[i + 4], static_cast<std::uint64_t>(cur + prefix0[4]));
            detail::write_trade_id_fast(ticks[i + 5], static_cast<std::uint64_t>(cur + prefix0[5]));
            detail::write_trade_id_fast(ticks[i + 6], static_cast<std::uint64_t>(cur + prefix0[6]));
            detail::write_trade_id_fast(ticks[i + 7], static_cast<std::uint64_t>(cur + prefix0[7]));

            cur += static_cast<std::int64_t>(prefix0[7]);
        }

        for (; i < size; ++i) {
            cur += static_cast<std::int64_t>(zigzag_decode_u32(deltas[i])) + delta_offset;
            detail::write_trade_id_fast(ticks[i], static_cast<std::uint64_t>(cur));
        }
    }

#   endif

//------------------------------------------------------------------------------
// Dispatcher: chooses the best implementation based on available SIMD.
//------------------------------------------------------------------------------
        
    /// \ingroup dfh_tick_id_delta
    /// \brief Decodes ZigZag-encoded int32 deltas stored in \c uint32_t back into trade IDs.
    /// \details
    /// Restores IDs using: \c cur_id = prev_id + delta + 1, where \c delta is ZigZag-decoded.
    /// \tparam TickType Tick structure type. Must provide \c set_trade_id(uint64_t).
    /// \param deltas Input deltas array. Must contain \p size elements.
    /// \param ticks Output tick array. Must contain \p size elements. Only trade ID is written.
    /// \param size Number of elements to decode.
    /// \param initial_id Base ID used for the first reconstruction.
    /// \note No range validation is performed; caller must ensure input is valid.
    template<class TickType>
    inline void decode_id_delta_u32(
            const std::uint32_t* deltas,
            TickType* ticks,
            std::size_t size,
            std::int64_t initial_id) noexcept {
        if (size == 0) return;
#       if defined(__SSE2__) && defined(__AVX2__)
        // Runtime threshold policy for u32 ID decode:
        // - TradeTick write-path tends to be writer-bound: prefer SSE2.
        // - Plain DTOs may benefit from AVX2 only on very large blocks.
        constexpr std::size_t U32_AVX2_THRESHOLD = dispatcher_policy::ID_U32_AVX2_THRESHOLD;
        if constexpr (std::is_same_v<TickType, dfh::TradeTick>) {
            decode_id_delta_sse2_u32(deltas, ticks, size, initial_id);
        } else if (size >= U32_AVX2_THRESHOLD) {
            decode_id_delta_avx2_u32(deltas, ticks, size, initial_id);
        } else {
            decode_id_delta_sse2_u32(deltas, ticks, size, initial_id);
        }
#       elif defined(__SSE2__)
        decode_id_delta_sse2_u32(deltas, ticks, size, initial_id);
#       elif defined(__AVX2__)
        decode_id_delta_avx2_u32(deltas, ticks, size, initial_id);
#       else
        decode_id_delta_scalar_u32(deltas, ticks, size, initial_id);
#       endif
    }

    /// \ingroup dfh_tick_id_delta
    /// \brief Encodes trade IDs as ZigZag-encoded int64 deltas stored in \c uint64_t.
    /// \details
    /// Stores deltas as: \c delta = (cur_id - prev_id - 1).
    /// This makes consecutive IDs (prev_id + 1) produce zero deltas, improving compression.
    /// \tparam TickType Tick structure type. Must provide \c trade_id().
    /// \param ticks Pointer to input ticks.
    /// \param deltas Output buffer for encoded deltas. Must contain at least \p size elements.
    /// \param size Number of ticks to encode.
    /// \param initial_id Base ID used for the first delta.
    /// \throw std::invalid_argument If \p initial_id is out of range or if IDs are not strictly increasing.
    /// \note This encoder assumes \c trade_id fits into signed 64-bit and is strictly increasing.
    template<class TickType>
    void encode_id_delta_u64(
            const TickType* ticks,
            std::uint64_t* deltas,
            std::size_t size,
            std::int64_t initial_id) {
        if (size == 0) return;
        if (initial_id < 0 ||
            initial_id >= static_cast<std::int64_t>(TickType::max_trade_id())) {
            throw std::invalid_argument("encode_id_delta_u64: initial_id out of range");
        }

        const std::int64_t first_id = static_cast<std::int64_t>(ticks[0].trade_id());
        if (first_id <= initial_id) {
            throw std::invalid_argument("encode_id_delta_u64: ticks[0].trade_id() <= initial_id");
        }

        constexpr std::int64_t delta_offset = 1;
        std::int64_t cur_id, delta;
        for (std::size_t i = 0; i < size; ++i) {
            cur_id = static_cast<std::int64_t>(ticks[i].trade_id());
            if (!safe_diff_check_i64(cur_id, initial_id)) throw std::invalid_argument("encode_id_delta_u64: int64 diff overflow");
            delta = cur_id - delta_offset;
            if (!safe_diff_check_i64(delta, initial_id)) throw std::invalid_argument("encode_id_delta_u64: int64 diff overflow");
            delta -= initial_id;
            deltas[i] = zigzag_encode_u64(delta);
            initial_id = cur_id;
        }
    }
    
//------------------------------------------------------------------------------
// Scalar implementation for ZigZag decoding (no SIMD).
//------------------------------------------------------------------------------

    template<class TickType>
    /// \brief Decodes an encoded delta stream into destination ticks (backend helper).
    /// \note Function: \c decode_id_delta_scalar_u64.
    /// \thread_safety Thread-safe (no shared mutable state).


    inline void decode_id_delta_scalar_u64(
            const std::uint64_t* deltas,
            TickType* ticks,
            std::size_t size,
            std::int64_t initial_id
        ) noexcept {
        constexpr std::int64_t delta_offset = 1;
        std::int64_t cur = initial_id;

        for (std::size_t i = 0; i < size; ++i) {
            cur += static_cast<std::int64_t>(zigzag_decode_u64(deltas[i])) + delta_offset;
            detail::write_trade_id_fast(ticks[i], static_cast<std::uint64_t>(cur));
        }
    }
    
//------------------------------------------------------------------------------
// SSE2: 2 lanes of int64
//------------------------------------------------------------------------------

#   if defined(__SSE2__)
    template<class TickType>
    /// \brief Decodes an encoded delta stream into destination ticks (backend helper).
    /// \note Function: \c decode_id_delta_sse2_u64.
    /// \thread_safety Thread-safe (no shared mutable state).


    inline void decode_id_delta_sse2_u64(
            const std::uint64_t* deltas,
            TickType* ticks,
            std::size_t size,
            std::int64_t initial_id
        ) noexcept {
        constexpr std::int64_t delta_offset = 1;
        std::int64_t cur = initial_id;
        std::size_t i = 0;

        constexpr std::size_t simd_width = 2;
        const std::size_t simd_end = (size / simd_width) * simd_width;

        alignas(16) std::int64_t prefix[simd_width];
        const __m128i ones = _mm_set1_epi64x(delta_offset);

        for (; i < simd_end; i += simd_width) {
            const __m128i z = _mm_loadu_si128(reinterpret_cast<const __m128i*>(deltas + i));
            __m128i d = zigzag_decode_u64_sse2(z);
            d = _mm_add_epi64(d, ones);

            __m128i t = d;
            t = _mm_add_epi64(t, _mm_slli_si128(t, 8));
            _mm_store_si128(reinterpret_cast<__m128i*>(prefix), t);

            const std::uint64_t id0 = static_cast<std::uint64_t>(cur + prefix[0]);
            const std::uint64_t id1 = static_cast<std::uint64_t>(cur + prefix[1]);

            detail::write_trade_id_fast(ticks[i + 0], id0);
            detail::write_trade_id_fast(ticks[i + 1], id1);

            cur += prefix[1];
        }

        for (; i < size; ++i) {
            cur += static_cast<std::int64_t>(zigzag_decode_u64(deltas[i])) + delta_offset;
            detail::write_trade_id_fast(ticks[i], static_cast<std::uint64_t>(cur));
        }
    }

#   endif

//------------------------------------------------------------------------------
// AVX2: 4 lanes of int64
//------------------------------------------------------------------------------

#   if defined(__AVX2__)
    template<class TickType>
    /// \brief Decodes an encoded delta stream into destination ticks (backend helper).
    /// \note Function: \c decode_id_delta_avx2_u64.
    /// \thread_safety Thread-safe (no shared mutable state).


    inline void decode_id_delta_avx2_u64(
            const std::uint64_t* deltas,
            TickType* ticks,
            std::size_t size,
            std::int64_t initial_id
        ) noexcept {
        constexpr std::int64_t delta_offset = 1;
        std::int64_t cur = initial_id;
        std::size_t i = 0;

        constexpr std::size_t simd_width = 4;
        const std::size_t simd_end = (size / simd_width) * simd_width;

        alignas(32) std::int64_t prefix[simd_width];
        const __m256i ones = _mm256_set1_epi64x(delta_offset);

        for (; i < simd_end; i += simd_width) {
            const __m256i z = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(deltas + i));
            __m256i d = zigzag_decode_u64_avx2(z);
            d = _mm256_add_epi64(d, ones);

            __m256i t = d;
            t = _mm256_add_epi64(t, _mm256_slli_si256(t, 8));

            const __m128i low = _mm256_castsi256_si128(t);
            const std::int64_t low_last = _mm_cvtsi128_si64(_mm_srli_si128(low, 8));
            const __m256i add_hi = _mm256_setr_epi64x(0, 0, low_last, low_last);
            t = _mm256_add_epi64(t, add_hi);

            _mm256_store_si256(reinterpret_cast<__m256i*>(prefix), t);

            const std::uint64_t id0 = static_cast<std::uint64_t>(cur + prefix[0]);
            const std::uint64_t id1 = static_cast<std::uint64_t>(cur + prefix[1]);
            const std::uint64_t id2 = static_cast<std::uint64_t>(cur + prefix[2]);
            const std::uint64_t id3 = static_cast<std::uint64_t>(cur + prefix[3]);

            detail::write_trade_id_fast(ticks[i + 0], id0);
            detail::write_trade_id_fast(ticks[i + 1], id1);
            detail::write_trade_id_fast(ticks[i + 2], id2);
            detail::write_trade_id_fast(ticks[i + 3], id3);

            cur += prefix[simd_width - 1];
        }

        for (; i < size; ++i) {
            cur += static_cast<std::int64_t>(zigzag_decode_u64(deltas[i])) + delta_offset;
            detail::write_trade_id_fast(ticks[i], static_cast<std::uint64_t>(cur));
        }
    }

#   endif

//------------------------------------------------------------------------------
// Dispatcher
//------------------------------------------------------------------------------

    /// \ingroup dfh_tick_id_delta
    /// \brief Decodes ZigZag-encoded int64 deltas stored in \c uint64_t back into trade IDs.
    /// \details
    /// Restores IDs using: \c cur_id = prev_id + delta + 1, where \c delta is ZigZag-decoded.
    /// \tparam TickType Tick structure type. Must provide \c set_trade_id(uint64_t).
    /// \param deltas Input deltas array. Must contain \p size elements.
    /// \param ticks Output tick array. Must contain \p size elements. Only trade ID is written.
    /// \param size Number of elements to decode.
    /// \param initial_id Base ID used for the first reconstruction.
    /// \note No range validation is performed; caller must ensure input is valid.
    template<class TickType>
    inline void decode_id_delta_u64(
            const std::uint64_t* deltas,
            TickType* ticks,
            std::size_t size,
            std::int64_t initial_id
        ) noexcept {
        if (size == 0) return;
#       if defined(__AVX2__)
        decode_id_delta_avx2_u64(deltas, ticks, size, initial_id);
#       elif defined(__SSE2__)
        decode_id_delta_sse2_u64(deltas, ticks, size, initial_id);
#       else
        decode_id_delta_scalar_u64(deltas, ticks, size, initial_id);
#       endif
    }

//------------------------------------------------------------------------------


#endif // _DFH_COMPRESSION_UTILS_ZIG_ZAG_DELTA_ID_HPP_INCLUDED
