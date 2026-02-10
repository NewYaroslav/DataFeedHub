#pragma once
#ifndef _DFH_COMPRESSION_UTILS_ZIG_ZAG_DELTA_HPP_INCLUDED
#define _DFH_COMPRESSION_UTILS_ZIG_ZAG_DELTA_HPP_INCLUDED

/// \file
/// \brief Delta + ZigZag codecs (scalar/SIMD) and compile-time dispatched wrappers.
///
/// \warning Do NOT include this header directly.
/// Include only via \c include/DataFeedHub/compression/utils.hpp to guarantee that all required
/// dependencies and configuration macros are included in the correct order.
///
/// \details
/// This header provides a set of delta + ZigZag codecs. Most algorithms have multiple
/// implementations (scalar, SSE2, AVX2, etc.) to enable fair comparative benchmarking.
/// In normal usage you should call the *dispatcher* functions: they automatically select
/// the fastest supported implementation using compile-time feature macros.
///
/// \note Some low-level ZigZag helpers (encode/decode primitives) are located in \c zig_zag.hpp.
///
/// \section dfh_zigzag_delta_api API overview
/// \subsection dfh_zigzag_delta_api_dispatch Dispatchers (use these in production)
/// - \c encode_delta_zig_zag_u32(...)
/// - \c encode_delta_zig_zag_i32(...)
/// - \c encode_delta_zig_zag_u64(...)
/// - \c encode_delta_zig_zag_i64(...)
/// - \c decode_delta_zig_zag_u32(...)
/// - \c decode_delta_zig_zag_i32(...)
/// - \c decode_delta_zig_zag_u64(...)
/// - \c decode_delta_zig_zag_i64(...)
///
/// \subsection dfh_zigzag_delta_api_scalar Scalar implementations (for correctness & benchmarks)
/// - \c encode_delta_zig_zag_scalar_u32(...)
/// - \c encode_delta_zig_zag_scalar_i32(...)
/// - \c encode_delta_zig_zag_scalar_u64(...)
/// - \c encode_delta_zig_zag_scalar_i64(...)
/// - \c decode_delta_zig_zag_scalar_u32(...)
/// - \c decode_delta_zig_zag_scalar_i32(...)
/// - \c decode_delta_zig_zag_scalar_u64(...)
/// - \c decode_delta_zig_zag_scalar_i64(...)
///
/// \subsection dfh_zigzag_delta_api_sse2 SSE2 implementations (benchmarks / optional direct use)
/// - \c encode_delta_zig_zag_sse2_u32(...)
/// - \c encode_delta_zig_zag_sse2_i32(...)
/// - \c encode_delta_zig_zag_sse2_u64(...)
/// - \c encode_delta_zig_zag_sse2_i64(...)
/// - \c decode_delta_zig_zag_sse2_u32(...)
/// - \c decode_delta_zig_zag_sse2_i32(...)
/// - \c decode_delta_zig_zag_sse2_u64(...)
/// - \c decode_delta_zig_zag_sse2_i64(...)
///
/// \subsection dfh_zigzag_delta_api_avx2 AVX2 implementations (benchmarks / optional direct use)
/// - \c encode_delta_zig_zag_avx2_u32(...)
/// - \c encode_delta_zig_zag_avx2_i32(...)
/// - \c encode_delta_zig_zag_avx2_u64(...)
/// - \c encode_delta_zig_zag_avx2_i64(...)
/// - \c decode_delta_zig_zag_avx2_u32(...)
/// - \c decode_delta_zig_zag_avx2_i32(...)
/// - \c decode_delta_zig_zag_avx2_u64(...)
/// - \c decode_delta_zig_zag_avx2_i64(...)
///
/// \subsection dfh_zigzag_delta_api_not_dispatched SIMD backends not used by dispatchers
/// - \c encode_delta_zig_zag_sse2_u32(...)
/// - \c encode_delta_zig_zag_sse2_i32(...)
/// - \c decode_delta_zig_zag_sse2_u64(...)
/// - \c decode_delta_zig_zag_sse2_i64(...)
/// - \c encode_delta_zig_zag_avx512_u64(...)
/// - \c encode_delta_zig_zag_avx512_i64(...)
///
/// These backends are kept for micro-benchmarking and ISA studies.

#include "zig_zag.hpp"

namespace dfh::compression {

    /// \brief Checks whether the mathematical difference (a - b) fits in int32_t.
    /// \details Does not evaluate signed (a - b). Uses uint64_t wraparound arithmetic (mod 2^64)
    ///          and checks membership in a 2^32-sized window corresponding to [INT32_MIN, INT32_MAX].
    [[nodiscard]] inline bool diff_fits_i32(std::int64_t a, std::int64_t b) noexcept {
        // Want: d = a - b in [INT32_MIN, INT32_MAX]
        // Equivalent: (d - INT32_MIN) in [0, 2^32 - 1]
        // Compute using uint64_t (mod 2^64) to avoid signed overflow/UB.
        std::uint64_t ua = static_cast<std::uint64_t>(a);
        std::uint64_t ub = static_cast<std::uint64_t>(b);

        std::uint64_t d  = ua - ub; // modulo 2^64
        // Shift by -INT32_MIN (= 2^31) and check that the result fits in 32 bits.
        std::uint64_t x  = d - static_cast<std::uint64_t>(static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min())); // subtract (-2^31) => add 2^31

        return x <= 0xFFFFFFFFull; // fits in int32 range
    }

    /// \brief Checks whether the mathematical difference (a - b) fits in int64_t.
    /// \details Returns false if computing (a - b) in int64_t would overflow.
    [[nodiscard]] inline bool safe_diff_check_i64(std::int64_t a, std::int64_t b) noexcept {
#       if defined(__GNUC__) || defined(__clang__)
        std::int64_t out;
        return !__builtin_sub_overflow(a, b, &out);
#       else
        // Portable fallback for compilers without __builtin_sub_overflow.
        if (b > 0) return a >= (std::numeric_limits<std::int64_t>::min() + b);
        if (b < 0) return a <= (std::numeric_limits<std::int64_t>::max() + b);
        return true;
#       endif
    }

//------------------------------------------------------------------------------

    /// \brief Delta-encodes a non-decreasing (sorted) sequence.
    /// \details
    /// Produces `output[i] = input[i] - prev`, where `prev` is `initial_value` for i=0
    /// and `input[i-1]` for i>0.
    ///
    /// \tparam InputType  Input value type (e.g., uint32_t, uint64_t).
    /// \tparam OutputType Delta type (typically same as InputType).
    /// \param input          Input array of size \p size (must be non-decreasing).
    /// \param output         Output array of deltas of size \p size.
    /// \param size           Number of elements.
    /// \param initial_value  Base value for the first delta (usually input[0] of previous block).
    /// \warning Assumes `input[i] >= prev` for all i. If this is violated and the types are unsigned,
    ///          the subtraction will underflow (wrap). For signed types, overflow/underflow is UB.
    template<class InputType = std::uint32_t, class OutputType = std::uint32_t>
    void encode_delta_sorted(
            const InputType* input,
            OutputType* output,
            std::size_t size,
            InputType initial_value
        ) {
        if (size == 0) return;
        
        // Compute in a widened unsigned type when possible to avoid surprises on casts.
        using Wide = std::conditional_t<
                (sizeof(InputType) > sizeof(OutputType)), InputType, OutputType>;

        for (std::size_t i = 0; i < size; ++i) {
            const Wide prev = static_cast<Wide>(initial_value);
            const Wide cur  = static_cast<Wide>(input[i]);
            const Wide d    = cur - prev;

            output[i] = static_cast<OutputType>(d);
            initial_value = input[i];
        }
    }

    /// \brief Decodes a delta-encoded non-decreasing (sorted) sequence.
    /// \details
    /// Restores values as:
    /// - `output[0] = initial_value + input[0]`
    /// - `output[i] = output[i-1] + input[i]` for i>0
    ///
    /// \tparam InputType  Delta type (e.g., uint32_t, uint64_t).
    /// \tparam OutputType Output value type (typically same as InputType).
    /// \param input          Input array of deltas of size \p size.
    /// \param output         Output array of restored values of size \p size.
    /// \param size           Number of elements.
    /// \param initial_value  Base value for the first element (the same base used by encoder).
    /// \warning For unsigned types, overflow wraps around. For signed types, overflow is UB.
    template<class InputType = std::uint32_t, class OutputType = std::uint32_t>
    void decode_delta_sorted(
            const InputType* input,
            OutputType* output,
            std::size_t size,
            OutputType initial_value
        ) {
        if (size == 0) return;
        output[0] = static_cast<OutputType>(initial_value + static_cast<OutputType>(input[0]));
        for (std::size_t i = 1; i < size; ++i) {
            output[i] = static_cast<OutputType>(output[i - 1] + static_cast<OutputType>(input[i]));
        }
    }

