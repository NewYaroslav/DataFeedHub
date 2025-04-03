#pragma once
#ifndef _DFH_COMPRESSION_UTILS_ZIG_ZAG_DELTA_HPP_INCLUDED
#define _DFH_COMPRESSION_UTILS_ZIG_ZAG_DELTA_HPP_INCLUDED

/// \file zig_zag_delta.hpp
/// \brief

namespace dfh::compression {

    /// \brief Checks if a delta array can fit into int32_t without overflow.
    /// \param input Pointer to the input array.
    /// \param size Number of elements in the array.
    /// \return True if all deltas can fit into int32_t, false otherwise.
    bool check_deltas_fit_int32(const int32_t* input, size_t size) {
        for (size_t i = 1; i < size; ++i) {
            if ((static_cast<int64_t>(input[i]) - static_cast<int64_t>(input[i - 1])) > INT32_MAX ||
                (static_cast<int64_t>(input[i]) - static_cast<int64_t>(input[i - 1])) < INT32_MIN) {
                return false;
            }
        }
        return true;
    }

    bool check_deltas_fit_int32(const int32_t* input, size_t size, int32_t initial_value) {
        if ((static_cast<int64_t>(input[0]) - static_cast<int64_t>(initial_value)) > INT32_MAX ||
            (static_cast<int64_t>(input[0]) - static_cast<int64_t>(initial_value)) < INT32_MIN) {
            return false;
        }
        for (size_t i = 1; i < size; ++i) {
            if ((static_cast<int64_t>(input[i]) - static_cast<int64_t>(input[i - 1])) > INT32_MAX ||
                (static_cast<int64_t>(input[i]) - static_cast<int64_t>(input[i - 1])) < INT32_MIN) {
                return false;
            }
        }
        return true;
    }

//------------------------------------------------------------------------------

    template<class InputType = int32_t, class OutputType = int32_t>
    void encode_delta(const InputType* input, OutputType* output, size_t size, InputType initial_value) {
        if (size == 0) return;

        output[0] = input[0] - initial_value;
        for (size_t i = 1; i < size; ++i) {
            output[i] = input[i] - input[i-1];
        }
    }

    template<class InputType = int32_t, class OutputType = int32_t>
    void decode_delta(const InputType* input, OutputType* output, size_t size, OutputType initial_value) {
        if (size == 0) return;

        output[0] = initial_value + input[0];
        for (size_t i = 1; i < size; ++i) {
            output[i] = input[i] + output[i - 1];
        }
    }

//------------------------------------------------------------------------------

    template<class InputType = uint32_t, class OutputType = uint32_t>
    void encode_delta_sorted(const InputType* input, OutputType* output, size_t size, InputType initial_value) {
        if (size == 0) return;

        InputType delta;
        for (size_t i = 0; i < size; ++i) {
            delta = input[i] - initial_value;
            initial_value = input[i];
            output[i] = delta;
        }
    }

    template<class InputType = uint32_t, class OutputType = uint32_t>
    void decode_delta_sorted(const InputType* input, OutputType* output, size_t size, OutputType initial_value) {
        if (size == 0) return;

        output[0] = initial_value + input[0];
        for (size_t i = 1; i < size; ++i) {
            output[i] = input[i] + output[i - 1];
        }
    }

//------------------------------------------------------------------------------

    void encode_delta_int32(const int32_t* input, uint32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;

        constexpr int64_t min_val = static_cast<int64_t>(std::numeric_limits<int32_t>::min());
        constexpr int64_t max_val = static_cast<int64_t>(std::numeric_limits<int32_t>::max());
        int64_t raw_delta;
        int32_t delta;
        for (size_t i = 0; i < size; ++i) {
            raw_delta = static_cast<int64_t>(input[i]) - static_cast<int64_t>(initial_value);
            if (raw_delta < min_val || raw_delta > max_val) throw std::overflow_error("Delta overflow: input[i] - initial_value > int32 range");
            initial_value = input[i];
            delta = static_cast<int32_t>(raw_delta);
            output[i] = (delta << 1) ^ (delta >> 31);
        }


        output[0] = input[0] - initial_value;
        for (size_t i = 1; i < size; ++i) {
            output[i] = input[i] - input[i-1];
        }
    }

