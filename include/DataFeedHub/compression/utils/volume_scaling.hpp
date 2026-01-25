#pragma once
#ifndef _DFH_COMPRESSION_UTILS_VOLUME_SCALING_HPP_INCLUDED
#define _DFH_COMPRESSION_UTILS_VOLUME_SCALING_HPP_INCLUDED

/// \file volume_scaling.hpp
/// \brief Volume scaling utilities for compressing and decompressing tick volume data.

namespace dfh::compression {

    /// \brief Tries to scale tick `volume` to `uint32_t` using the provided scale factor.
    /// \details Returns `false` if any scaled value exceeds `int32_t` max. This is intended for a
    ///          "try u32, fallback to u64" workflow.
    /// \tparam TickType Type of tick structure (must have `volume` convertible to double).
    /// \param ticks Pointer to input ticks.
    /// \param output Output array for scaled volume values.
    /// \param size Number of elements to process.
    /// \param scale Multiplicative scale factor.
    /// \return `true` if all values fit into `int32_t` max; otherwise `false`.
    /// \throw std::invalid_argument If a scaled volume becomes negative (invalid input data).
    template<class TickType>
    bool scale_volume_u32(
            const TickType* ticks,
            std::uint32_t* output,
            std::size_t size,
            double scale
        ) {
        constexpr std::int64_t max_i32 = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max());
        for (std::size_t i = 0; i < size; ++i) {
            const std::int64_t v = std::llround(ticks[i].volume * scale);
            // Negative volume is considered invalid input data.
            if (v < 0) throw std::invalid_argument("scale_volume_u32: negative volume after scaling");
            // Doesn't fit into int32_t -> caller should fallback to uint64_t path.
            if (v > max_i32) return false;
            output[i] = static_cast<std::uint32_t>(v);
        }
        return true;
    }

    /// \brief Scales the `volume` field of tick data to `uint64_t` using the provided scale factor.
    /// \tparam TickType Type of tick structure.
    /// \param ticks Pointer to input ticks.
    /// \param output Output array for scaled volume values.
    /// \param size Number of elements to process.
    /// \param scale Multiplicative scale factor.
    template<class TickType>
    void scale_volume_u64(
            const TickType* ticks,
            std::uint64_t* output,
            std::size_t size,
            double scale
        ) {
        for (std::size_t i = 0; i < size; ++i) {
            output[i] = static_cast<std::uint64_t>(std::llround(ticks[i].volume * scale));
        }
    }

//------------------------------------------------------------------------------

    /// \brief Scalar implementation of volume unscaling (InputType -> TickType::volume as double).
    /// \tparam InputType Type of scaled input values (e.g., uint32_t, uint64_t).
    /// \tparam TickType Type of tick structure (must have `double volume` field).
    /// \param input Pointer to scaled volume values.
    /// \param ticks Output tick array.
    /// \param size Number of elements to process.
    /// \param scale Original scale factor used in compression.
    template<class InputType, class TickType>
    inline void unscale_volume_scalar(
            const InputType* input,
            TickType* ticks,
            std::size_t size,
            double scale
        ) {
        if (scale == 0.0) {
            throw std::invalid_argument("unscale_volume_scalar: scale must be non-zero");
        }

        const double inv_scale = 1.0 / scale;
        for (std::size_t i = 0; i < size; ++i) {
            ticks[i].volume = static_cast<double>(input[i]) * inv_scale;
        }
    }

#if defined(__AVX2__)
    /// \brief AVX2 implementation of unscaling for uint32_t input (SIMD convert/mul, scalar stores into AoS).
    /// \tparam TickType Type of tick structure (must have `double volume` field).
    /// \param input Pointer to scaled volume values (uint32_t).
    /// \param ticks Output tick array.
    /// \param size Number of elements to process.
    /// \param scale Original scale factor used in compression.
    /// \note Stores into `ticks[i].volume` are scalar because AoS layout requires scatter, which AVX2 lacks.
    template<class TickType>
    inline void unscale_volume_avx2_u32(
            const std::uint32_t* input,
            TickType* ticks,
            std::size_t size,
            double scale
        ) {
        static_assert(std::is_standard_layout_v<TickType>, "TickType must be standard-layout.");
        static_assert(std::is_same_v<decltype(TickType{}.volume), double>, "TickType::volume must be double.");

        if (scale == 0.0) {
            throw std::invalid_argument("unscale_volume_avx2_u32: scale must be non-zero");
        }

        const double inv_scale = 1.0 / scale;
        const __m256d inv = _mm256_set1_pd(inv_scale);

        alignas(32) double tmp[4];

        std::size_t i = 0;
        for (; i + 4 <= size; i += 4) {
            // NOTE: treated as int32 for conversion. If values may exceed INT32_MAX, use uint64_t path instead.
            const __m128i v32 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(input + i));
            __m256d vd = _mm256_cvtepi32_pd(v32);
            vd = _mm256_mul_pd(vd, inv);

            _mm256_store_pd(tmp, vd);

            ticks[i + 0].volume = tmp[0];
            ticks[i + 1].volume = tmp[1];
            ticks[i + 2].volume = tmp[2];
            ticks[i + 3].volume = tmp[3];
        }

        for (; i < size; ++i) {
            ticks[i].volume = static_cast<double>(input[i]) * inv_scale;
        }
    }
#endif // __AVX2__