//------------------------------------------------------------------------------

    /// \defgroup dfh_tick_time_delta Time delta codecs
    /// \ingroup dfh_compression
    /// \brief Encoding/decoding of tick timestamps as millisecond deltas.
    ///
    /// The encoder stores timestamps as non-negative deltas in milliseconds:
    /// - \c delta[0] = ticks[0].time_ms - base_time_ms
    /// - \c delta[i] = ticks[i].time_ms - ticks[i-1].time_ms, for i > 0
    ///
    /// The decoder reconstructs absolute timestamps:
    /// - \c ticks[0].time_ms = delta[0] + base_time_ms
    /// - \c ticks[i].time_ms = ticks[i-1].time_ms + delta[i]
    ///
    /// Requirements:
    /// - Input timestamps must be non-decreasing.
    /// - Deltas are stored as \c uint32_t, so callers must ensure each delta fits 32 bits.
    ///   Pick an appropriate \c base_time_ms and sampling interval.
    ///
    /// \par Example
    /// \code
    /// struct MarketTick { std::uint64_t time_ms; };
    ///
    /// std::vector<MarketTick> ticks = { {1000}, {1010}, {1050} };
    /// const std::int64_t base_time_ms = 900;
    ///
    /// std::vector<std::uint32_t> deltas(ticks.size());
    /// dfh::compression::encode_time_delta(ticks.data(), deltas.data(), ticks.size(), base_time_ms);
    ///
    /// std::vector<MarketTick> restored(ticks.size());
    /// dfh::compression::decode_time_delta(deltas.data(), restored.data(), restored.size(), base_time_ms);
    /// \endcode


    /// \ingroup dfh_tick_time_delta
    /// \brief Encodes \c time_ms into millisecond deltas stored in \c uint32_t.
    /// \details Deltas are guaranteed to fit into signed 32-bit range \c [0..INT32_MAX]
    ///          (SIMD-friendly invariant). The input timestamps must be non-decreasing.
    /// \tparam TickType Tick structure type. Must provide a readable member \c time_ms (recommended \c int64_t).
    /// \param ticks Pointer to the first tick.
    /// \param output Output buffer. Must contain at least \p size elements.
    /// \param size Number of ticks to encode.
    /// \param initial_time Base timestamp in milliseconds for the first delta (must be non-negative).
    /// \throw std::invalid_argument If \p initial_time is negative or if \c time_ms is not non-decreasing.
    /// \throw std::overflow_error If any delta exceeds \c INT32_MAX.
    template<class TickType>
    void encode_time_delta(
            const TickType* ticks,
            std::uint32_t* output,
            std::size_t size,
            std::int64_t initial_time) {
        if (size == 0) return;
        if (initial_time < 0) throw std::invalid_argument("encode_time_delta: initial_time must be >= 0");

        std::int64_t cur, delta;
        for (std::size_t i = 0; i < size; ++i) {
            cur = ticks[i].time_ms;
            if (cur < initial_time)  throw std::invalid_argument("encode_time_delta: timestamps must be non-decreasing");
            delta = cur - initial_time;
            if (delta > std::numeric_limits<std::int32_t>::max()) throw std::overflow_error("encode_time_delta: delta > int32_max");
            output[i] = static_cast<std::uint32_t>(delta);
            initial_time = cur;
        }
    }

    /// \ingroup dfh_tick_time_delta
    /// \brief Decodes millisecond deltas stored in \c uint32_t back into \c time_ms.
    /// \details Assumes deltas were produced by \ref encode_time_delta and therefore fit into
    ///          signed 32-bit range \c [0..INT32_MAX]. No validation is performed.
    /// \tparam TickType Tick structure type. Must provide a writable member \c time_ms (recommended \c int64_t).
    /// \param deltas Input deltas array. Must contain \p size elements.
    /// \param ticks Output tick array. Must contain \p size elements. Only \c time_ms is written.
    /// \param size Number of elements to decode.
    /// \param initial_time Base timestamp in milliseconds for the first element (must be non-negative).
    template<class TickType>
    void decode_time_delta(
            const std::uint32_t* deltas,
            TickType* ticks,
            std::size_t size,
            std::int64_t initial_time) {
        if (size == 0) return;

        for (std::size_t i = 0; i < size; ++i) {
            initial_time += static_cast<std::int64_t>(deltas[i]);
            ticks[i].time_ms = initial_time;
        }
    }
    