    void decode_delta_int32(const uint32_t* input, int32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;

        output[0] = initial_value + input[0];
        for (size_t i = 1; i < size; ++i) {
            output[i] = input[i] + output[i - 1];
        }
    }

    void encode_delta_int64(const int64_t* input, uint64_t* output, size_t size, int64_t initial_value) {
        if (size == 0) return;

        output[0] = input[0] - initial_value;
        for (size_t i = 1; i < size; ++i) {
            output[i] = input[i] - input[i-1];
        }
    }

    void decode_delta_int64(const uint64_t* input, int64_t* output, size_t size, int64_t initial_value) {
        if (size == 0) return;

        output[0] = initial_value + input[0];
        for (size_t i = 1; i < size; ++i) {
            output[i] = input[i] + output[i - 1];
        }
    }

//------------------------------------------------------------------------------

    template<class TickType>
    void encode_time_delta(
            const TickType* ticks,
            uint32_t* output,
            size_t size,
            int64_t initial_time) {
        if (size == 0) return;

        if (ticks[0].time_ms < static_cast<uint64_t>(initial_time)) {
            throw std::overflow_error(
                "encode_time_delta: The first tick's timestamp ("
                + std::to_string(ticks[0].time_ms)
                + ") is less than the initial_time ("
                + std::to_string(initial_time) + ")."
            );
        }

        output[0] = ticks[0].time_ms - initial_time;
        for (size_t i = 1; i < size; ++i) {
            if (ticks[i].time_ms < ticks[i - 1].time_ms) {
                throw std::overflow_error(
                    "encode_time_delta: The timestamp of tick at index "
                    + std::to_string(i) + " (" + std::to_string(ticks[i].time_ms)
                    + ") is less than the timestamp of the previous tick ("
                    + std::to_string(ticks[i - 1].time_ms) + ")."
                );
            }
            output[i] = ticks[i].time_ms - ticks[i - 1].time_ms;
        }
    }

    template<class TickType>
    void decode_time_delta(
            const uint32_t* deltas,
            TickType* ticks,
            size_t size,
            int64_t initial_time) {
        if (size == 0) return;

        ticks[0].time_ms = deltas[0] + initial_time;
        for (size_t i = 1; i < size; ++i) {
            ticks[i].time_ms = ticks[i - 1].time_ms + deltas[i];
        }
    }

//------------------------------------------------------------------------------

    template<class TickType>
    void encode_last_delta_zig_zag_int32(
            const TickType* ticks,
            uint32_t* output,
            size_t size,
            double price_scale,
            int64_t initial_price) {
        if (size == 0) return;

        constexpr int64_t min_val = static_cast<int64_t>(std::numeric_limits<int32_t>::min());
        constexpr int64_t max_val = static_cast<int64_t>(std::numeric_limits<int32_t>::max());
        int64_t raw_delta, scaled_price;
        int32_t delta;
        for (size_t i = 0; i < size; ++i) {
            scaled_price = std::llround(ticks[i].last * price_scale);
            raw_delta = scaled_price - initial_price;
            if (raw_delta < min_val || raw_delta > max_val) throw std::overflow_error("Delta overflow: scaled_price - initial_price > int32 range");
            delta = static_cast<int32_t>(raw_delta);
            output[i] = static_cast<uint32_t>((delta << 1) ^ (delta >> 31));
            initial_price = scaled_price;
        }
    }

