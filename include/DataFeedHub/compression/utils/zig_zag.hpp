#pragma once
#ifndef _DFH_COMPRESSION_UTILS_ZIG_ZAG_HPP_INCLUDED
#define _DFH_COMPRESSION_UTILS_ZIG_ZAG_HPP_INCLUDED

/// \ingroup dfh_compression
/// \file zig_zag.hpp
/// \brief Provides Zig-Zag encoding/decoding utilities and optional SIMD-accelerated array codecs.
///
/// \warning Do NOT include this header directly.
/// Include only via \c include/DataFeedHub/compression/utils.hpp to guarantee that all required
/// dependencies, intrinsics headers, and configuration macros are included in the correct order.
///
/// \details
/// This header contains:
/// - Scalar ZigZag primitives for single values (32/64-bit),
/// - Optional SIMD helpers for ZigZag operations on vector registers,
/// - Scalar and AVX2 array encoders/decoders,
/// - Public dispatcher functions that select the best available implementation.
///
/// Some functionality is provided in multiple implementations (scalar vs SIMD) to make it possible
/// to run comparative benchmarks. In production code you should use the dispatcher functions
/// (encode_zig_zag_* / decode_zig_zag_*): they automatically choose the fastest variant available
/// on the current platform/compiler using feature macros (e.g., \c __AVX2__, \c __SSE2__).
///
/// \section dfh_zigzag_api API overview
/// \subsection dfh_zigzag_api_scalar_primitives Scalar primitives (single value)
/// - \c zigzag_encode_u32(std::int32_t)
/// - \c zigzag_encode_u64(std::int64_t)
/// - \c zigzag_decode_u32(std::uint32_t)
/// - \c zigzag_decode_u64(std::uint64_t)
///
/// \subsection dfh_zigzag_api_simd_helpers SIMD helpers (vector registers)
/// - SSE2:
///   - \c zigzag_encode_u64_sse2(__m128i)
///   - \c zigzag_decode_u32_sse2(__m128i)
///   - \c zigzag_decode_u64_sse2(__m128i)
/// - AVX2:
///   - \c zigzag_encode_u64_avx2(__m256i)
///   - \c zigzag_decode_u32_avx2(__m256i)
///   - \c zigzag_decode_u64_avx2(__m256i)
/// - AVX-512:
///   - \c zigzag_encode_u64_avx512(__m512i)
///   - \c zigzag_decode_u64_avx512(__m512i)
///
/// \subsection dfh_zigzag_api_array_scalar Scalar array codecs (baseline)
/// - \c encode_zig_zag_scalar_u32(const std::int32_t*, std::uint32_t*, std::size_t)
/// - \c decode_zig_zag_scalar_u32(const std::uint32_t*, std::int32_t*, std::size_t)
/// - \c encode_zig_zag_scalar_u64(const std::int64_t*, std::uint64_t*, std::size_t)
/// - \c decode_zig_zag_scalar_u64(const std::uint64_t*, std::int64_t*, std::size_t)
///
/// \subsection dfh_zigzag_api_array_avx2 AVX2 array codecs (benchmarks / optional direct use)
/// - \c encode_zig_zag_avx2_u32(const std::int32_t*, std::uint32_t*, std::size_t)
/// - \c decode_zig_zag_avx2_u32(const std::uint32_t*, std::int32_t*, std::size_t)
/// - \c encode_zig_zag_avx2_u64(const std::int64_t*, std::uint64_t*, std::size_t)
/// - \c decode_zig_zag_avx2_u64(const std::uint64_t*, std::int64_t*, std::size_t)
///
/// \subsection dfh_zigzag_api_dispatch Dispatchers (use these in production)
/// - \c encode_zig_zag_u32(const std::int32_t*, std::uint32_t*, std::size_t)
/// - \c decode_zig_zag_u32(const std::uint32_t*, std::int32_t*, std::size_t)
/// - \c encode_zig_zag_u64(const std::int64_t*, std::uint64_t*, std::size_t)
/// - \c decode_zig_zag_u64(const std::uint64_t*, std::int64_t*, std::size_t)


#include <cstddef>
#include <cstdint>

#if defined(__SSE2__)
#   include <emmintrin.h>
#endif

#if defined(__AVX2__)
#   include <immintrin.h>
#endif

namespace dfh::compression {
    
    /// \brief Encodes a 32-bit signed integer to ZigZag-encoded uint32.
    /// \details Inverse of zigzag_decode_u32().
    /// \param value Signed integer to encode.
    /// \return ZigZag-encoded unsigned integer.
    inline std::uint32_t zigzag_encode_u32(std::int32_t value) noexcept {
        const std::uint32_t u = static_cast<std::uint32_t>(value);
        return (u << 1) ^ static_cast<std::uint32_t>(-(u >> 31));
    }