//------------------------------------------------------------------------------

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
            detail::trade_id_writer<TickType>::write(ticks[i], static_cast<std::uint64_t>(cur));
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

        // Align deltas for aligned loads (16 bytes).
        constexpr std::size_t align = 16;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(deltas + i) % align) == 0) break;
            cur += static_cast<std::int64_t>(zigzag_decode_u32(deltas[i])) + delta_offset;
            detail::trade_id_writer<TickType>::write(ticks[i], static_cast<std::uint64_t>(cur));
        }

        constexpr std::size_t simd_width = 4;
        const std::size_t simd_end = i + ((size - i) / simd_width) * simd_width;

        alignas(16) std::int32_t prefix[simd_width];
        const __m128i ones = _mm_set1_epi32(static_cast<int>(delta_offset));

        for (; i < simd_end; i += simd_width) {
            const __m128i z = _mm_load_si128(reinterpret_cast<const __m128i*>(deltas + i));
            __m128i d = zigzag_decode_u32_sse2(z);
            d = _mm_add_epi32(d, ones); // increments = delta + 1

            // Prefix sum in lanes: [a, a+b, a+b+c, a+b+c+d]
            __m128i t = d;
            t = _mm_add_epi32(t, _mm_slli_si128(t, 4));
            t = _mm_add_epi32(t, _mm_slli_si128(t, 8));

            _mm_store_si128(reinterpret_cast<__m128i*>(prefix), t);

            // Expand to int64 and write
            const std::uint64_t id0 = static_cast<std::uint64_t>(cur + static_cast<std::int64_t>(prefix[0]));
            const std::uint64_t id1 = static_cast<std::uint64_t>(cur + static_cast<std::int64_t>(prefix[1]));
            const std::uint64_t id2 = static_cast<std::uint64_t>(cur + static_cast<std::int64_t>(prefix[2]));
            const std::uint64_t id3 = static_cast<std::uint64_t>(cur + static_cast<std::int64_t>(prefix[3]));

            detail::trade_id_writer<TickType>::write(ticks[i + 0], id0);
            detail::trade_id_writer<TickType>::write(ticks[i + 1], id1);
            detail::trade_id_writer<TickType>::write(ticks[i + 2], id2);
            detail::trade_id_writer<TickType>::write(ticks[i + 3], id3);

            cur += static_cast<std::int64_t>(prefix[3]); // carry
        }

        for (; i < size; ++i) {
            cur += static_cast<std::int64_t>(zigzag_decode_u32(deltas[i])) + delta_offset;
            detail::trade_id_writer<TickType>::write(ticks[i], static_cast<std::uint64_t>(cur));
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

        // Align deltas for aligned loads (32 bytes).
        constexpr std::size_t align = 32;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(deltas + i) % align) == 0) break;
            cur += static_cast<std::int64_t>(zigzag_decode_u32(deltas[i])) + delta_offset;
            detail::trade_id_writer<TickType>::write(ticks[i], static_cast<std::uint64_t>(cur));
        }

        constexpr std::size_t simd_width = 8;
        const std::size_t simd_end = i + ((size - i) / simd_width) * simd_width;

        alignas(32) std::int32_t prefix[simd_width];
        const __m256i ones = _mm256_set1_epi32(static_cast<int>(delta_offset));

        for (; i < simd_end; i += simd_width) {
            const __m256i z = _mm256_load_si256(reinterpret_cast<const __m256i*>(deltas + i));
            __m256i d = zigzag_decode_u32_avx2(z);
            d = _mm256_add_epi32(d, ones); // increments

            // Prefix sum within each 128-bit lane
            __m256i t = d;
            t = _mm256_add_epi32(t, _mm256_slli_si256(t, 4));
            t = _mm256_add_epi32(t, _mm256_slli_si256(t, 8));

            // Carry from low 128 lane into high 128 lane
            const __m128i low = _mm256_castsi256_si128(t);
            const std::int32_t low_last = _mm_extract_epi32(low, 3);
            const __m256i add_hi = _mm256_setr_epi32(0,0,0,0, low_last,low_last,low_last,low_last);
            t = _mm256_add_epi32(t, add_hi);

            _mm256_store_si256(reinterpret_cast<__m256i*>(prefix), t);

            // Expand + write
            for (std::size_t k = 0; k < simd_width; ++k) {
                const std::uint64_t id =
                    static_cast<std::uint64_t>(cur + static_cast<std::int64_t>(prefix[k]));
                detail::trade_id_writer<TickType>::write(ticks[i + k], id);
            }

            cur += static_cast<std::int64_t>(prefix[simd_width - 1]);
        }

        for (; i < size; ++i) {
            cur += static_cast<std::int64_t>(zigzag_decode_u32(deltas[i])) + delta_offset;
            detail::trade_id_writer<TickType>::write(ticks[i], static_cast<std::uint64_t>(cur));
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
#       if defined(__AVX2__)
        decode_id_delta_avx2_u32(deltas, ticks, size, initial_id);
#       elif defined(__SSE2__)
        decode_id_delta_sse2_u32(deltas, ticks, size, initial_id);
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
            detail::trade_id_writer<TickType>::write(ticks[i], static_cast<std::uint64_t>(cur));
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

        // Align deltas for aligned loads (16 bytes).
        constexpr std::size_t align = 16;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(deltas + i) % align) == 0) break;
            cur += static_cast<std::int64_t>(zigzag_decode_u64(deltas[i])) + delta_offset;
            detail::trade_id_writer<TickType>::write(ticks[i], static_cast<std::uint64_t>(cur));
        }

        constexpr std::size_t simd_width = 2;
        const std::size_t simd_end = i + ((size - i) / simd_width) * simd_width;

        alignas(16) std::int64_t prefix[simd_width];
        const __m128i ones = _mm_set1_epi64x(delta_offset);

        for (; i < simd_end; i += simd_width) {
            const __m128i z = _mm_load_si128(reinterpret_cast<const __m128i*>(deltas + i));
            __m128i d = zigzag_decode_u64_sse2(z);
            d = _mm_add_epi64(d, ones); // increments = delta + 1

            // Prefix sum for 2 lanes: [a, a+b]
            __m128i t = d;
            t = _mm_add_epi64(t, _mm_slli_si128(t, 8));

            _mm_store_si128(reinterpret_cast<__m128i*>(prefix), t);

            const std::uint64_t id0 = static_cast<std::uint64_t>(cur + prefix[0]);
            const std::uint64_t id1 = static_cast<std::uint64_t>(cur + prefix[1]);

            detail::trade_id_writer<TickType>::write(ticks[i + 0], id0);
            detail::trade_id_writer<TickType>::write(ticks[i + 1], id1);

            cur += prefix[1];
        }

        for (; i < size; ++i) {
            cur += static_cast<std::int64_t>(zigzag_decode_u64(deltas[i])) + delta_offset;
            detail::trade_id_writer<TickType>::write(ticks[i], static_cast<std::uint64_t>(cur));
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

        // Align deltas for aligned loads (32 bytes).
        constexpr std::size_t align = 32;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(deltas + i) % align) == 0) break;
            cur += static_cast<std::int64_t>(zigzag_decode_u64(deltas[i])) + delta_offset;
            detail::trade_id_writer<TickType>::write(ticks[i], static_cast<std::uint64_t>(cur));
        }

        constexpr std::size_t simd_width = 4;
        const std::size_t simd_end = i + ((size - i) / simd_width) * simd_width;

        alignas(32) std::int64_t prefix[simd_width];
        const __m256i ones = _mm256_set1_epi64x(delta_offset);

        for (; i < simd_end; i += simd_width) {
            const __m256i z = _mm256_load_si256(reinterpret_cast<const __m256i*>(deltas + i));
            __m256i d = zigzag_decode_u64_avx2(z);
            d = _mm256_add_epi64(d, ones); // increments

            // Prefix sum inside each 128-bit lane (2 + 2), then carry to high lane.
            __m256i t = d;
            t = _mm256_add_epi64(t, _mm256_slli_si256(t, 8)); // within each 128: [a, a+b] | [c, c+d]

            // carry from low lane last (element 1) to high lane elements (2..3)
            const __m128i low = _mm256_castsi256_si128(t);
            const std::int64_t low_last = _mm_cvtsi128_si64(_mm_srli_si128(low, 8)); // element 1
            const __m256i add_hi = _mm256_setr_epi64x(0, 0, low_last, low_last);
            t = _mm256_add_epi64(t, add_hi); // now: [a, a+b, a+b+c, a+b+c+d]

            _mm256_store_si256(reinterpret_cast<__m256i*>(prefix), t);

            for (std::size_t k = 0; k < simd_width; ++k) {
                const std::uint64_t id = static_cast<std::uint64_t>(cur + prefix[k]);
                detail::trade_id_writer<TickType>::write(ticks[i + k], id);
            }

            cur += prefix[simd_width - 1];
        }

        for (; i < size; ++i) {
            cur += static_cast<std::int64_t>(zigzag_decode_u64(deltas[i])) + delta_offset;
            detail::trade_id_writer<TickType>::write(ticks[i], static_cast<std::uint64_t>(cur));
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
    bool encode_price_delta_zig_zag_u32_scalar(
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
    bool encode_price_delta_zig_zag_u32_sse2(
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
    bool encode_price_delta_zig_zag_u32_avx2(
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
    bool encode_price_delta_zig_zag_u32_avx512(
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
        return encode_price_delta_zig_zag_u32_avx512<TickType, PriceMember>(ticks, output, size, price_scale, initial_price);
#       elif defined(__AVX2__)
        return encode_price_delta_zig_zag_u32_avx2<TickType, PriceMember>(ticks, output, size, price_scale, initial_price);
#       elif defined(__SSE2__)
        return encode_price_delta_zig_zag_u32_sse2<TickType, PriceMember>(ticks, output, size, price_scale, initial_price);
#       else
        return encode_price_delta_zig_zag_u32_scalar<TickType, PriceMember>(ticks, output, size, price_scale, initial_price);
#       endif
    }
    
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

    template<class TickType, double TickType::* PriceMember>
    void decode_price_delta_zig_zag_u32_scalar(
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
    void decode_price_delta_zig_zag_u32_sse2(
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
    void decode_price_delta_zig_zag_u32_avx2(
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
    template<class TickType, double TickType::* PriceMember>
    void decode_price_delta_zig_zag_u32_avx512(
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
#       if defined(__AVX512F__)
        decode_price_delta_zig_zag_u32_avx512<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
#       elif defined(__AVX2__)
        decode_price_delta_zig_zag_u32_avx2<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
#       elif defined(__SSE2__)
        decode_price_delta_zig_zag_u32_sse2<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
#       else
        decode_price_delta_zig_zag_u32_scalar<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
#       endif
    }
    
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

    template<class TickType, double TickType::* PriceMember>
    void encode_price_delta_zig_zag_u64_scalar(
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
    void encode_price_delta_zig_zag_u64_avx2(
            const TickType* ticks,
            std::uint64_t* output,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) {
        if (size == 0) return;
        if (!ticks || !output) return;

        std::int64_t prev = initial_price;

        std::size_t i = 0;
        for (; i + 4 <= size; i += 4) {
            const std::int64_t c0 = static_cast<std::int64_t>(std::llround((ticks[i+0].*PriceMember) * price_scale));
            const std::int64_t c1 = static_cast<std::int64_t>(std::llround((ticks[i+1].*PriceMember) * price_scale));
            const std::int64_t c2 = static_cast<std::int64_t>(std::llround((ticks[i+2].*PriceMember) * price_scale));
            const std::int64_t c3 = static_cast<std::int64_t>(std::llround((ticks[i+3].*PriceMember) * price_scale));

            // safe diff checks (scalar; cheap vs llround)
            if (!safe_diff_check_i64(c0, prev)) throw std::overflow_error("encode_price_delta_zig_zag_u64: int64 delta overflow");
            if (!safe_diff_check_i64(c1, c0  )) throw std::overflow_error("encode_price_delta_zig_zag_u64: int64 delta overflow");
            if (!safe_diff_check_i64(c2, c1  )) throw std::overflow_error("encode_price_delta_zig_zag_u64: int64 delta overflow");
            if (!safe_diff_check_i64(c3, c2  )) throw std::overflow_error("encode_price_delta_zig_zag_u64: int64 delta overflow");

            // vectorize deltas + zigzag
            __m256i cur  = _mm256_setr_epi64x(c0, c1, c2, c3);
            __m256i prevv= _mm256_setr_epi64x(prev, c0, c1, c2);
            __m256i d    = _mm256_sub_epi64(cur, prevv);

            // sign mask: all-ones if d<0
            __m256i sign = _mm256_cmpgt_epi64(_mm256_setzero_si256(), d);
            __m256i zz   = _mm256_xor_si256(_mm256_slli_epi64(d, 1), sign);

            _mm256_store_si256(reinterpret_cast<__m256i*>(output + i), zz);

            prev = c3;
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
    void encode_price_delta_zig_zag_u64_avx512(
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
            __m512i prevv = _mm512_alignr_epi64(cur, cur, 7); // rotates; we’ll overwrite lane0 anyway
            // after alignr, lane0 contains c7; fix by shifting:
            // simpler: make index permute
            const __m512i idx = _mm512_setr_epi64(0,0,1,2,3,4,5,6);
            prevv = _mm512_permutexvar_epi64(idx, cur);
            prevv = _mm512_mask_mov_epi64(prevv, 0x01, _mm512_set1_epi64(prev)); // lane0 = prev

            __m512i d = _mm512_sub_epi64(cur, prevv);

            // safe_diff_check_i64 for each lane? full SIMD overflow check is bulky.
            // Here: follow your contract: throw if intermediate subtraction overflows int64.
            // In practice prices won’t overflow int64; we do scalar check per element cheaply.
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
        encode_price_delta_zig_zag_u64_avx512<TickType, PriceMember>(ticks, output, size, price_scale, initial_price);
#       elif defined(__AVX2__)
        encode_price_delta_zig_zag_u64_avx2<TickType, PriceMember>(ticks, output, size, price_scale, initial_price);
#       else
        // SSE2: no good vector ZigZag(i64); keep scalar
        encode_price_delta_zig_zag_u64_scalar<TickType, PriceMember>(ticks, output, size, price_scale, initial_price);
#       endif
    }

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

    template<class TickType, double TickType::* PriceMember>
    /// \brief Decodes an encoded delta stream into destination ticks (backend helper).
    /// \note Function: \c decode_price_delta_zig_zag_u64_scalar.
    /// \thread_safety Thread-safe (no shared mutable state).


    inline void decode_price_delta_zig_zag_u64_scalar(
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
    /// \note Function: \c decode_price_delta_zig_zag_u64_sse2.
    /// \thread_safety Thread-safe (no shared mutable state).


    inline void decode_price_delta_zig_zag_u64_sse2(
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

            // AoS store: scalar loop (как у тебя в примере)
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
    /// \note Function: \c decode_price_delta_zig_zag_u64_avx2.
    /// \thread_safety Thread-safe (no shared mutable state).


    inline void decode_price_delta_zig_zag_u64_avx2(
            const std::uint64_t* deltas,
            TickType* ticks,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price) noexcept {
        if (size == 0) return;
        const double inv_scale = 1.0 / price_scale;
        std::int64_t cur = initial_price;
        std::size_t i = 0;
        // Align for aligned loads (32 bytes).
        constexpr std::size_t align = 32;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(deltas + i) % align) == 0) break;
            cur += zigzag_decode_u64_scalar(deltas[i]);
            ticks[i].*PriceMember = static_cast<double>(cur) * inv_scale;
        }

        constexpr std::size_t simd_width = 4;
        const std::size_t simd_end = i + ((size - i) / simd_width) * simd_width;

        alignas(32) std::int64_t prefix[simd_width];

        for (; i < simd_end; i += simd_width) {
            const __m256i z = _mm256_load_si256(reinterpret_cast<const __m256i*>(deltas + i));
            __m256i d = zigzag_decode_u64_avx2(z);

            // Prefix sum inside each 128-bit lane: [a,b] | [c,d] -> [a,a+b] | [c,c+d]
            __m256i t = d;
            t = _mm256_add_epi64(t, _mm256_slli_si256(t, 8));

            // carry from low lane last (element 1) to high lane elements (2..3)
            const __m128i low = _mm256_castsi256_si128(t);
            const std::int64_t low_last = _mm_cvtsi128_si64(_mm_srli_si128(low, 8)); // element 1
            const __m256i add_hi = _mm256_setr_epi64x(0, 0, low_last, low_last);
            t = _mm256_add_epi64(t, add_hi); // [a, a+b, a+b+c, a+b+c+d]

            _mm256_store_si256(reinterpret_cast<__m256i*>(prefix), t);

            // AoS store: scalar loop
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
    /// \note Function: \c decode_price_delta_zig_zag_u64_avx512.
    /// \thread_safety Thread-safe (no shared mutable state).


    inline void decode_price_delta_zig_zag_u64_avx512(
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
#       if defined(__AVX512F__)
        decode_price_delta_zig_zag_u64_avx512<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
#       elif defined(__AVX2__)
        decode_price_delta_zig_zag_u64_avx2<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
#       elif defined(__SSE2__)
        decode_price_delta_zig_zag_u64_sse2<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
#       else
        decode_price_delta_zig_zag_u64_scalar<TickType, PriceMember>(deltas, ticks, size, price_scale, initial_price);
#       endif
    }

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

    /// \name Delta+ZigZag integer codecs (32-bit paths)
    /// \brief Backend-specific and dispatcher helpers for 32-bit encode/decode.
    /// \note Backends: scalar, SSE2, AVX2, AVX512F (where available).
    /// @{

    /// \brief Encodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c encode_delta_zig_zag_scalar_u32.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline bool encode_delta_zig_zag_scalar_u32(
            const std::uint32_t* input,
            std::uint32_t* output,
            std::size_t size,
            std::uint32_t initial_value
        ) noexcept {
        constexpr std::int64_t minv = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min());
        constexpr std::int64_t maxv = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max());
        std::int64_t d;
        std::uint32_t prev = initial_value;
        for (std::size_t i = 0; i < size; ++i) {
            d = static_cast<std::int64_t>(input[i]) - static_cast<std::int64_t>(prev);
            if (d < minv || d > maxv) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(d));
            prev = input[i];
        }
        return true;
    }

    /// \brief Encodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c encode_delta_zig_zag_scalar_i32.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline bool encode_delta_zig_zag_scalar_i32(
            const std::int32_t* input,
            std::uint32_t* output,
            std::size_t size,
            std::uint32_t initial_value
        ) noexcept {
        constexpr std::int64_t minv = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min());
        constexpr std::int64_t maxv = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max());

        std::int64_t d;
        std::int64_t prev = static_cast<std::int64_t>(static_cast<std::int32_t>(initial_value));
        for (std::size_t i = 0; i < size; ++i) {
            d = static_cast<std::int64_t>(input[i]) - prev;
            if (d < minv || d > maxv) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(d));
            prev = static_cast<std::int64_t>(input[i]);
        }
        return true;
    }


#   if defined(__SSE2__)
    /// \brief Encodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c encode_delta_zig_zag_sse2_u32.
    /// \note Kept for benchmarks/manual comparison; release dispatcher uses scalar on SSE2-only targets
    ///       because this backend does not provide stable speedup vs scalar.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline bool encode_delta_zig_zag_sse2_u32(
            const std::uint32_t* input,
            std::uint32_t* output,
            std::size_t size,
            std::uint32_t initial_value) noexcept {
        constexpr std::int64_t minv = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min());
        constexpr std::int64_t maxv = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max());

        std::size_t i = 0;
        std::uint32_t prev = initial_value;

        // align both for 16B stores/loads
        constexpr std::size_t align = 16;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            const std::int64_t d = static_cast<std::int64_t>(input[i]) - static_cast<std::int64_t>(prev);
            if (d < minv || d > maxv) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(d));
            prev = input[i];
        }

        constexpr std::size_t W = 4;
        const std::size_t end = i + ((size - i) / W) * W;
        alignas(16) std::int32_t d[W];

        for (; i < end; i += W) {
            // scalar delta+check (safe)
            for (std::size_t k = 0; k < W; ++k) {
                const std::int64_t dk = static_cast<std::int64_t>(input[i + k]) - static_cast<std::int64_t>(prev);
                if (dk < minv || dk > maxv) return false;
                d[k] = static_cast<std::int32_t>(dk);
                prev = input[i + k];
            }
            __m128i dv = _mm_load_si128(reinterpret_cast<const __m128i*>(d));
            __m128i zz = zigzag_encode_u32_sse2(dv);
            _mm_store_si128(reinterpret_cast<__m128i*>(output + i), zz);
        }

        for (; i < size; ++i) {
            const std::int64_t dk = static_cast<std::int64_t>(input[i]) - static_cast<std::int64_t>(prev);
            if (dk < minv || dk > maxv) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(dk));
            prev = input[i];
        }
        return true;
    }

    /// \brief Encodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c encode_delta_zig_zag_sse2_i32.
    /// \note Kept for benchmarks/manual comparison; release dispatcher uses scalar on SSE2-only targets
    ///       because this backend does not provide stable speedup vs scalar.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline bool encode_delta_zig_zag_sse2_i32(
            const std::int32_t* input,
            std::uint32_t* output,
            std::size_t size,
            std::uint32_t initial_value
        ) noexcept {
        constexpr std::int64_t minv = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min());
        constexpr std::int64_t maxv = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max());

        std::size_t i = 0;
        std::int64_t prev = static_cast<std::int64_t>(static_cast<std::int32_t>(initial_value));

        constexpr std::size_t align = 16;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            const std::int64_t d0 = static_cast<std::int64_t>(input[i]) - prev;
            if (d0 < minv || d0 > maxv) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(d0));
            prev = static_cast<std::int64_t>(input[i]);
        }

        constexpr std::size_t W = 4;
        const std::size_t end = i + ((size - i) / W) * W;
        alignas(16) std::int32_t d[W];

        for (; i < end; i += W) {
            for (std::size_t k = 0; k < W; ++k) {
                const std::int64_t dk = static_cast<std::int64_t>(input[i + k]) - prev;
                if (dk < minv || dk > maxv) return false;
                d[k] = static_cast<std::int32_t>(dk);
                prev = static_cast<std::int64_t>(input[i + k]);
            }
            __m128i dv = _mm_load_si128(reinterpret_cast<const __m128i*>(d));
            __m128i zz = zigzag_encode_u32_sse2(dv);
            _mm_store_si128(reinterpret_cast<__m128i*>(output + i), zz);
        }

        for (; i < size; ++i) {
            const std::int64_t dk = static_cast<std::int64_t>(input[i]) - prev;
            if (dk < minv || dk > maxv) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(dk));
            prev = static_cast<std::int64_t>(input[i]);
        }
        return true;
    }