    template<class TickType>
    void decode_last_delta_zig_zag_int32(
            const uint32_t* deltas,
            TickType* ticks,
            size_t size,
            double price_scale,
            int64_t initial_price) {
        int64_t scaled_price;
        double inv_initial_price = 1.0 / price_scale;
        int32_t delta;
        for (size_t i = 0; i < size; ++i) {
            delta = (deltas[i] >> 1) ^ -(deltas[i] & 1);
            scaled_price = initial_price + delta;
            ticks[i].last = static_cast<double>(scaled_price) * inv_initial_price;
            initial_price = scaled_price;
        }
    }

    template<class TickType>
    void encode_last_delta_zig_zag_int64(
            const TickType* ticks,
            uint64_t* output,
            size_t size,
            double price_scale,
            int64_t initial_price) {
        if (size == 0) return;
        int64_t scaled_price, delta;
        for (size_t i = 0; i < size; ++i) {
            scaled_price = std::llround(ticks[i].last * price_scale);
            delta = scaled_price - initial_price;
            output[i] = (delta << 1) ^ (delta >> 63);
            initial_price = scaled_price;
        }
    }

    template<class TickType>
    void decode_last_delta_zig_zag_int64(
            const uint64_t* deltas,
            TickType* ticks,
            size_t size,
            double price_scale,
            int64_t initial_price) {
        int64_t scaled_price, delta;
        double inv_initial_price = 1.0 / price_scale;
        for (size_t i = 0; i < size; ++i) {
            delta = (deltas[i] >> 1) ^ -(deltas[i] & 1);
            scaled_price = initial_price + delta;
            ticks[i].last = static_cast<double>(scaled_price) * inv_initial_price;
            initial_price = scaled_price;
        }
    }

//------------------------------------------------------------------------------

    /// \brief Performs delta and Zig-Zag encoding in a single pass.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    /// \param initial_value
    void encode_delta_zig_zag_int32(
            const int32_t* input,
            uint32_t* output,
            size_t size,
            int32_t initial_value) {
        if (size == 0) return;
        int32_t prev = initial_value, delta_value;
#       if defined(__SSE2__)
        constexpr size_t simd_width = 4;
        const size_t aligned_size = size - ((size - 1) % simd_width);
        __m128i delta;

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            if (aligned_size > 1) {
                for (size_t i = 0; i < simd_width; ++i) {
                    delta_value = input[i] - prev;
                    output[i] = (delta_value << 1) ^ (delta_value >> 31);
                    prev = input[i];
                }
            }
            for (size_t i = simd_width; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i - 1])));

                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1),
                        _mm_srai_epi32(delta, 31)));
            }
        } else {
            for (size_t i = 1; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i - 1])));

                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1),
                        _mm_srai_epi32(delta, 31)));
            }
        }

        delta_value = input[0] - initial_value;
        output[0] = (delta_value << 1) ^ (delta_value >> 31);
        prev = aligned_size > 0 ? input[aligned_size - 1] : initial_value;
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - prev;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
            prev = input[i];
        }
#       else
        for (size_t i = 0; i < size; ++i) {
            delta_value = input[i] - prev;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
            prev = input[i];
        }