    /// \brief Encodes a 64-bit signed integer to ZigZag-encoded uint64.
    /// \details Inverse of zigzag_decode_u64().
    /// \param value Signed integer to encode.
    /// \return ZigZag-encoded unsigned integer.
    inline std::uint64_t zigzag_encode_u64(std::int64_t value) noexcept {
        const std::uint64_t u = static_cast<std::uint64_t>(value);
        return (u << 1) ^ static_cast<std::uint64_t>(-(u >> 63));
    }

    /// \brief Decodes a ZigZag-encoded uint32 to a signed 32-bit integer.
    /// \details Inverse of zigzag_encode_u32().
    /// \param z ZigZag-encoded unsigned integer to decode.
    /// \return Decoded signed integer.
    inline std::int32_t zigzag_decode_u32(std::uint32_t z) noexcept {
        return static_cast<std::int32_t>((z >> 1) ^ (0u - (z & 1u)));
    }

    /// \brief Decodes a ZigZag-encoded uint64 to a signed 64-bit integer.
    /// \details Inverse of zigzag_encode_u64().
    /// \param z ZigZag-encoded unsigned integer to decode.
    /// \return Decoded signed integer.
    inline std::int64_t zigzag_decode_u64(std::uint64_t z) noexcept {
        return static_cast<std::int64_t>((z >> 1) ^ (0ull - (z & 1ull)));
    }

#   if defined(__SSE2__)
    /// \brief SSE2 ZigZag encoder for packed int64 deltas: int64 -> uint64 (per 64-bit lane).
    /// \details Uses (x << 1) ^ signmask, where signmask is 0 or -1 derived from the sign bit.
    [[nodiscard]] inline __m128i zigzag_encode_u64_sse2(__m128i d) noexcept {
        const __m128i shl1 = _mm_slli_epi64(d, 1);
        // signbit: 0 or 1 in each 64-bit lane
        const __m128i signbit = _mm_srli_epi64(d, 63);
        // signmask: 0 or 0xFFFFFFFFFFFFFFFF (i.e. 0 or -1) in each 64-bit lane
        const __m128i signmask = _mm_sub_epi64(_mm_setzero_si128(), signbit);
        return _mm_xor_si128(shl1, signmask);
    }

    [[nodiscard]] inline __m128i zigzag_decode_u32_sse2(__m128i z) noexcept {
        const __m128i shr1 = _mm_srli_epi32(z, 1);
        const __m128i lsb  = _mm_and_si128(z, _mm_set1_epi32(1));
        const __m128i mask = _mm_sub_epi32(_mm_setzero_si128(), lsb); // 0 - lsb => 0 or -1
        return _mm_xor_si128(shr1, mask);
    }
    
    [[nodiscard]] inline __m128i zigzag_decode_u64_sse2(__m128i z) noexcept {
        const __m128i one  = _mm_set1_epi64x(1);
        const __m128i sign = _mm_and_si128(z, one);                    // 0 or 1 in each lane
        const __m128i neg  = _mm_sub_epi64(_mm_setzero_si128(), sign); // 0 or -1
        const __m128i shr  = _mm_srli_epi64(z, 1);
        return _mm_xor_si128(shr, neg);
    }
#   endif

#   if defined(__AVX2__)
    /// \brief AVX2 ZigZag encoder for packed int64 deltas: int64 -> uint64 (per 64-bit lane).
    /// \details Uses (x << 1) ^ signmask, where signmask is 0 or -1 derived from the sign bit.
    [[nodiscard]] inline __m256i zigzag_encode_u64_avx2(__m256i d) noexcept {
        const __m256i shl1 = _mm256_slli_epi64(d, 1);
        // signbit: 0 or 1 in each 64-bit lane
        const __m256i signbit = _mm256_srli_epi64(d, 63);
        // signmask: 0 or 0xFFFFFFFFFFFFFFFF (i.e. 0 or -1) in each 64-bit lane
        const __m256i signmask = _mm256_sub_epi64(_mm256_setzero_si256(), signbit);
        return _mm256_xor_si256(shl1, signmask);
    }

    [[nodiscard]] inline __m256i zigzag_decode_u32_avx2(__m256i z) noexcept {
        const __m256i shr1 = _mm256_srli_epi32(z, 1);
        const __m256i lsb  = _mm256_and_si256(z, _mm256_set1_epi32(1));
        const __m256i mask = _mm256_sub_epi32(_mm256_setzero_si256(), lsb);
        return _mm256_xor_si256(shr1, mask);
    }
    