#   endif

#   if defined(__AVX2__)
    /// \brief Encodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c encode_delta_zig_zag_avx2_u32.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline bool encode_delta_zig_zag_avx2_u32(
            const std::uint32_t* input,
            std::uint32_t* output,
            std::size_t size,
            std::uint32_t initial_value
        ) noexcept {
        constexpr std::int64_t minv = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min());
        constexpr std::int64_t maxv = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max());

        std::size_t i = 0;
        std::uint32_t prev = initial_value;
        std::int64_t delta;

        constexpr std::size_t align = 32;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            delta = static_cast<std::int64_t>(input[i]) - static_cast<std::int64_t>(prev);
            if (delta < minv || delta > maxv) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(delta));
            prev = input[i];
        }

        constexpr std::size_t W = 8;
        const std::size_t end = i + ((size - i) / W) * W;
        alignas(32) std::int32_t d[W];

        for (; i < end; i += W) {
            for (std::size_t k = 0; k < W; ++k) {
                delta = static_cast<std::int64_t>(input[i + k]) - static_cast<std::int64_t>(prev);
                if (delta < minv || delta > maxv) return false;
                d[k] = static_cast<std::int32_t>(delta);
                prev = input[i + k];
            }
            __m256i dv = _mm256_load_si256(reinterpret_cast<const __m256i*>(d));
            __m256i zz = zigzag_encode_u32_avx2(dv);
            _mm256_store_si256(reinterpret_cast<__m256i*>(output + i), zz);
        }

        for (; i < size; ++i) {
            delta = static_cast<std::int64_t>(input[i]) - static_cast<std::int64_t>(prev);
            if (delta < minv || delta > maxv) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(delta));
            prev = input[i];
        }
        return true;
    }

    /// \brief Encodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c encode_delta_zig_zag_avx2_i32.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline bool encode_delta_zig_zag_avx2_i32(
            const std::int32_t* input,
            std::uint32_t* output,
            std::size_t size,
            std::uint32_t initial_value
        ) noexcept {
        constexpr std::int64_t minv = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min());
        constexpr std::int64_t maxv = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max());

        std::size_t i = 0;
        std::int64_t prev = static_cast<std::int64_t>(static_cast<std::int32_t>(initial_value));
        std::int64_t delta;

        constexpr std::size_t align = 32;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            delta = static_cast<std::int64_t>(input[i]) - prev;
            if (delta < minv || delta > maxv) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(delta));
            prev = static_cast<std::int64_t>(input[i]);
        }

        constexpr std::size_t W = 8;
        const std::size_t end = i + ((size - i) / W) * W;
        alignas(32) std::int32_t d[W];

        for (; i < end; i += W) {
            for (std::size_t k = 0; k < W; ++k) {
                delta = static_cast<std::int64_t>(input[i + k]) - prev;
                if (delta < minv || delta > maxv) return false;
                d[k] = static_cast<std::int32_t>(delta);
                prev = static_cast<std::int64_t>(input[i + k]);
            }
            __m256i dv = _mm256_load_si256(reinterpret_cast<const __m256i*>(d));
            __m256i zz = zigzag_encode_u32_avx2(dv);
            _mm256_store_si256(reinterpret_cast<__m256i*>(output + i), zz);
        }

        for (; i < size; ++i) {
            delta = static_cast<std::int64_t>(input[i]) - prev;
            if (delta < minv || delta > maxv) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(delta));
            prev = static_cast<std::int64_t>(input[i]);
        }
        return true;
    }
#   endif

#   if defined(__AVX512F__)
    /// \brief Encodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c encode_delta_zig_zag_avx512_u32.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline bool encode_delta_zig_zag_avx512_u32(
            const std::uint32_t* input,
            std::uint32_t* output,
            std::size_t size,
            std::uint32_t initial_value
        ) noexcept {
        constexpr std::int64_t minv = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min());
        constexpr std::int64_t maxv = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max());

        std::size_t i = 0;
        std::uint32_t prev = initial_value;
        std::int64_t delta;

        constexpr std::size_t align = 64;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            delta = static_cast<std::int64_t>(input[i]) - static_cast<std::int64_t>(prev);
            if (delta < minv || delta > maxv) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(delta));
            prev = input[i];
        }

        constexpr std::size_t W = 16;
        const std::size_t end = i + ((size - i) / W) * W;
        alignas(64) std::int32_t d[W];

        for (; i < end; i += W) {
            for (std::size_t k = 0; k < W; ++k) {
                delta = static_cast<std::int64_t>(input[i + k]) - static_cast<std::int64_t>(prev);
                if (delta < minv || delta > maxv) return false;
                d[k] = static_cast<std::int32_t>(delta);
                prev = input[i + k];
            }
            __m512i dv = _mm512_load_si512(reinterpret_cast<const void*>(d));
            __m512i zz = zigzag_encode_u32_avx512(dv);
            _mm512_store_si512(reinterpret_cast<void*>(output + i), zz);
        }

        for (; i < size; ++i) {
            delta = static_cast<std::int64_t>(input[i]) - static_cast<std::int64_t>(prev);
            if (delta < minv || delta > maxv) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(delta));
            prev = input[i];
        }
        return true;
    }

    /// \brief Encodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c encode_delta_zig_zag_avx512_i32.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline bool encode_delta_zig_zag_avx512_i32(
            const std::int32_t* input,
            std::uint32_t* output,
            std::size_t size,
            std::uint32_t initial_value
        ) noexcept {
        constexpr std::int64_t minv = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min());
        constexpr std::int64_t maxv = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max());

        std::size_t i = 0;
        std::int64_t prev = static_cast<std::int64_t>(static_cast<std::int32_t>(initial_value));
        std::int64_t delta;

        constexpr std::size_t align = 64;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            delta = static_cast<std::int64_t>(input[i]) - prev;
            if (delta < minv || delta > maxv) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(delta));
            prev = static_cast<std::int64_t>(input[i]);
        }

        constexpr std::size_t W = 16;
        const std::size_t end = i + ((size - i) / W) * W;
        alignas(64) std::int32_t d[W];

        for (; i < end; i += W) {
            for (std::size_t k = 0; k < W; ++k) {
                delta = static_cast<std::int64_t>(input[i + k]) - prev;
                if (delta < minv || delta > maxv) return false;
                d[k] = static_cast<std::int32_t>(delta);
                prev = static_cast<std::int64_t>(input[i + k]);
            }
            __m512i dv = _mm512_load_si512(reinterpret_cast<const void*>(d));
            __m512i zz = zigzag_encode_u32_avx512(dv);
            _mm512_store_si512(reinterpret_cast<void*>(output + i), zz);
        }

        for (; i < size; ++i) {
            delta = static_cast<std::int64_t>(input[i]) - prev;
            if (delta < minv || delta > maxv) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(delta));
            prev = static_cast<std::int64_t>(input[i]);
        }
        return true;
    }
