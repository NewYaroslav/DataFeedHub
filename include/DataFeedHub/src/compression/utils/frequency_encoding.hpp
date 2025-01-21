#pragma once
#ifndef _DFH_COMPRESSION_UTILS_FREQUENCY_ENCODING_HPP_INCLUDED
#define _DFH_COMPRESSION_UTILS_FREQUENCY_ENCODING_HPP_INCLUDED

/// \file frequency_encoding.hpp
/// \brief

#include <vector>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cstdint>
#include <immintrin.h>
#include <emmintrin.h> // SSE2
#include <smmintrin.h> // SSE4.1 (если доступен)

namespace dfh::compression {

    /// \brief Encodes an array of values into codes, where the most frequent values get the smallest codes.
    /// \param input_values Pointer to the original array of values.
    /// \param encoded_values Pointer to the array to store the encoded values (same size as input_values).
    /// \param num_values Number of elements in the input array.
    /// \param sorted_values Array to store the sorted unique values.
    /// \param sorted_to_index_map Array to store the codes corresponding to each unique value.
    template<
        class InputType = uint32_t,
        class OutputType = uint32_t,
        class ValuesType = std::vector<InputType>,
        class IndexType = std::vector<uint32_t>>
    void encode_frequency(
            const InputType* input_values,
            OutputType* encoded_values,
            size_t num_values,
            ValuesType& sorted_values,
            IndexType& sorted_to_index_map) {
        // Step 1: Count frequencies
        std::map<InputType, uint32_t> freq_map;
        for (size_t i = 0; i < num_values; ++i) {
            freq_map[input_values[i]]++;
        }

        // Step 2: Create a vector of (frequency, value) pairs and sort it
        std::vector<std::pair<uint32_t, InputType>> freq_pairs;
        freq_pairs.reserve(freq_map.size());
        for (const auto& kv : freq_map) {
            freq_pairs.emplace_back(kv.second, kv.first);
        }

        std::sort(freq_pairs.begin(), freq_pairs.end(),
          [](const auto& a, const auto& b) {
              if (a.first != b.first) return a.first > b.first; // Higher frequency first
              return a.second < b.second;                       // Lower value for equal frequency
          });

        // Step 3: Create value-to-code mapping
        std::unordered_map<InputType, uint32_t> value_to_code;
        uint32_t code = 0;
        for (const auto& pair : freq_pairs) {
            value_to_code[pair.second] = code++;
        }

        // Step 4: Populate sorted_values and sorted_to_index_map
        size_t num_unique = freq_pairs.size();
        sorted_values.resize(num_unique);
        sorted_to_index_map.resize(num_unique);
        for (size_t i = 0; i < num_unique; ++i) {
            sorted_values[i] = freq_pairs[i].second;
            sorted_to_index_map[i] = value_to_code[freq_pairs[i].second];
        }

        // Step 5: Encode input values
        for (size_t i = 0; i < num_values; ++i) {
            encoded_values[i] = value_to_code[input_values[i]];
        }
    }

    /// \brief
    /// \param encoded_values Закодированные индексы (значения)
    /// \param decoded_results Декодированные значения
    /// \param num_encoded Количество закодированных элементов
    /// \param code_to_value Буфер для хранения таблицы преобразования
    /// \param sorted_values Упорядоченные значения (по частоте)
    /// \param sorted_to_index_map Отображение упорядоченных значений на исходные индексы
    /// \param num_values
    inline void decode_frequency(
            const uint64_t* encoded_values,
            uint64_t* decoded_results,
            size_t num_encoded,
            uint64_t* code_to_value,
            const uint64_t* sorted_values,
            const uint32_t* sorted_to_index_map,
            size_t num_values) {
        for (size_t i = 0; i < num_values; ++i) {
            code_to_value[sorted_to_index_map[i]] = sorted_values[i];
        }

#       if defined(__AVX2__)
        constexpr size_t simd_width = 4;
        const size_t aligned_size = num_encoded - (num_encoded % simd_width);

        for (size_t i = 0; i < aligned_size; i += simd_width) {
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&decoded_results[i]),
                _mm256_i64gather_epi64(
                    reinterpret_cast<const long long*>(code_to_value),
                    _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&encoded_values[i])),                                                   // Индексы
                    8));
        }

        for (size_t i = aligned_size; i < num_encoded; ++i) {
            decoded_results[i] = code_to_value[encoded_values[i]];
        }
#       else
        for (size_t i = 0; i < num_encoded; ++i) {
            decoded_results[i] = code_to_value[encoded_values[i]];
        }
#       endif
    }

    /// \brief
    /// \param encoded_values Закодированные индексы (значения)
    /// \param decoded_results Декодированные значения
    /// \param num_encoded Количество закодированных элементов
    /// \param code_to_value Буфер для хранения таблицы преобразования
    /// \param sorted_values Упорядоченные значения (по частоте)
    /// \param sorted_to_index_map Отображение упорядоченных значений на исходные индексы
    /// \param num_values
    inline void decode_frequency(
            const uint32_t* encoded_values,
            uint64_t* decoded_results,
            size_t num_encoded,
            uint64_t* code_to_value,
            const uint64_t* sorted_values,
            const uint32_t* sorted_to_index_map,
            size_t num_values) {

        for (size_t i = 0; i < num_values; ++i) {
            code_to_value[sorted_to_index_map[i]] = sorted_values[i];
        }

#       if defined(__AVX2__)
        constexpr size_t simd_width = 8;
        const size_t aligned_size = num_encoded - (num_encoded % simd_width);
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            __m256i indices_32 = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&encoded_values[i]));

            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&decoded_results[i]),
                _mm256_i64gather_epi64(
                    reinterpret_cast<const long long*>(code_to_value),
                    _mm256_cvtepi32_epi64(_mm256_castsi256_si128(indices_32)),
                    8));
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&decoded_results[i + 4]),
                _mm256_i64gather_epi64(
                    reinterpret_cast<const long long*>(code_to_value),
                    _mm256_cvtepi32_epi64(_mm256_extracti128_si256(indices_32, 1)),
                    8));
        }

        for (size_t i = aligned_size; i < num_encoded; ++i) {
            decoded_results[i] = code_to_value[encoded_values[i]];
        }
#       else
        for (size_t i = 0; i < num_encoded; ++i) {
            decoded_results[i] = code_to_value[encoded_values[i]];
        }
#       endif
    }

    inline void decode_frequency(
            const uint32_t* encoded_values,
            uint32_t* decoded_results,
            size_t num_encoded,
            uint32_t* code_to_value,
            const uint32_t* sorted_values,
            const uint32_t* sorted_to_index_map,
            size_t num_values) {

        for (size_t i = 0; i < num_values; ++i) {
            code_to_value[sorted_to_index_map[i]] = sorted_values[i];
        }

#       if defined(__AVX2__)
        constexpr size_t simd_width = 8;
        const size_t aligned_size = num_encoded - (num_encoded % simd_width);

        for (size_t i = 0; i < aligned_size; i += simd_width) {
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&decoded_results[i]),
                _mm256_i32gather_epi32(
                    reinterpret_cast<const int*>(code_to_value),
                    _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&encoded_values[i])),
                    4));
        }

        for (size_t i = aligned_size; i < num_encoded; ++i) {
            decoded_results[i] = code_to_value[encoded_values[i]];
        }
#       else
        for (size_t i = 0; i < num_encoded; ++i) {
            decoded_results[i] = code_to_value[encoded_values[i]];
        }
#       endif
    }

};

#endif // _DFH_COMPRESSION_UTILS_FREQUENCY_ENCODING_HPP_INCLUDED