#       endif
    }

    /// \brief Performs delta and Zig-Zag decoding in a single pass.
    /// \param input Pointer to the encoded array.
    /// \param output Pointer to the decoded array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for reconstruction.
    void decode_delta_zig_zag_int32(
            const uint32_t* input,
            int32_t* output,
            size_t size,
            int32_t initial_value) {
        if (size == 0) return;
        int32_t zigzag = (input[0] >> 1) ^ -(input[0] & 1);
        output[0] = initial_value + zigzag;
        for (size_t i = 1; i < size; ++i) {
            zigzag = (input[i] >> 1) ^ -(input[i] & 1);
            output[i] = output[i - 1] + zigzag;
        }
    }

    void encode_delta_zig_zag_int32(
            const uint32_t* input,
            uint32_t* output,
            size_t size,
            uint32_t initial_value) {
        constexpr int64_t min_val = static_cast<int64_t>(std::numeric_limits<int32_t>::min());
        constexpr int64_t max_val = static_cast<int64_t>(std::numeric_limits<int32_t>::max());
        int64_t raw_delta;
        int32_t delta;
        for (size_t i = 0; i < size; ++i) {
            raw_delta = static_cast<int64_t>(input[i]) - static_cast<int64_t>(initial_value);
            if (raw_delta < min_val || raw_delta > max_val) throw std::overflow_error("Delta overflow: input[i] - initial_value > int32 range");
            initial_value = input[i];
            delta = static_cast<int32_t>(raw_delta);
            output[i] = (delta << 1) ^ (delta >> 31);
        }
    }

    void decode_delta_zig_zag_int32(
            const uint32_t* input,
            uint32_t* output,
            size_t size,
            uint32_t initial_value) {
        if (size == 0) return;
        int64_t zigzag = (input[0] >> 1) ^ -(input[0] & 1);
        output[0] = static_cast<uint32_t>(static_cast<int64_t>(initial_value) + zigzag);
        for (size_t i = 1; i < size; ++i) {
            zigzag = (input[i] >> 1) ^ -(input[i] & 1);
            output[i] = static_cast<uint32_t>(static_cast<int64_t>(output[i - 1]) + zigzag);
        }
    }

//------------------------------------------------------------------------------

    /// \brief Performs delta and Zig-Zag encoding in a single pass (64-bit).
    /// \param input Pointer to the input array (int64_t).
    /// \param output Pointer to the output array (uint64_t).
    /// \param size Number of elements in the array.
    /// \param initial_value The reference value for delta computation (64-bit).
    void encode_delta_zig_zag_int64(
            const int64_t* input,
            uint64_t* output,
            size_t size,
            int64_t initial_value) {
        if (size == 0) return;
        int64_t prev = initial_value, delta_value;

#       if defined(__SSE2__)
        constexpr size_t simd_width = 2; // SSE2 обрабатывает 2 int64 за раз
        const size_t aligned_size = size - ((size - 1) % simd_width);
        __m128i delta;

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            if (aligned_size > 1) {
                for (size_t i = 0; i < simd_width; ++i) {
                    delta_value = input[i] - prev;
                    output[i] = (delta_value << 1) ^ (delta_value >> 63);
                    prev = input[i];
                }
            }
            for (size_t i = simd_width; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi64(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i - 1])));
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))
                    ));
            }
        } else {
            for (size_t i = 1; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi64(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i - 1])));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))
                    ));
            }
        }

        delta_value = input[0] - initial_value;
        output[0] = (delta_value << 1) ^ (delta_value >> 63);
        prev = aligned_size > 0 ? input[aligned_size - 1] : initial_value;
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - prev;
            output[i] = (delta_value << 1) ^ (delta_value >> 63);
            prev = input[i];
        }
#       else
        for (size_t i = 0; i < size; ++i) {
            delta_value = input[i] - prev;
            output[i] = (delta_value << 1) ^ (delta_value >> 63);
            prev = input[i];
        }
#       endif
    }

    /// \brief Performs delta and Zig-Zag decoding in a single pass for 64-bit integers.
    /// \param input Pointer to the encoded array.
    /// \param output Pointer to the decoded array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for reconstruction.
    void decode_delta_zig_zag_int64(
            const uint64_t* input,
            int64_t* output,
            size_t size,
            int64_t initial_value) {
        if (size == 0) return;
        int64_t zigzag = (input[0] >> 1) ^ -(input[0] & 1);
        output[0] = initial_value + zigzag;
        for (size_t i = 1; i < size; ++i) {
            zigzag = (input[i] >> 1) ^ -(input[i] & 1);
            output[i] = output[i - 1] + zigzag;
        }
    }

    void encode_delta_zig_zag_int64(
            const uint64_t* input,
            uint64_t* output,
            size_t size,
            uint64_t initial_value) {
        int64_t delta;
        for (size_t i = 0; i < size; ++i) {
            delta = static_cast<int64_t>(input[i]) - static_cast<int64_t>(initial_value);
            initial_value = input[i];
            output[i] = (delta << 1) ^ (delta >> 63);
        }
    }

    void decode_delta_zig_zag_int64(
            const uint64_t* input,
            uint64_t* output,
            size_t size,
            uint64_t initial_value) {
        if (size == 0) return;
        int64_t zigzag = (input[0] >> 1) ^ -(input[0] & 1);
        output[0] = static_cast<uint64_t>(static_cast<int64_t>(initial_value) + zigzag);
        for (size_t i = 1; i < size; ++i) {
            zigzag = (input[i] >> 1) ^ -(input[i] & 1);
            output[i] = static_cast<uint64_t>(static_cast<int64_t>(output[i - 1]) + zigzag);
        }
    }