#   endif

    /// \brief Encodes a delta+ZigZag sequence (dispatcher).
    /// \note Function: \c encode_delta_zig_zag_u32.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline bool encode_delta_zig_zag_u32(
            const std::uint32_t* input,
            std::uint32_t* output,
            std::size_t size,
            std::uint32_t initial_value
        ) noexcept {
#       if defined(__AVX512F__)
        return encode_delta_zig_zag_avx512_u32(input, output, size, initial_value);
#       elif defined(__AVX2__)
        return encode_delta_zig_zag_avx2_u32(input, output, size, initial_value);
#       elif defined(__SSE2__)
        return encode_delta_zig_zag_scalar_u32(input, output, size, initial_value);
#       else
        return encode_delta_zig_zag_scalar_u32(input, output, size, initial_value);
#       endif
    }

    /// \brief Encodes a delta+ZigZag sequence (dispatcher).
    /// \note Function: \c encode_delta_zig_zag_i32.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline bool encode_delta_zig_zag_i32(
            const std::int32_t* input,
            std::uint32_t* output,
            std::size_t size,
            std::uint32_t initial_value
        ) noexcept {
#       if defined(__AVX512F__)
        return encode_delta_zig_zag_avx512_i32(input, output, size, initial_value);
#       elif defined(__AVX2__)
        return encode_delta_zig_zag_avx2_i32(input, output, size, initial_value);
#       elif defined(__SSE2__)
        return encode_delta_zig_zag_scalar_i32(input, output, size, initial_value);
#       else
        return encode_delta_zig_zag_scalar_i32(input, output, size, initial_value);
#       endif
    }
    
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

    /// \brief Decodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c decode_delta_zig_zag_scalar_i32.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void decode_delta_zig_zag_scalar_i32(
            const std::uint32_t* input,
            std::int32_t* output,
            std::size_t size,
            std::int32_t initial_value
        ) noexcept {
        if (size == 0) return;
        std::int32_t base = static_cast<std::int32_t>(initial_value + zigzag_decode_u32(input[0]));
        output[0] = base;
        for (std::size_t i = 1; i < size; ++i) {
            base = static_cast<std::int32_t>(base + zigzag_decode_u32(input[i]));
            output[i] = base;
        }
    }

    /// \brief Decodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c decode_delta_zig_zag_scalar_u32.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void decode_delta_zig_zag_scalar_u32(
        const std::uint32_t* input,
        std::uint32_t* output,
        std::size_t size,
        std::uint32_t initial_value
        ) noexcept {
        if (size == 0) return;
        std::uint32_t base = static_cast<std::uint32_t>(static_cast<std::int64_t>(initial_value) +
                                                        static_cast<std::int64_t>(zigzag_decode_u32(input[0])));
        output[0] = base;
        for (std::size_t i = 1; i < size; ++i) {
            base = static_cast<std::uint32_t>(static_cast<std::int64_t>(base) +
                                              static_cast<std::int64_t>(zigzag_decode_u32(input[i])));
            output[i] = base;
        }
    }


#   if defined(__SSE2__)

    /// \brief Decodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c decode_delta_zig_zag_sse2_i32.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void decode_delta_zig_zag_sse2_i32(
            const std::uint32_t* input,
            std::int32_t* output,
            std::size_t size,
            std::int32_t initial_value
        ) noexcept {
        if (size == 0) return;

        std::size_t i = 0;
        std::int32_t base = static_cast<std::int32_t>(initial_value + zigzag_decode_u32(input[0]));
        output[0] = base;
        i = 1;

        // align input/output for aligned loads/stores (16 bytes)
        constexpr std::size_t align = 16;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            base = static_cast<std::int32_t>(base + zigzag_decode_u32(input[i]));
            output[i] = base;
        }

        constexpr std::size_t W = 4;
        const std::size_t simd_end = i + ((size - i) / W) * W;

        for (; i < simd_end; i += W) {
            __m128i z = _mm_load_si128(reinterpret_cast<const __m128i*>(input + i));
            __m128i d = zigzag_decode_u32_sse2(z);

            // inclusive prefix sum in 128: [d0,d1,d2,d3] -> [d0, d0+d1, d0+d1+d2, d0+d1+d2+d3]
            __m128i t = d;
            t = _mm_add_epi32(t, _mm_slli_si128(t, 4));
            t = _mm_add_epi32(t, _mm_slli_si128(t, 8));

            __m128i b = _mm_set1_epi32(base);
            __m128i outv = _mm_add_epi32(t, b);

            _mm_store_si128(reinterpret_cast<__m128i*>(output + i), outv);

            base = _mm_cvtsi128_si32(_mm_shuffle_epi32(outv, _MM_SHUFFLE(3,3,3,3)));
        }

        for (; i < size; ++i) {
            base = static_cast<std::int32_t>(base + zigzag_decode_u32(input[i]));
            output[i] = base;
        }
    }

    /// \brief Decodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c decode_delta_zig_zag_sse2_u32.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void decode_delta_zig_zag_sse2_u32(
            const std::uint32_t* input,
            std::uint32_t* output,
            std::size_t size,
            std::uint32_t initial_value
        ) noexcept {
        if (size == 0) return;

        std::size_t i = 0;
        std::uint32_t base_u = static_cast<std::uint32_t>(static_cast<std::int64_t>(initial_value) +
                                                          static_cast<std::int64_t>(zigzag_decode_u32(input[0])));
        output[0] = base_u;
        i = 1;

        // We keep base in int32 register (bitwise identical for add mod 2^32)
        std::int32_t base = static_cast<std::int32_t>(base_u);

        constexpr std::size_t align = 16;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            base = static_cast<std::int32_t>(base + zigzag_decode_u32(input[i]));
            output[i] = static_cast<std::uint32_t>(base);
        }

        constexpr std::size_t W = 4;
        const std::size_t simd_end = i + ((size - i) / W) * W;

        for (; i < simd_end; i += W) {
            __m128i z = _mm_load_si128(reinterpret_cast<const __m128i*>(input + i));
            __m128i d = zigzag_decode_u32_sse2(z);

            __m128i t = d;
            t = _mm_add_epi32(t, _mm_slli_si128(t, 4));
            t = _mm_add_epi32(t, _mm_slli_si128(t, 8));

            __m128i b = _mm_set1_epi32(base);
            __m128i outv = _mm_add_epi32(t, b);

            _mm_store_si128(reinterpret_cast<__m128i*>(output + i), outv);

            base = _mm_cvtsi128_si32(_mm_shuffle_epi32(outv, _MM_SHUFFLE(3,3,3,3)));
        }

        for (; i < size; ++i) {
            base = static_cast<std::int32_t>(base + zigzag_decode_u32(input[i]));
            output[i] = static_cast<std::uint32_t>(base);
        }
    }
#   endif

#   if defined(__AVX2__)
    
    /// \brief Decodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c decode_delta_zig_zag_avx2_i32.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void decode_delta_zig_zag_avx2_i32(
            const std::uint32_t* input,
            std::int32_t* output,
            std::size_t size,
            std::int32_t initial_value
        ) noexcept {
        if (size == 0) return;

        std::size_t i = 1;
        std::int32_t base = static_cast<std::int32_t>(initial_value + zigzag_decode_u32(input[0]));
        output[0] = base;

        constexpr std::size_t align = 32;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            base = static_cast<std::int32_t>(base + zigzag_decode_u32(input[i]));
            output[i] = base;
        }

        constexpr std::size_t W = 8;
        const std::size_t simd_end = i + ((size - i) / W) * W;

        for (; i < simd_end; i += W) {
            __m256i z = _mm256_load_si256(reinterpret_cast<const __m256i*>(input + i));
            __m256i d = zigzag_decode_u32_avx2(z);

            // prefix within each 128 lane (4 elems)
            __m256i t = d;
            t = _mm256_add_epi32(t, _mm256_slli_si256(t, 4));
            t = _mm256_add_epi32(t, _mm256_slli_si256(t, 8));

            // cross-lane carry (low lane sum to high lane)
            __m128i lo = _mm256_castsi256_si128(t);
            int lo_sum = _mm_cvtsi128_si32(_mm_shuffle_epi32(lo, _MM_SHUFFLE(3,3,3,3)));
            __m128i hi = _mm256_extracti128_si256(t, 1);
            hi = _mm_add_epi32(hi, _mm_set1_epi32(lo_sum));
            __m256i scan = _mm256_inserti128_si256(_mm256_castsi128_si256(lo), hi, 1);

            __m256i b = _mm256_set1_epi32(base);
            __m256i outv = _mm256_add_epi32(scan, b);

            _mm256_store_si256(reinterpret_cast<__m256i*>(output + i), outv);

            __m128i out_hi = _mm256_extracti128_si256(outv, 1);
            base = _mm_cvtsi128_si32(_mm_shuffle_epi32(out_hi, _MM_SHUFFLE(3,3,3,3)));
        }

        for (; i < size; ++i) {
            base = static_cast<std::int32_t>(base + zigzag_decode_u32(input[i]));
            output[i] = base;
        }
    }

    /// \brief Decodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c decode_delta_zig_zag_avx2_u32.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void decode_delta_zig_zag_avx2_u32(
            const std::uint32_t* input,
            std::uint32_t* output,
            std::size_t size,
            std::uint32_t initial_value
        ) noexcept {
        if (size == 0) return;

        std::size_t i = 1;
        std::uint32_t base_u = static_cast<std::uint32_t>(static_cast<std::int64_t>(initial_value) +
                                                          static_cast<std::int64_t>(zigzag_decode_u32(input[0])));
        output[0] = base_u;
        std::int32_t base = static_cast<std::int32_t>(base_u);

        constexpr std::size_t align = 32;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            base = static_cast<std::int32_t>(base + zigzag_decode_u32(input[i]));
            output[i] = static_cast<std::uint32_t>(base);
        }

        constexpr std::size_t W = 8;
        const std::size_t simd_end = i + ((size - i) / W) * W;

        for (; i < simd_end; i += W) {
            __m256i z = _mm256_load_si256(reinterpret_cast<const __m256i*>(input + i));
            __m256i d = zigzag_decode_u32_avx2(z);

            __m256i t = d;
            t = _mm256_add_epi32(t, _mm256_slli_si256(t, 4));
            t = _mm256_add_epi32(t, _mm256_slli_si256(t, 8));

            __m128i lo = _mm256_castsi256_si128(t);
            int lo_sum = _mm_cvtsi128_si32(_mm_shuffle_epi32(lo, _MM_SHUFFLE(3,3,3,3)));
            __m128i hi = _mm256_extracti128_si256(t, 1);
            hi = _mm_add_epi32(hi, _mm_set1_epi32(lo_sum));
            __m256i scan = _mm256_inserti128_si256(_mm256_castsi128_si256(lo), hi, 1);

            __m256i b = _mm256_set1_epi32(base);
            __m256i outv = _mm256_add_epi32(scan, b);

            _mm256_store_si256(reinterpret_cast<__m256i*>(output + i), outv);

            __m128i out_hi = _mm256_extracti128_si256(outv, 1);
            base = _mm_cvtsi128_si32(_mm_shuffle_epi32(out_hi, _MM_SHUFFLE(3,3,3,3)));
        }

        for (; i < size; ++i) {
            base = static_cast<std::int32_t>(base + zigzag_decode_u32(input[i]));
            output[i] = static_cast<std::uint32_t>(base);
        }
    }
