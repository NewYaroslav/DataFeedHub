#pragma once
#ifndef _DFH_COMPRESSION_UTILS_FREQUENCY_ENCODING_HPP_INCLUDED
#define _DFH_COMPRESSION_UTILS_FREQUENCY_ENCODING_HPP_INCLUDED

/// \file frequency_encoding.hpp
/// \brief Frequency-based dictionary remap: most frequent values get smallest codes.
///
/// This utility builds a dictionary of unique values ordered by decreasing frequency
/// (ties are resolved by value ascending) and replaces the original values by dictionary
/// codes (0..num_unique-1).
///
/// Decoding restores values by direct lookup: value = dict_values[code].
///
/// \note This is not entropy coding; it is a fast dictionary remap intended to improve
/// downstream compression.
/// \note The decoder does not validate code ranges. The caller must ensure that all codes
/// are in [0, num_unique).

namespace dfh::compression {

    /// \brief Encodes input values into codes by descending frequency.
    ///
    /// \tparam InputType  Value type (e.g., std::uint32_t, std::uint64_t).
    /// \tparam OutputType Code type (typically std::uint32_t).
    /// \tparam ValuesType Dictionary container type (typically std::vector<InputType>).
    ///
    /// \param input_values      Input array of size \p num_values.
    /// \param encoded_values    Output array of codes of size \p num_values.
    /// \param num_values        Number of input elements.
    /// \param dict_values       Output dictionary values in code order (size = num_unique).
    /// \param expected_uniques  Expected number of unique values (used for reserve()).
    ///
    /// Complexity:
    /// - Counting: O(N) expected
    /// - Sorting uniques: O(U log U), U = number of unique values
    /// - Encoding: O(N) expected
    template<
        class InputType = std::uint32_t,
        class OutputType = std::uint32_t,
        class ValuesType = std::vector<InputType>>
    void encode_frequency(
            const InputType* input_values,
            OutputType* encoded_values,
            std::size_t num_values,
            ValuesType& dict_values,
            std::size_t expected_uniques = 16384
        ) {
        // Fast path: empty input
        if (num_values == 0u) {
            dict_values.clear();
            return;
        }

        // Step 1: Count frequencies (expected U ~ 4k..20k).
        std::unordered_map<InputType, std::uint32_t> freq_map;
        freq_map.reserve(expected_uniques);

        for (std::size_t i = 0; i < num_values; ++i) {
            ++freq_map[input_values[i]];
        }

        // Step 2: Collect pairs (freq, value) and sort.
        using Pair = std::pair<std::uint32_t, InputType>; // (freq, value)
        std::vector<Pair> freq_pairs;
        freq_pairs.reserve(freq_map.size());
        for (const auto& kv : freq_map) {
            freq_pairs.emplace_back(kv.second, kv.first);
        }

        std::sort(freq_pairs.begin(), freq_pairs.end(),
          [](const Pair& a, const Pair& b) {
              if (a.first != b.first) return a.first > b.first; // Higher frequency first
              return a.second < b.second;                       // Lower value for equal frequency
          });
          
        // Step 3: Dictionary values in code order (code == index in dict_values).
        std::size_t num_unique = freq_pairs.size();
        dict_values.resize(num_unique);

        for (std::size_t i = 0; i < num_unique; ++i) {
            dict_values[i] = freq_pairs[i].second;
        }
        
        // Step 4: Build value -> code table for encoding.
        std::unordered_map<InputType, std::uint32_t> value_to_code;
        value_to_code.reserve(num_unique);

        for (std::size_t i = 0; i < num_unique; ++i) {
#           if __cplusplus >= 201703L
            // C++17+
            value_to_code.try_emplace(dict_values[i], static_cast<std::uint32_t>(i));
#           else
            // C++11/14
            value_to_code.emplace(dict_values[i], static_cast<std::uint32_t>(i));
#           endif
        }

        // Step 5: Encode input values.
        for (std::size_t i = 0; i < num_values; ++i) {
            encoded_values[i] = static_cast<OutputType>(value_to_code.find(input_values[i])->second);
        }
    }

    /// \brief Scalar decoder (baseline implementation).
    ///
    /// \tparam CodeT  Code type (e.g., std::uint32_t, std::uint64_t).
    /// \tparam ValueT Value type (e.g., std::uint32_t, std::uint64_t).
    ///
    /// \param encoded_values  Input codes array of size \p num_encoded.
    /// \param decoded_results Output values array of size \p num_encoded.
    /// \param num_encoded     Number of elements to decode.
    /// \param dict_values     Dictionary values in code order.
    template<class CodeT, class ValueT>
    inline void decode_frequency_scalar(
            const CodeT* encoded_values,
            ValueT* decoded_results,
            std::size_t num_encoded,
            const ValueT* dict_values
        ) {
        for (std::size_t i = 0; i < num_encoded; ++i) {
            decoded_results[i] = dict_values[static_cast<std::size_t>(encoded_values[i])];
        }
    }

#   if defined(__AVX2__)
    