    [[nodiscard]] inline __m256i zigzag_decode_u64_avx2(__m256i z) noexcept {
        const __m256i one  = _mm256_set1_epi64x(1);
        const __m256i sign = _mm256_and_si256(z, one);
        const __m256i neg  = _mm256_sub_epi64(_mm256_setzero_si256(), sign);
        const __m256i shr  = _mm256_srli_epi64(z, 1);
        return _mm256_xor_si256(shr, neg);
    }
#   endif

#   if defined(__AVX512F__)
    [[nodiscard]] inline __m512i zigzag_encode_u64_avx512(__m512i d) noexcept {
        return _mm512_xor_si512(_mm512_slli_epi64(d, 1), _mm512_srai_epi64(d, 63));
    }
    
    [[nodiscard]] inline __m512i zigzag_decode_u64_avx512(__m512i z) noexcept {
        __m512i shr1 = _mm512_srli_epi64(z, 1);
        __m512i lsb  = _mm512_and_si512(z, _mm512_set1_epi64(1));
        __m512i mask = _mm512_sub_epi64(_mm512_setzero_si512(), lsb);
        return _mm512_xor_si512(shr1, mask);
    }
#   endif

// -----------------------------------------------------------------------------
// Scalar array implementations (baseline)
// -----------------------------------------------------------------------------

    /// \brief Scalar encoder: int32 -> uint32.
    inline void encode_zig_zag_scalar_u32(
            const std::int32_t* input,
            std::uint32_t* output,
            std::size_t size) noexcept {
        for (std::size_t i = 0; i < size; ++i) {
            output[i] = zigzag_encode_u32(input[i]);
        }
    }

    /// \brief Scalar decoder: uint32 -> int32.
    inline void decode_zig_zag_scalar_u32(
            const std::uint32_t* input,
            std::int32_t* output,
            std::size_t size) noexcept {
        for (std::size_t i = 0; i < size; ++i) {
            output[i] = zigzag_decode_u32(input[i]);
        }
    }

    /// \brief Scalar encoder: int64 -> uint64.
    inline void encode_zig_zag_scalar_u64(
            const std::int64_t* input,
            std::uint64_t* output,
            std::size_t size) noexcept {
        for (std::size_t i = 0; i < size; ++i) {
            output[i] = zigzag_encode_u64(input[i]);
        }
    }

    /// \brief Scalar decoder: uint64 -> int64.
    inline void decode_zig_zag_scalar_u64(
            const std::uint64_t* input,
            std::int64_t* output,
            std::size_t size) noexcept {
        for (std::size_t i = 0; i < size; ++i) {
            output[i] = zigzag_decode_u64(input[i]);
        }
    }
    
#   if defined(__AVX2__)
    // -------------------------------------------------------------------------
    // AVX2 array implementations
    // -------------------------------------------------------------------------

    /// \brief AVX2 encoder: int32 -> uint32, requires 32-byte alignment.
    /// \warning input/output must be 32-byte aligned.
    inline void encode_zig_zag_avx2_u32(
            const std::int32_t* input,
            std::uint32_t* output,
            std::size_t size) noexcept {
        constexpr std::size_t simd_width = 8; // 8 x u32 in 256-bit
        const std::size_t aligned_size = size & ~std::size_t(simd_width - 1);

        std::size_t i = 0;
        for (; i < aligned_size; i += simd_width) {
            const __m256i v = _mm256_load_si256(reinterpret_cast<const __m256i*>(input + i));
            const __m256i u_shl1 = _mm256_slli_epi32(v, 1);

            const __m256i signbit = _mm256_srli_epi32(v, 31);                 // 0/1
            const __m256i signmask = _mm256_sub_epi32(_mm256_setzero_si256(), signbit); // 0 / 0xFFFFFFFF

            const __m256i zz = _mm256_xor_si256(u_shl1, signmask);
            _mm256_store_si256(reinterpret_cast<__m256i*>(output + i), zz);
        }

        for (; i < size; ++i) {
            output[i] = zigzag_encode_u32(input[i]);
        }
    }

    /// \brief AVX2 decoder: uint32 -> int32, requires 32-byte alignment.
    /// \warning input/output must be 32-byte aligned.
    inline void decode_zig_zag_avx2_u32(
            const std::uint32_t* input,
            std::int32_t* output,
            std::size_t size) noexcept {
        constexpr std::size_t simd_width = 8;
        const std::size_t aligned_size = size & ~std::size_t(simd_width - 1);

        std::size_t i = 0;
        for (; i < aligned_size; i += simd_width) {
            const __m256i z = _mm256_load_si256(reinterpret_cast<const __m256i*>(input + i));
            const __m256i z_shr1 = _mm256_srli_epi32(z, 1);

            const __m256i lsb = _mm256_and_si256(z, _mm256_set1_epi32(1)); // 0/1
            const __m256i mask = _mm256_sub_epi32(_mm256_setzero_si256(), lsb); // 0 / 0xFFFFFFFF

            const __m256i v = _mm256_xor_si256(z_shr1, mask);
            _mm256_store_si256(reinterpret_cast<__m256i*>(output + i), v);
        }

        for (; i < size; ++i) {
            output[i] = zigzag_decode_u32(input[i]);
        }
    }