#   endif

#   if defined(__AVX512F__)
    /// \brief Decodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c decode_delta_zig_zag_avx512_i32.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void decode_delta_zig_zag_avx512_i32(
            const std::uint32_t* input,
            std::int32_t* output,
            std::size_t size,
            std::int32_t initial_value
        ) noexcept {
        if (size == 0) return;

        std::size_t i = 0;
        std::int32_t base = static_cast<std::int32_t>(initial_value + zigzag_decode_u32_scalar(input[0]));
        output[0] = base;
        i = 1;

        constexpr std::size_t align = 64;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            base = static_cast<std::int32_t>(base + zigzag_decode_u32_scalar(input[i]));
            output[i] = base;
        }

        constexpr std::size_t W = 16;
        const std::size_t simd_end = i + ((size - i) / W) * W;

        static const __m512i idx1 = _mm512_setr_epi32(0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14);
        static const __m512i idx2 = _mm512_setr_epi32(0,0,0,1,2,3,4,5,6,7,8,9,10,11,12,13);
        static const __m512i idx4 = _mm512_setr_epi32(0,0,0,0,0,1,2,3,4,5,6,7,8,9,10,11);
        static const __m512i idx8 = _mm512_setr_epi32(0,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7);

        for (; i < simd_end; i += W) {
            __m512i z = _mm512_load_si512(reinterpret_cast<const void*>(input + i));
            __m512i t = zigzag_decode_u32_avx512(z);

            t = _mm512_mask_add_epi32(t, 0xFFFEu, t, _mm512_permutexvar_epi32(idx1, t));
            t = _mm512_mask_add_epi32(t, 0xFFFCu, t, _mm512_permutexvar_epi32(idx2, t));
            t = _mm512_mask_add_epi32(t, 0xFFF0u, t, _mm512_permutexvar_epi32(idx4, t));
            t = _mm512_mask_add_epi32(t, 0xFF00u, t, _mm512_permutexvar_epi32(idx8, t));

            __m512i outv = _mm512_add_epi32(t, _mm512_set1_epi32(base));
            _mm512_store_si512(reinterpret_cast<void*>(output + i), outv);

            const __m512i idx_last = _mm512_set1_epi32(15);
            base = _mm_cvtsi128_si32(_mm512_castsi512_si128(_mm512_permutexvar_epi32(idx_last, outv)));
        }

        if (i < size) {
            const std::size_t rem = size - i;
            const __mmask16 tail_mask = static_cast<__mmask16>((1u << rem) - 1u);

            __m512i z = _mm512_maskz_loadu_epi32(tail_mask, input + i);
            __m512i t = zigzag_decode_u32_avx512(z);

            t = _mm512_mask_add_epi32(t, 0xFFFEu, t, _mm512_permutexvar_epi32(idx1, t));
            t = _mm512_mask_add_epi32(t, 0xFFFCu, t, _mm512_permutexvar_epi32(idx2, t));
            t = _mm512_mask_add_epi32(t, 0xFFF0u, t, _mm512_permutexvar_epi32(idx4, t));
            t = _mm512_mask_add_epi32(t, 0xFF00u, t, _mm512_permutexvar_epi32(idx8, t));

            __m512i outv = _mm512_add_epi32(t, _mm512_set1_epi32(base));
            _mm512_mask_storeu_epi32(output + i, tail_mask, outv);
        }
    }

    /// \brief Decodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c decode_delta_zig_zag_avx512_u32.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void decode_delta_zig_zag_avx512_u32(
            const std::uint32_t* input,
            std::uint32_t* output,
            std::size_t size,
            std::uint32_t initial_value
        ) noexcept {
        if (size == 0) return;

        std::size_t i = 0;
        std::uint32_t base_u = static_cast<std::uint32_t>(static_cast<std::int64_t>(initial_value) +
                                                          static_cast<std::int64_t>(zigzag_decode_u32_scalar(input[0])));
        output[0] = base_u;
        i = 1;
        std::int32_t base = static_cast<std::int32_t>(base_u);

        constexpr std::size_t align = 64;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            base = static_cast<std::int32_t>(base + zigzag_decode_u32_scalar(input[i]));
            output[i] = static_cast<std::uint32_t>(base);
        }

        constexpr std::size_t W = 16;
        const std::size_t simd_end = i + ((size - i) / W) * W;

        static const __m512i idx1 = _mm512_setr_epi32(0,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14);
        static const __m512i idx2 = _mm512_setr_epi32(0,0,0,1,2,3,4,5,6,7,8,9,10,11,12,13);
        static const __m512i idx4 = _mm512_setr_epi32(0,0,0,0,0,1,2,3,4,5,6,7,8,9,10,11);
        static const __m512i idx8 = _mm512_setr_epi32(0,0,0,0,0,0,0,0,0,1,2,3,4,5,6,7);

        for (; i < simd_end; i += W) {
            __m512i z = _mm512_load_si512(reinterpret_cast<const void*>(input + i));
            __m512i t = zigzag_decode_u32_avx512(z);

            t = _mm512_mask_add_epi32(t, 0xFFFEu, t, _mm512_permutexvar_epi32(idx1, t));
            t = _mm512_mask_add_epi32(t, 0xFFFCu, t, _mm512_permutexvar_epi32(idx2, t));
            t = _mm512_mask_add_epi32(t, 0xFFF0u, t, _mm512_permutexvar_epi32(idx4, t));
            t = _mm512_mask_add_epi32(t, 0xFF00u, t, _mm512_permutexvar_epi32(idx8, t));

            __m512i outv = _mm512_add_epi32(t, _mm512_set1_epi32(base));
            _mm512_store_si512(reinterpret_cast<void*>(output + i), outv);

            const __m512i idx_last = _mm512_set1_epi32(15);
            base = _mm_cvtsi128_si32(_mm512_castsi512_si128(_mm512_permutexvar_epi32(idx_last, outv)));
        }

        if (i < size) {
            const std::size_t rem = size - i;
            const __mmask16 tail_mask = static_cast<__mmask16>((1u << rem) - 1u);

            __m512i z = _mm512_maskz_loadu_epi32(tail_mask, input + i);
            __m512i t = zigzag_decode_u32_avx512(z);

            t = _mm512_mask_add_epi32(t, 0xFFFEu, t, _mm512_permutexvar_epi32(idx1, t));
            t = _mm512_mask_add_epi32(t, 0xFFFCu, t, _mm512_permutexvar_epi32(idx2, t));
            t = _mm512_mask_add_epi32(t, 0xFFF0u, t, _mm512_permutexvar_epi32(idx4, t));
            t = _mm512_mask_add_epi32(t, 0xFF00u, t, _mm512_permutexvar_epi32(idx8, t));

            __m512i outv = _mm512_add_epi32(t, _mm512_set1_epi32(base));
            _mm512_mask_storeu_epi32(output + i, tail_mask, outv);
        }
    }

#   endif


    /// \brief Performs delta and Zig-Zag decoding in a single pass.
    /// \param input Pointer to the encoded array.
    /// \param output Pointer to the decoded array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for reconstruction.
    inline void decode_delta_zig_zag_i32(
            const std::uint32_t* input,
            std::int32_t* output,
            std::size_t size,
            std::int32_t initial_value
        ) noexcept {
#       if defined(__AVX512F__)
        decode_delta_zig_zag_avx512_i32(input, output, size, initial_value);
#       elif defined(__AVX2__)
        decode_delta_zig_zag_avx2_i32(input, output, size, initial_value);
#       elif defined(__SSE2__)
        decode_delta_zig_zag_sse2_i32(input, output, size, initial_value);
#       else
        decode_delta_zig_zag_scalar_i32(input, output, size, initial_value);
#       endif
    }

    /// \brief Decodes a delta+ZigZag sequence (dispatcher).
    /// \note Function: \c decode_delta_zig_zag_u32.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void decode_delta_zig_zag_u32(
            const std::uint32_t* input,
            std::uint32_t* output,
            std::size_t size,
            std::uint32_t initial_value
        ) noexcept {
#       if defined(__AVX512F__)
        decode_delta_zig_zag_avx512_u32(input, output, size, initial_value);
#       elif defined(__AVX2__)
        decode_delta_zig_zag_avx2_u32(input, output, size, initial_value);
#       elif defined(__SSE2__)
        decode_delta_zig_zag_sse2_u32(input, output, size, initial_value);
#       else
        decode_delta_zig_zag_scalar_u32(input, output, size, initial_value);
#       endif
    }

    /// @}

//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

    /// \name Delta+ZigZag integer codecs (64-bit paths)
    /// \brief Backend-specific and dispatcher helpers for 64-bit encode/decode.
    /// \note Backends: scalar, SSE2, AVX2, AVX512F (where available).
    /// @{

    /// \brief Encodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c encode_delta_zig_zag_scalar_i64.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void encode_delta_zig_zag_scalar_i64(
            const std::int64_t* input,
            std::uint64_t* output,
            std::size_t size,
            std::int64_t initial_value
        ) noexcept {
        std::int64_t cur;
        for (std::size_t i = 0; i < size; ++i) {
            cur = input[i];
            output[i] = zigzag_encode_u64(cur - initial_value);
            initial_value = cur;
        }
    }
    
    /// \brief Encodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c encode_delta_zig_zag_scalar_u64.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void encode_delta_zig_zag_scalar_u64(
            const std::uint64_t* input,
            std::uint64_t* output,
            std::size_t size,
            std::uint64_t initial_value
        ) noexcept {
        encode_delta_zig_zag_scalar_i64(reinterpret_cast<const std::int64_t*>(input), output, size,
                                        static_cast<std::int64_t>(initial_value));
    }
    
