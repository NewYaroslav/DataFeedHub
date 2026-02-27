#pragma once
#ifndef _DFH_COMPRESSION_UTILS_ZIG_ZAG_DELTA_HPP_INCLUDED
#define _DFH_COMPRESSION_UTILS_ZIG_ZAG_DELTA_HPP_INCLUDED

/// \file zig_zag_delta.hpp
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
/// \subsection dfh_zigzag_delta_api_sorted Sorted delta codecs
/// - \c encode_delta_sorted(...)
/// - \c decode_delta_sorted(...)
///
/// \subsection dfh_zigzag_delta_api_time Time delta codecs
/// - \c encode_time_delta(...)
/// - \c decode_time_delta(...)
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
#include <type_traits>

namespace dfh::compression {

/// \brief Runtime dispatcher thresholds for hot SIMD paths.
/// \details
/// Centralized policy constants for easier calibration from benchmark data.
namespace dispatcher_policy {
    /// \brief Minimum element count to switch non-TradeTick u32 ID decode to AVX2.
    inline constexpr std::size_t ID_U32_AVX2_THRESHOLD = 256000;

    /// \brief Minimum element count to switch u64 price decode from SSE2 to AVX512.
    inline constexpr std::size_t PRICE_U64_DECODE_AVX512_THRESHOLD = 50000;
} // namespace dispatcher_policy


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

    #include "zig_zag_delta_id.hpp"
    #include "zig_zag_delta_price.hpp"
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