    /// \brief AVX2 encoder: int64 -> uint64, requires 32-byte alignment.
    /// \warning input/output must be 32-byte aligned.
    inline void encode_zig_zag_avx2_u64(
            const std::int64_t* input,
            std::uint64_t* output,
            std::size_t size) noexcept {
        constexpr std::size_t simd_width = 4; // 4 x u64 in 256-bit
        const std::size_t aligned_size = size & ~std::size_t(simd_width - 1);

        std::size_t i = 0;
        for (; i < aligned_size; i += simd_width) {
            const __m256i v = _mm256_load_si256(reinterpret_cast<const __m256i*>(input + i));
            const __m256i u_shl1 = _mm256_slli_epi64(v, 1);

            const __m256i signbit = _mm256_srli_epi64(v, 63); // 0/1
            const __m256i signmask = _mm256_sub_epi64(_mm256_setzero_si256(), signbit); // 0 / 0xFFFFFFFFFFFFFFFF

            const __m256i zz = _mm256_xor_si256(u_shl1, signmask);
            _mm256_store_si256(reinterpret_cast<__m256i*>(output + i), zz);
        }

        for (; i < size; ++i) {
            output[i] = zigzag_encode_u64(input[i]);
        }
    }

    /// \brief AVX2 decoder: uint64 -> int64, requires 32-byte alignment.
    /// \warning input/output must be 32-byte aligned.
    inline void decode_zig_zag_avx2_u64(
            const std::uint64_t* input,
            std::int64_t* output,
            std::size_t size) noexcept {
        constexpr std::size_t simd_width = 4;
        const std::size_t aligned_size = size & ~std::size_t(simd_width - 1);

        std::size_t i = 0;
        for (; i < aligned_size; i += simd_width) {
            const __m256i z = _mm256_load_si256(reinterpret_cast<const __m256i*>(input + i));
            const __m256i z_shr1 = _mm256_srli_epi64(z, 1);

            const __m256i lsb = _mm256_and_si256(z, _mm256_set1_epi64x(1)); // 0/1
            const __m256i mask = _mm256_sub_epi64(_mm256_setzero_si256(), lsb); // 0 / 0xFFFFFFFFFFFFFFFF

            const __m256i v = _mm256_xor_si256(z_shr1, mask);
            _mm256_store_si256(reinterpret_cast<__m256i*>(output + i), v);
        }

        for (; i < size; ++i) {
            output[i] = zigzag_decode_u64(input[i]);
        }
    }
#   endif // __AVX2__


    // -------------------------------------------------------------------------
    // Public API: dispatch by macros
    // -------------------------------------------------------------------------

    /// \brief Encodes int32 array to ZigZag uint32 array.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    /// \note If AVX2 is enabled, input/output must be 32-byte aligned.
    inline void encode_zig_zag_u32(
            const std::int32_t* input,
            std::uint32_t* output,
            std::size_t size) noexcept {
#       if defined(__AVX2__)
        encode_zig_zag_avx2_u32(input, output, size);
#       else
        encode_zig_zag_scalar_u32(input, output, size);
#       endif
    }

    /// \brief Decodes ZigZag uint32 array to int32 array.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    /// \note If AVX2 is enabled, input/output must be 32-byte aligned.
    inline void decode_zig_zag_u32(
            const std::uint32_t* input,
            std::int32_t* output,
            std::size_t size) noexcept {
#       if defined(__AVX2__)
        decode_zig_zag_avx2_u32(input, output, size);
#       else
        decode_zig_zag_scalar_u32(input, output, size);
#       endif
    }

    /// \brief Encodes int64 array to ZigZag uint64 array.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    /// \note If AVX2 is enabled, input/output must be 32-byte aligned.
    inline void encode_zig_zag_u64(
            const std::int64_t* input,
            std::uint64_t* output,
            std::size_t size) noexcept {
#       if defined(__AVX2__)
        encode_zig_zag_avx2_u64(input, output, size);
#       else
        encode_zig_zag_scalar_u64(input, output, size);
#       endif
    }

    /// \brief Decodes ZigZag uint64 array to int64 array.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    /// \note If AVX2 is enabled, input/output must be 32-byte aligned.
    inline void decode_zig_zag_u64(
            const std::uint64_t* input,
            std::int64_t* output,
            std::size_t size) noexcept {
#       if defined(__AVX2__)
        decode_zig_zag_avx2_u64(input, output, size);
#       else
        decode_zig_zag_scalar_u64(input, output, size);
#       endif
    }

} // namespace dfh::compression

#endif // _DFH_COMPRESSION_UTILS_ZIG_ZAG_HPP_INCLUDED