//------------------------------------------------------------------------------

    /// \brief Performs delta and Zig-Zag encoding in a single pass.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for delta computation.
    void encode_delta_zig_zag_chunked8_int32(const int32_t* input, uint32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 8;
        const size_t aligned_size = size - (size % simd_width);

#       if defined(__SSE2__)
        constexpr size_t sse_simd_width = 4;

        __m128i base = _mm_set1_epi32(initial_value);
        __m128i delta;

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                delta = _mm_sub_epi32(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width])),
                    base);
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                base = _mm_set1_epi32(input[i + simd_width - 1]);
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                delta = _mm_sub_epi32(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width])),
                    base);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                base = _mm_set1_epi32(input[i + simd_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = input[aligned_size - 1];
        }

        int32_t delta_value;
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
        }
#       else
        int32_t delta_value;
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                delta_value = input[j] - initial_value;
                output[j] = (delta_value << 1) ^ (delta_value >> 31);
            }
            initial_value = input[j_max - 1];
        }
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
        }
#       endif
    }

    /// \brief Performs delta and Zig-Zag decoding in a single pass.
    /// \param input Pointer to the encoded array.
    /// \param output Pointer to the decoded array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for reconstruction.
    void decode_delta_zig_zag_chunked8_int32(const uint32_t* input, int32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 8;
        const size_t aligned_size = size - (size % simd_width);

#       if defined(__SSE2__)
        constexpr size_t sse_simd_width = 4;

        __m128i base = _mm_set1_epi32(initial_value);
        __m128i delta;

        const __m128i one = _mm_set1_epi32(1);
        const __m128i zero = _mm_setzero_si128();

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                delta = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width]));
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi32(output[i + simd_width - 1]);
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                delta = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width]));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi32(output[i + simd_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = output[aligned_size - 1];
        }

        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
        }
#       else
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                output[j] = initial_value + ((input[j] >> 1) ^ -(input[j] & 1));
                initial_value = output[j];
            }
        }
        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
            initial_value = output[i];
        }
#       endif
    }

//------------------------------------------------------------------------------

    /// \brief Performs delta and Zig-Zag encoding in a single pass.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for delta computation.
    void encode_delta_zig_zag_chunked4_int32(const int32_t* input, uint32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 4;
        const size_t aligned_size = size - (size % simd_width);

#       if defined(__SSE2__)
        __m128i base = _mm_set1_epi32(initial_value);
        __m128i delta;

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                base = _mm_set1_epi32(input[i + simd_width - 1]);
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                base = _mm_set1_epi32(input[i + simd_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = input[aligned_size - 1];
        }

        int32_t delta_value;
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
        }
#       else
        int32_t delta_value;
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                delta_value = input[j] - initial_value;
                output[j] = (delta_value << 1) ^ (delta_value >> 31);
            }
            initial_value = input[j_max - 1];
        }
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
        }