#if defined(__AVX512F__)
    /// \brief AVX-512F implementation of unscaling for uint32_t input (SIMD convert/mul, scalar stores into AoS).
    /// \tparam TickType Type of tick structure (must have `double volume` field).
    /// \param input Pointer to scaled volume values (uint32_t).
    /// \param ticks Output tick array.
    /// \param size Number of elements to process.
    /// \param scale Original scale factor used in compression.
    /// \note Stores into `ticks[i].volume` are scalar because AoS layout still requires scatter for full SIMD stores.
    template<class TickType>
    inline void unscale_volume_avx512_u32(
            const std::uint32_t* input,
            TickType* ticks,
            std::size_t size,
            double scale
        ) {
        static_assert(std::is_standard_layout_v<TickType>, "TickType must be standard-layout.");
        static_assert(std::is_same_v<decltype(TickType{}.volume), double>, "TickType::volume must be double.");

        if (scale == 0.0) {
            throw std::invalid_argument("unscale_volume_avx512_u32: scale must be non-zero");
        }

        const double inv_scale = 1.0 / scale;
        const __m512d inv = _mm512_set1_pd(inv_scale);

        alignas(64) double tmp[8];

        std::size_t i = 0;
        for (; i + 8 <= size; i += 8) {
            const __m256i v32 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(input + i));
            __m512d vd = _mm512_cvtepu32_pd(v32);
            vd = _mm512_mul_pd(vd, inv);

            _mm512_store_pd(tmp, vd);

            ticks[i + 0].volume = tmp[0];
            ticks[i + 1].volume = tmp[1];
            ticks[i + 2].volume = tmp[2];
            ticks[i + 3].volume = tmp[3];
            ticks[i + 4].volume = tmp[4];
            ticks[i + 5].volume = tmp[5];
            ticks[i + 6].volume = tmp[6];
            ticks[i + 7].volume = tmp[7];
        }

        for (; i < size; ++i) {
            ticks[i].volume = static_cast<double>(input[i]) * inv_scale;
        }
    }

    /// \brief AVX-512DQ implementation of unscaling for uint64_t input (SIMD convert/mul, scalar stores into AoS).
    /// \tparam TickType Type of tick structure (must have `double volume` field).
    /// \param input Pointer to scaled volume values (uint64_t).
    /// \param ticks Output tick array.
    /// \param size Number of elements to process.
    /// \param scale Original scale factor used in compression.
    /// \note Requires AVX-512DQ for uint64->double conversion.
#   if defined(__AVX512DQ__)
    template<class TickType>
    inline void unscale_volume_avx512_u64(
            const std::uint64_t* input,
            TickType* ticks,
            std::size_t size,
            double scale
        ) {
        static_assert(std::is_standard_layout_v<TickType>, "TickType must be standard-layout.");
        static_assert(std::is_same_v<decltype(TickType{}.volume), double>, "TickType::volume must be double.");

        if (scale == 0.0) {
            throw std::invalid_argument("unscale_volume_avx512_u64: scale must be non-zero");
        }

        const double inv_scale = 1.0 / scale;
        const __m512d inv = _mm512_set1_pd(inv_scale);

        alignas(64) double tmp[8];

        std::size_t i = 0;
        for (; i + 8 <= size; i += 8) {
            const __m512i v64 = _mm512_loadu_si512(reinterpret_cast<const void*>(input + i));
            __m512d vd = _mm512_cvtepu64_pd(v64);
            vd = _mm512_mul_pd(vd, inv);

            _mm512_store_pd(tmp, vd);

            ticks[i + 0].volume = tmp[0];
            ticks[i + 1].volume = tmp[1];
            ticks[i + 2].volume = tmp[2];
            ticks[i + 3].volume = tmp[3];
            ticks[i + 4].volume = tmp[4];
            ticks[i + 5].volume = tmp[5];
            ticks[i + 6].volume = tmp[6];
            ticks[i + 7].volume = tmp[7];
        }

        for (; i < size; ++i) {
            ticks[i].volume = static_cast<double>(input[i]) * inv_scale;
        }
    }
#   endif // __AVX512DQ__
#endif // __AVX512F__

//------------------------------------------------------------------------------

    /// \brief Restores scaled `volume` values (`uint32_t`) back into tick structures.
    /// \tparam TickType Type of tick structure.
    /// \param input Pointer to scaled volume values.
    /// \param ticks Output tick array.
    /// \param size Number of elements to process.
    /// \param scale Original scale factor used in compression.
    template<class TickType>
    void unscale_volume(
            const std::uint32_t* input,
            TickType* ticks,
            std::size_t size,
            double scale
        ) {
#if defined(__AVX512F__)
        unscale_volume_avx512_u32<TickType>(input, ticks, size, scale);
#elif defined(__AVX2__)
        unscale_volume_avx2_u32<TickType>(input, ticks, size, scale);
#else
        unscale_volume_scalar<std::uint32_t, TickType>(input, ticks, size, scale);
#endif
    }

    /// \brief Restores scaled `volume` values (`uint64_t`) back into tick structures.
    /// \tparam TickType Type of tick structure.
    /// \param input Pointer to scaled volume values.
    /// \param ticks Output tick array.
    /// \param size Number of elements to process.
    /// \param scale Original scale factor used in compression.
    template<class TickType>
    void unscale_volume(
            const std::uint64_t* input,
            TickType* ticks,
            std::size_t size,
            double scale
        ) {
#if defined(__AVX512DQ__)
        unscale_volume_avx512_u64<TickType>(input, ticks, size, scale);
#else
        unscale_volume_scalar<std::uint64_t, TickType>(input, ticks, size, scale);
#endif
    }

}; // namespace dfh::compression

#endif // _DFH_COMPRESSION_UTILS_VOLUME_SCALING_HPP_INCLUDED