#   if defined(__SSE2__)

    /// \brief Encodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c encode_delta_zig_zag_sse2_i64.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void encode_delta_zig_zag_sse2_i64(
            const std::int64_t* input,
            std::uint64_t* output,
            std::size_t size,
            std::int64_t initial_value
        ) noexcept {
        std::size_t i = 0;
        std::int64_t prev = initial_value;

        constexpr std::size_t align = 16;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            const std::int64_t cur = input[i];
            const std::int64_t d   = static_cast<std::int64_t>(static_cast<std::uint64_t>(cur) -
                                                               static_cast<std::uint64_t>(prev));
            output[i] = zigzag_encode_scalar_u64(d);
            prev = cur;
        }

        constexpr std::size_t W = 2;
        const std::size_t end = i + ((size - i) / W) * W;

        for (; i < end; i += W) {
            __m128i cur = _mm_load_si128(reinterpret_cast<const __m128i*>(input + i));

            __m128i prevv = _mm_slli_si128(cur, 8);
            prevv = _mm_or_si128(prevv, _mm_cvtsi64_si128(prev));

            __m128i d = _mm_sub_epi64(cur, prevv);
            __m128i zz = zigzag_encode_u64_sse2(d);

            _mm_store_si128(reinterpret_cast<__m128i*>(output + i), zz);

            prev = _mm_cvtsi128_si64(_mm_srli_si128(cur, 8));
        }

        for (; i < size; ++i) {
            const std::int64_t cur = input[i];
            const std::int64_t d   = static_cast<std::int64_t>(static_cast<std::uint64_t>(cur) -
                                                               static_cast<std::uint64_t>(prev));
            output[i] = zigzag_encode_scalar_u64(d);
            prev = cur;
        }
    }
    
    /// \brief Encodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c encode_delta_zig_zag_sse2_u64.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void encode_delta_zig_zag_sse2_u64(
            const std::uint64_t* input,
            std::uint64_t* output,
            std::size_t size,
            std::uint64_t initial_value
        ) noexcept {
        encode_delta_zig_zag_sse2_i64(reinterpret_cast<const std::int64_t*>(input), output, size,
                                      static_cast<std::int64_t>(initial_value));
    }
#   endif


#   if defined(__AVX2__)

    /// \brief Encodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c encode_delta_zig_zag_avx2_i64.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void encode_delta_zig_zag_avx2_i64(
            const std::int64_t* input,
            std::uint64_t* output,
            std::size_t size,
            std::int64_t initial_value
        ) noexcept {
        std::size_t i = 0;
        std::int64_t prev = initial_value;

        constexpr std::size_t align = 32;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            const std::int64_t cur = input[i];
            const std::int64_t d   = static_cast<std::int64_t>(static_cast<std::uint64_t>(cur) -
                                                               static_cast<std::uint64_t>(prev));
            output[i] = zigzag_encode_scalar_u64(d);
            prev = cur;
        }

        constexpr std::size_t W = 4;
        const std::size_t end = i + ((size - i) / W) * W;

        for (; i < end; i += W) {
            __m256i cur = _mm256_load_si256(reinterpret_cast<const __m256i*>(input + i));

            __m256i sh = _mm256_permute4x64_epi64(cur, _MM_SHUFFLE(2,1,0,3)); // [cur3,cur0,cur1,cur2]
            __m256i prevv = _mm256_blend_epi32(_mm256_set1_epi64x(prev), sh, 0xFC);

            __m256i d = _mm256_sub_epi64(cur, prevv);
            __m256i zz = zigzag_encode_u64_avx2(d);

            _mm256_store_si256(reinterpret_cast<__m256i*>(output + i), zz);

            prev = static_cast<std::int64_t>(_mm256_extract_epi64(cur, 3));
        }

        for (; i < size; ++i) {
            const std::int64_t cur = input[i];
            const std::int64_t d   = static_cast<std::int64_t>(static_cast<std::uint64_t>(cur) -
                                                               static_cast<std::uint64_t>(prev));
            output[i] = zigzag_encode_scalar_u64(d);
            prev = cur;
        }
    }

    /// \brief Encodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c encode_delta_zig_zag_avx2_u64.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void encode_delta_zig_zag_avx2_u64(
            const std::uint64_t* input,
            std::uint64_t* output,
            std::size_t size,
            std::uint64_t initial_value
        ) noexcept {
        encode_delta_zig_zag_avx2_i64(reinterpret_cast<const std::int64_t*>(input), output, size,
                                      static_cast<std::int64_t>(initial_value));
    }
#   endif

#if defined(__AVX512F__)
    
    /// \brief Encodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c encode_delta_zig_zag_avx512_i64.
    /// \note Not selected by dispatchers; kept for micro-benchmarks / ISA experiments.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void encode_delta_zig_zag_avx512_i64(
            const std::int64_t* input,
            std::uint64_t* output,
            std::size_t size,
            std::int64_t initial_value
        ) noexcept {
        std::size_t i = 0;
        std::int64_t prev = initial_value;

        constexpr std::size_t align = 64;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            const std::int64_t cur = input[i];
            const std::int64_t d   = static_cast<std::int64_t>(static_cast<std::uint64_t>(cur) -
                                                               static_cast<std::uint64_t>(prev));
            output[i] = zigzag_encode_scalar_u64(d);
            prev = cur;
        }

        constexpr std::size_t W = 8;
        const std::size_t end = i + ((size - i) / W) * W;

        alignas(64) std::int64_t cur_lane[W];
        alignas(64) std::int64_t d_lane[W];

        for (; i < end; i += W) {
            __m512i cur = _mm512_load_si512(reinterpret_cast<const void*>(input + i));
            _mm512_store_si512(reinterpret_cast<void*>(cur_lane), cur);

            std::int64_t p = prev;
            for (std::size_t k = 0; k < W; ++k) {
                const std::int64_t c = cur_lane[k];
                d_lane[k] = static_cast<std::int64_t>(static_cast<std::uint64_t>(c) -
                                                      static_cast<std::uint64_t>(p));
                p = c;
            }
            prev = cur_lane[W - 1];

            __m512i d = _mm512_load_si512(reinterpret_cast<const void*>(d_lane));
            __m512i zz = zigzag_encode_u64_avx512(d);
            _mm512_store_si512(reinterpret_cast<void*>(output + i), zz);
        }

        for (; i < size; ++i) {
            const std::int64_t cur = input[i];
            const std::int64_t d   = static_cast<std::int64_t>(static_cast<std::uint64_t>(cur) -
                                                               static_cast<std::uint64_t>(prev));
            output[i] = zigzag_encode_scalar_u64(d);
            prev = cur;
        }
    }
    
    /// \brief Encodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c encode_delta_zig_zag_avx512_u64.
    /// \note Not selected by dispatchers; kept for micro-benchmarks / ISA experiments.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void encode_delta_zig_zag_avx512_u64(
            const std::uint64_t* input,
            std::uint64_t* output,
            std::size_t size,
            std::uint64_t initial_value
        ) noexcept {
        encode_delta_zig_zag_avx512_i64(reinterpret_cast<const std::int64_t*>(input), output, size,
                                        static_cast<std::int64_t>(initial_value));
    }
#   endif


    /// \brief Performs delta and Zig-Zag encoding in a single pass (64-bit unsigned interface).
    /// \param input Pointer to the input array (uint64_t).
    /// \param output Pointer to the output array (uint64_t).
    /// \param size Number of elements in the array.
    /// \param initial_value The reference value for delta computation (64-bit).
    /// \pre Each mathematical delta `input[i] - prev` (with `prev = initial_value` for i=0,
    ///      otherwise `prev = input[i-1]`) must fit in `int64_t`.
    /// \note Implementation reuses signed-delta internals; values are represented as `uint64_t`,
    ///       but the delta domain is effectively `int64_t`.
    /// \thread_safety Thread-safe (pure function over caller-provided buffers).
    inline void encode_delta_zig_zag_u64(
            const std::uint64_t* input,
            std::uint64_t* output,
            std::size_t size,
            std::uint64_t initial_value) noexcept {
#       if defined(__AVX2__)
        encode_delta_zig_zag_avx2_u64(input, output, size, initial_value);
#       elif defined(__AVX512F__)
        encode_delta_zig_zag_scalar_u64(input, output, size, initial_value);
#       elif defined(__SSE2__)
        encode_delta_zig_zag_sse2_u64(input, output, size, initial_value);
#       else
        encode_delta_zig_zag_scalar_u64(input, output, size, initial_value);
#       endif
    }

    /// \brief Encodes a delta+ZigZag sequence (dispatcher).
    /// \note Function: \c encode_delta_zig_zag_i64.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void encode_delta_zig_zag_i64(
        const std::int64_t* input,
        std::uint64_t* output,
        std::size_t size,
        std::int64_t initial_value
    ) noexcept {
#       if defined(__AVX2__)
        encode_delta_zig_zag_avx2_i64(input, output, size, initial_value);
#       elif defined(__AVX512F__)
        encode_delta_zig_zag_scalar_i64(input, output, size, initial_value);
#       elif defined(__SSE2__)
        encode_delta_zig_zag_sse2_i64(input, output, size, initial_value);
#       else
        encode_delta_zig_zag_scalar_i64(input, output, size, initial_value);
#       endif
    }
    
//------------------------------------------------------------------------------
//
//------------------------------------------------------------------------------

    /// \brief Decodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c decode_delta_zig_zag_scalar_i64.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void decode_delta_zig_zag_scalar_i64(
            const std::uint64_t* input,
            std::int64_t* output,
            std::size_t size,
            std::int64_t initial_value
        ) noexcept {
        if (size == 0) return;
        std::int64_t base = initial_value + zigzag_decode_u64(input[0]);
        output[0] = base;
        for (std::size_t i = 1; i < size; ++i) {
            base += zigzag_decode_u64(input[i]);
            output[i] = base;
        }
    }
    
    /// \brief Decodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c decode_delta_zig_zag_scalar_u64.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void decode_delta_zig_zag_scalar_u64(
            const std::uint64_t* input,
            std::uint64_t* output,
            std::size_t size,
            std::int64_t initial_value
        ) noexcept {
        decode_delta_zig_zag_scalar_i64(input, reinterpret_cast<std::int64_t*>(output), size, initial_value);
    }

#   if defined(__SSE2__)
    /// \brief Decodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c decode_delta_zig_zag_sse2_i64.
    /// \note Kept for benchmarks/manual comparison; release dispatcher uses scalar on SSE2-only targets
    ///       because this backend does not provide stable speedup vs scalar.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void decode_delta_zig_zag_sse2_i64(
            const std::uint64_t* input,
            std::int64_t* output,
            std::size_t size,
            std::int64_t initial_value
        ) noexcept {
        if (size == 0) return;
        std::size_t i = 1;
        std::int64_t base = initial_value + zigzag_decode_u64(input[0]);
        output[0] = base;
        constexpr std::size_t align = 16;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            base += zigzag_decode_u64(input[i]);
            output[i] = base;
        }

        constexpr std::size_t W = 2;
        const std::size_t end = i + ((size - i) / W) * W;

        for (; i < end; i += W) {
            __m128i z = _mm_load_si128(reinterpret_cast<const __m128i*>(input + i));
            __m128i d = zigzag_decode_u64_sse2(z);

            // prefix within 128: [a,b] -> [a, a+b]
            __m128i t = d;
            t = _mm_add_epi64(t, _mm_slli_si128(t, 8));

            __m128i b = _mm_set1_epi64x(base);
            __m128i outv = _mm_add_epi64(t, b);

            _mm_store_si128(reinterpret_cast<__m128i*>(output + i), outv);

            base = _mm_cvtsi128_si64(_mm_srli_si128(outv, 8));
        }

        for (; i < size; ++i) {
            base += zigzag_decode_u64(input[i]);
            output[i] = base;
        }
    }

    /// \brief Decodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c decode_delta_zig_zag_sse2_u64.
    /// \note Kept for benchmarks/manual comparison; release dispatcher uses scalar on SSE2-only targets
    ///       because this backend does not provide stable speedup vs scalar.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void decode_delta_zig_zag_sse2_u64(
            const std::uint64_t* input,
            std::uint64_t* output,
            std::size_t size,
            std::int64_t initial_value
        ) noexcept {
        decode_delta_zig_zag_sse2_i64(input, reinterpret_cast<std::int64_t*>(output), size, initial_value);
    }