#       endif
    }

    /// \brief Performs delta and Zig-Zag decoding in a single pass.
    /// \param input Pointer to the encoded array.
    /// \param output Pointer to the decoded array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for reconstruction.
    void decode_delta_zig_zag_chunked4_int32(const uint32_t* input, int32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 4;
        const size_t aligned_size = size - (size % simd_width);

#       if defined(__SSE2__)
        __m128i base = _mm_set1_epi32(initial_value);
        __m128i delta;

        const __m128i one = _mm_set1_epi32(1);
        const __m128i zero = _mm_setzero_si128();

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi32(output[i + simd_width - 1]);
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi32(output[i + simd_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = output[aligned_size - 1];
        }

        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
        }
#       else
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                output[j] = initial_value + ((input[j] >> 1) ^ -(input[j] & 1));
                initial_value = output[j];
            }
        }
        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
            initial_value = output[i];
        }
#       endif
    }

//------------------------------------------------------------------------------

    /// \brief Performs delta and Zig-Zag encoding in a single pass for 64-bit integers.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for delta computation.
    void encode_delta_zig_zag_chunked4_int64(const int64_t* input, uint64_t* output, size_t size, int64_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 4;
        const size_t aligned_size = size - (size % simd_width);

#       if defined(__SSE2__)
        constexpr size_t sse_simd_width = 2;

        __m128i base = _mm_set1_epi64x(initial_value);
        __m128i delta;

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi64(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))));


                delta = _mm_sub_epi64(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width])),
                    base);
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))));

                base = _mm_set1_epi64x(input[i + simd_width - 1]);
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi64(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))));


                delta = _mm_sub_epi64(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width])),
                    base);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))));

                base = _mm_set1_epi64x(input[i + simd_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = input[aligned_size - 1];
        }

        int64_t delta_value;
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 63);
        }
#       else
        int64_t delta_value;
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                delta_value = input[j] - initial_value;
                output[j] = (delta_value << 1) ^ (delta_value >> 63);
            }
            initial_value = input[j_max - 1];
        }
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 63);
        }
#       endif
    }

    /// \brief Performs delta and Zig-Zag decoding in a single pass for 64-bit integers.
    /// \param input Pointer to the encoded array.
    /// \param output Pointer to the decoded array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for reconstruction.
    void decode_delta_zig_zag_chunked4_int64(const uint64_t* input, int64_t* output, size_t size, int64_t initial_value) {
        if (size == 0) return;

        constexpr size_t chunk_width = 4;
        const size_t aligned_size = size - (size % chunk_width);

#       if defined(__SSE2__)
        constexpr size_t sse_simd_width = 2;
        __m128i base = _mm_set1_epi64x(initial_value);
        __m128i delta;

        const __m128i one = _mm_set1_epi64x(1);
        const __m128i zero = _mm_setzero_si128();

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += chunk_width) {
                delta = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i]));

                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi64(_mm_xor_si128(
                        _mm_srli_epi64(delta, 1),
                        _mm_sub_epi64(zero, _mm_and_si128(delta, one))), base));

                delta = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width]));

                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_add_epi64(_mm_xor_si128(
                        _mm_srli_epi64(delta, 1),
                        _mm_sub_epi64(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi64x(output[i + chunk_width - 1]);
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += chunk_width) {
                delta = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i]));

                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi64(_mm_xor_si128(
                        _mm_srli_epi64(delta, 1),
                        _mm_sub_epi64(zero, _mm_and_si128(delta, one))), base));

                delta = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width]));

                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_add_epi64(_mm_xor_si128(
                        _mm_srli_epi64(delta, 1),
                        _mm_sub_epi64(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi64x(output[i + chunk_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = output[aligned_size - 1];
        }

        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
        }
#       else
        for (size_t i = 0; i < aligned_size; i += chunk_width) {
            size_t j_max = i + chunk_width;
            for (size_t j = i; j < j_max; ++j) {
                output[j] = initial_value + ((input[j] >> 1) ^ -(input[j] & 1));
                initial_value = output[j];
            }
            initial_value = output[j_max - 1];
        }
        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
            initial_value = output[i];
        }
#       endif
    }

};

#endif // _DFH_COMPRESSION_UTILS_ZIG_ZAG_DELTA_HPP_INCLUDED