    /// \brief AVX2 decoder: uint64 codes -> uint64 values using gather.
    inline void decode_frequency_avx2_u64_to_u64(
            const std::uint64_t* encoded_values,
            std::uint64_t* decoded_results,
            std::size_t num_encoded,
            const std::uint64_t* dict_values
        ) {
        constexpr std::size_t simd_width = 4;
        const std::size_t aligned_size = num_encoded - (num_encoded % simd_width);
        const long long* base = reinterpret_cast<const long long*>(dict_values);

        std::size_t i = 0;
        for (; i < aligned_size; i += simd_width) {
            const __m256i idx = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&encoded_values[i]));
            const __m256i gathered = _mm256_i64gather_epi64(base, idx, 8);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&decoded_results[i]), gathered);
        }
        for (; i < num_encoded; ++i) {
            decoded_results[i] = dict_values[encoded_values[i]];
        }
    }

    /// \brief AVX2 decoder: uint32 codes -> uint64 values using gather.
    inline void decode_frequency_avx2_u32_to_u64(
            const std::uint32_t* encoded_values,
            std::uint64_t* decoded_results,
            std::size_t num_encoded,
            const std::uint64_t* dict_values
        ) {
        constexpr std::size_t simd_width = 8;
        const std::size_t aligned_size = num_encoded - (num_encoded % simd_width);
        const long long* base = reinterpret_cast<const long long*>(dict_values);

        std::size_t i = 0;
        for (; i < aligned_size; i += simd_width) {
            const __m256i idx32 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&encoded_values[i]));

            const __m128i lo128 = _mm256_castsi256_si128(idx32);
            const __m128i hi128 = _mm256_extracti128_si256(idx32, 1);

            const __m256i idx64_lo = _mm256_cvtepi32_epi64(lo128);
            const __m256i idx64_hi = _mm256_cvtepi32_epi64(hi128);

            const __m256i gathered_lo = _mm256_i64gather_epi64(base, idx64_lo, 8);
            const __m256i gathered_hi = _mm256_i64gather_epi64(base, idx64_hi, 8);

            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&decoded_results[i]), gathered_lo);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&decoded_results[i + 4]), gathered_hi);
        }

        for (; i < num_encoded; ++i) {
            decoded_results[i] = dict_values[encoded_values[i]];
        }
    }
    
    /// \brief AVX2 decoder: uint32 codes -> uint32 values using gather.
    inline void decode_frequency_avx2_u32_to_u32(
            const std::uint32_t* encoded_values,
            std::uint32_t* decoded_results,
            std::size_t num_encoded,
            const std::uint32_t* dict_values
        ) {
        constexpr std::size_t simd_width = 8;
        const std::size_t aligned_size = num_encoded - (num_encoded % simd_width);
        const int* base = reinterpret_cast<const int*>(dict_values);

        std::size_t i = 0;
        for (; i < aligned_size; i += simd_width) {
            const __m256i idx = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&encoded_values[i]));
            const __m256i gathered = _mm256_i32gather_epi32(base, idx, 4);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&decoded_results[i]), gathered);
        }

        for (; i < num_encoded; ++i) {
            decoded_results[i] = dict_values[encoded_values[i]];
        }
    }

#   endif

    /// \brief Decodes uint64 codes into uint64 values.
    ///
    /// \param encoded_values  Input codes array of size \p num_encoded.
    /// \param decoded_results Output values array of size \p num_encoded.
    /// \param num_encoded     Number of codes to decode.
    /// \param dict_values     Dictionary values in code order.
    inline void decode_frequency(
            const std::uint64_t* encoded_values,
            std::uint64_t* decoded_results,
            std::size_t num_encoded,
            const std::uint64_t* dict_values) {
#       if defined(__AVX2__)
        decode_frequency_avx2_u64_to_u64(encoded_values, decoded_results, num_encoded, dict_values);
#       else
        decode_frequency_scalar(encoded_values, decoded_results, num_encoded, dict_values);
#       endif
    }

    /// \brief Decodes uint32 codes into uint64 values.
    ///
    /// \param encoded_values  Input codes array of size \p num_encoded.
    /// \param decoded_results Output values array of size \p num_encoded.
    /// \param num_encoded     Number of codes to decode.
    /// \param dict_values     Dictionary values in code order.
    inline void decode_frequency(
            const std::uint32_t* encoded_values,
            std::uint64_t* decoded_results,
            std::size_t num_encoded,
            const std::uint64_t* dict_values) {
#       if defined(__AVX2__)
        decode_frequency_avx2_u32_to_u64(encoded_values, decoded_results, num_encoded, dict_values);
#       else
        decode_frequency_scalar(encoded_values, decoded_results, num_encoded, dict_values);
#       endif
    }

    /// \brief Decodes uint32 codes into uint32 values.
    ///
    /// \param encoded_values  Input codes array of size \p num_encoded.
    /// \param decoded_results Output values array of size \p num_encoded.
    /// \param num_encoded     Number of codes to decode.
    /// \param dict_values     Dictionary values in code order.
    inline void decode_frequency(
            const std::uint32_t* encoded_values,
            std::uint32_t* decoded_results,
            std::size_t num_encoded,
            const std::uint32_t* dict_values) {
#       if defined(__AVX2__)
        decode_frequency_avx2_u32_to_u32(encoded_values, decoded_results, num_encoded, dict_values);
#       else
        decode_frequency_scalar(encoded_values, decoded_results, num_encoded, dict_values);
#       endif
    }

} // namespace dfh::compression

#endif // _DFH_COMPRESSION_UTILS_FREQUENCY_ENCODING_HPP_INCLUDED