#   endif

#if defined(__AVX2__)
    
    /// \brief Decodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c decode_delta_zig_zag_avx2_i64.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void decode_delta_zig_zag_avx2_i64(
            const std::uint64_t* input,
            std::int64_t* output,
            std::size_t size,
            std::int64_t initial_value
        ) noexcept {
        if (size == 0) return;

        std::size_t i = 1;
        std::int64_t base = initial_value + zigzag_decode_u64(input[0]);
        output[0] = base;

        constexpr std::size_t align = 32;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            base += zigzag_decode_u64(input[i]);
            output[i] = base;
        }

        constexpr std::size_t W = 4;
        const std::size_t end = i + ((size - i) / W) * W;
        const std::size_t end2 = i + ((end - i) / (W * 2)) * (W * 2);

        for (; i < end2; i += (W * 2)) {
            __m256i z0 = _mm256_load_si256(reinterpret_cast<const __m256i*>(input + i));
            __m256i z1 = _mm256_load_si256(reinterpret_cast<const __m256i*>(input + i + W));

            __m256i d0 = zigzag_decode_u64_avx2(z0);
            __m256i d1 = zigzag_decode_u64_avx2(z1);

            __m256i t0 = _mm256_add_epi64(d0, _mm256_slli_si256(d0, 8));
            const __m256i low_last_broadcast0 = _mm256_permute4x64_epi64(t0, 0x55);
            const __m256i add_hi0 = _mm256_blend_epi32(_mm256_setzero_si256(), low_last_broadcast0, 0xF0);
            t0 = _mm256_add_epi64(t0, add_hi0);

            const std::int64_t base_next = base + static_cast<std::int64_t>(_mm256_extract_epi64(t0, 3));
            __m256i outv0 = _mm256_add_epi64(t0, _mm256_set1_epi64x(base));
            _mm256_store_si256(reinterpret_cast<__m256i*>(output + i), outv0);

            __m256i t1 = _mm256_add_epi64(d1, _mm256_slli_si256(d1, 8));
            const __m256i low_last_broadcast1 = _mm256_permute4x64_epi64(t1, 0x55);
            const __m256i add_hi1 = _mm256_blend_epi32(_mm256_setzero_si256(), low_last_broadcast1, 0xF0);
            t1 = _mm256_add_epi64(t1, add_hi1);
            __m256i outv1 = _mm256_add_epi64(t1, _mm256_set1_epi64x(base_next));
            _mm256_store_si256(reinterpret_cast<__m256i*>(output + i + W), outv1);
            base = base_next + static_cast<std::int64_t>(_mm256_extract_epi64(t1, 3));
        }

        for (; i < end; i += W) {
            __m256i z = _mm256_load_si256(reinterpret_cast<const __m256i*>(input + i));
            __m256i d = zigzag_decode_u64_avx2(z);
            __m256i t = _mm256_add_epi64(d, _mm256_slli_si256(d, 8));
            const __m256i low_last_broadcast = _mm256_permute4x64_epi64(t, 0x55);
            const __m256i add_hi = _mm256_blend_epi32(_mm256_setzero_si256(), low_last_broadcast, 0xF0);
            t = _mm256_add_epi64(t, add_hi);
            __m256i outv = _mm256_add_epi64(t, _mm256_set1_epi64x(base));
            _mm256_store_si256(reinterpret_cast<__m256i*>(output + i), outv);
            base = static_cast<std::int64_t>(_mm256_extract_epi64(outv, 3));
        }

        for (; i < size; ++i) {
            base += zigzag_decode_u64(input[i]);
            output[i] = base;
        }
    }

    /// \brief Decodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c decode_delta_zig_zag_avx2_u64.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void decode_delta_zig_zag_avx2_u64(
            const std::uint64_t* input,
            std::uint64_t* output,
            std::size_t size,
            std::int64_t initial_value
        ) noexcept {
        decode_delta_zig_zag_avx2_i64(input, reinterpret_cast<std::int64_t*>(output), size, initial_value);
    }
#   endif

#   if defined(__AVX512F__)

    /// \brief Decodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c decode_delta_zig_zag_avx512_i64.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void decode_delta_zig_zag_avx512_i64(
            const std::uint64_t* input,
            std::int64_t* output,
            std::size_t size,
            std::int64_t initial_value
        ) noexcept {
        if (size == 0) return;

        std::size_t i = 1;
        std::int64_t base = initial_value + zigzag_decode_u64(input[0]);
        output[0] = base;

        constexpr std::size_t align = 64;
        for (; i < size; ++i) {
            if ((reinterpret_cast<std::uintptr_t>(input + i)  % align) == 0 &&
                (reinterpret_cast<std::uintptr_t>(output + i) % align) == 0) break;
            base += zigzag_decode_u64(input[i]);
            output[i] = base;
        }

        constexpr std::size_t W = 8;
        const std::size_t end = i + ((size - i) / W) * W;

        static const __m512i idx1 = _mm512_setr_epi64(0,0,1,2,3,4,5,6);
        static const __m512i idx2 = _mm512_setr_epi64(0,0,0,1,2,3,4,5);
        static const __m512i idx4 = _mm512_setr_epi64(0,0,0,0,0,1,2,3);

        for (; i < end; i += W) {
            __m512i z = _mm512_load_si512(reinterpret_cast<const void*>(input + i));
            __m512i t = zigzag_decode_u64_avx512(z);

            t = _mm512_mask_add_epi64(t, 0xFEu, t, _mm512_permutexvar_epi64(idx1, t));
            t = _mm512_mask_add_epi64(t, 0xFCu, t, _mm512_permutexvar_epi64(idx2, t));
            t = _mm512_mask_add_epi64(t, 0xF0u, t, _mm512_permutexvar_epi64(idx4, t));

            __m512i outv = _mm512_add_epi64(t, _mm512_set1_epi64(base));
            _mm512_store_si512(reinterpret_cast<void*>(output + i), outv);

            const __m512i idx_last = _mm512_set1_epi64(7);
            base = _mm_cvtsi128_si64(_mm512_castsi512_si128(_mm512_permutexvar_epi64(idx_last, outv)));
        }

        if (i < size) {
            const std::size_t rem = size - i;
            const __mmask8 tail_mask = static_cast<__mmask8>((1u << rem) - 1u);

            __m512i z = _mm512_maskz_loadu_epi64(tail_mask, input + i);
            __m512i t = zigzag_decode_u64_avx512(z);

            t = _mm512_mask_add_epi64(t, 0xFEu, t, _mm512_permutexvar_epi64(idx1, t));
            t = _mm512_mask_add_epi64(t, 0xFCu, t, _mm512_permutexvar_epi64(idx2, t));
            t = _mm512_mask_add_epi64(t, 0xF0u, t, _mm512_permutexvar_epi64(idx4, t));

            __m512i outv = _mm512_add_epi64(t, _mm512_set1_epi64(base));
            _mm512_mask_storeu_epi64(output + i, tail_mask, outv);
        }
    }

    /// \brief Decodes a delta+ZigZag sequence (backend implementation).
    /// \note Function: \c decode_delta_zig_zag_avx512_u64.
    /// \thread_safety Thread-safe (no shared mutable state).

    inline void decode_delta_zig_zag_avx512_u64(
            const std::uint64_t* input,
            std::uint64_t* output,
            std::size_t size,
            std::int64_t initial_value
        ) noexcept {
        decode_delta_zig_zag_avx512_i64(input, reinterpret_cast<std::int64_t*>(output), size, initial_value);
    }

#   endif

    /// \brief Performs delta and Zig-Zag decoding in a single pass for 64-bit integers.
    /// \param input Pointer to the encoded array.
    /// \param output Pointer to the decoded array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for reconstruction.
    inline void decode_delta_zig_zag_i64(
            const std::uint64_t* input,
            std::int64_t* output,
            std::size_t size,
            std::int64_t initial_value
        ) noexcept {
#       if defined(__AVX512F__)
        decode_delta_zig_zag_avx512_i64(input, output, size, initial_value);
#       elif defined(__AVX2__)
        decode_delta_zig_zag_avx2_i64(input, output, size, initial_value);
#       elif defined(__SSE2__)
        decode_delta_zig_zag_scalar_i64(input, output, size, initial_value);
#       else
        decode_delta_zig_zag_scalar_i64(input, output, size, initial_value);
#       endif
    }

    /// \brief Performs delta and Zig-Zag decoding in a single pass (64-bit unsigned interface).
    /// \param input Pointer to the encoded array.
    /// \param output Pointer to the decoded array (`uint64_t`).
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for reconstruction.
    /// \pre Each decoded mathematical delta must be representable in `int64_t` in intermediate steps.
    /// \note Implementation reuses signed-delta internals; representation is `uint64_t`,
    ///       while accumulation semantics are signed-delta based.
    /// \thread_safety Thread-safe (pure function over caller-provided buffers).

    inline void decode_delta_zig_zag_u64(
        const std::uint64_t* input,
        std::uint64_t* output,
        std::size_t size,
        std::uint64_t initial_value
    ) noexcept {
#       if defined(__AVX512F__)
        decode_delta_zig_zag_avx512_u64(input, output, size, static_cast<std::int64_t>(initial_value));
#       elif defined(__AVX2__)
        decode_delta_zig_zag_avx2_u64(input, output, size, static_cast<std::int64_t>(initial_value));
#       elif defined(__SSE2__)
        decode_delta_zig_zag_scalar_u64(input, output, size, static_cast<std::int64_t>(initial_value));
#       else
        decode_delta_zig_zag_scalar_u64(input, output, size, static_cast<std::int64_t>(initial_value));
#       endif
    }

    /// @}

};

#endif // _DFH_COMPRESSION_UTILS_ZIG_ZAG_DELTA_HPP_INCLUDED
