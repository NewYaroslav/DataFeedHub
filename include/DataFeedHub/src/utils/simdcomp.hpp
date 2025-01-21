#pragma once
#ifndef _DFH_UTILS_SIMDCOMP_HPP_INCLUDED
#define _DFH_UTILS_SIMDCOMP_HPP_INCLUDED

/// \file simdcomp.hpp
/// \brief

#include <vector>
#include <simdcomp.h>

namespace dfh::utils {

    inline void append_simdcomp(
            std::vector<uint8_t>& binary_data,
            const uint32_t* values,
            size_t length,
            uint32_t bit) {
        if (!length) return;

        size_t num_blocks = length / SIMDBlockSize;
        size_t shortlength = length % SIMDBlockSize;

        std::vector<uint8_t> buffer(128 * sizeof(uint32_t) * (num_blocks + 1));
        uint8_t *buffer_ptr = buffer.data();

        for (size_t k = 0; k < num_blocks; ++k) {
            simdpackwithoutmask(values, reinterpret_cast<__m128i*>(buffer_ptr), bit);
            values += SIMDBlockSize;
            buffer_ptr += bit * sizeof(__m128i);
        }

        if (shortlength > 0) {
            __m128i* endofbuf = simdpack_shortlength(values, shortlength, reinterpret_cast<__m128i*>(buffer_ptr), bit);
            buffer_ptr = buffer.data();
            size_t bytes = (endofbuf - reinterpret_cast<__m128i*>(buffer_ptr)) * sizeof(__m128i);
            binary_data.insert(binary_data.end(), buffer_ptr, buffer_ptr + bytes);
            return;
        }

        size_t bytes = (buffer_ptr - buffer.data());
        binary_data.insert(binary_data.end(), buffer.data(), buffer.data() + bytes);
    }

    inline size_t extract_simdcomp(
			const uint8_t* buffer,
            size_t& offset,
            uint32_t* values,
            size_t length,
            uint32_t bit) {
        size_t start_offset = offset;
        buffer += offset;

        size_t num_blocks = length / SIMDBlockSize;
        size_t shortlength = length % SIMDBlockSize;
        size_t block_size = bit * sizeof(__m128i);

        for (size_t k = 0; k < num_blocks; ++k) {
            simdunpack(reinterpret_cast<const __m128i*>(buffer), values, bit);
            values += SIMDBlockSize;
            buffer += block_size;
            offset += block_size;
        }

        if (shortlength > 0) {
            const __m128i* endofbuf = simdunpack_shortlength(reinterpret_cast<const __m128i*>(buffer), shortlength, values, bit);
            size_t bytes = (endofbuf - reinterpret_cast<const __m128i*>(buffer)) * sizeof(__m128i);
            offset += bytes;
        }

        return offset - start_offset;
    }

//------------------------------------------------------------------------------

    inline void append_simdcomp(
            std::vector<uint8_t>& binary_data,
            const uint32_t* values,
            size_t length) {
        if (!length) return;

        size_t num_blocks = length / SIMDBlockSize;
        size_t shortlength = length % SIMDBlockSize;

        std::vector<uint8_t> buffer(128 * sizeof(uint32_t) * (num_blocks + 1) + (num_blocks + 1));
        uint8_t *buffer_ptr = buffer.data();

        uint32_t bit = 0;
        for (size_t k = 0; k < num_blocks; ++k) {
            bit = maxbits(values);
            *buffer_ptr = static_cast<uint8_t>(bit);
            buffer_ptr++;
            simdpackwithoutmask(values, reinterpret_cast<__m128i*>(buffer_ptr), bit);
            values += SIMDBlockSize;
            buffer_ptr += bit * sizeof(__m128i);
        }

        if (shortlength > 0) {
            bit = maxbits_length(values, shortlength);
            *buffer_ptr = static_cast<uint8_t>(bit);
            buffer_ptr++;
            __m128i* endofbuf = simdpack_shortlength(values, shortlength, reinterpret_cast<__m128i*>(buffer_ptr), bit);
            buffer_ptr = buffer.data();
            size_t bytes = (endofbuf - reinterpret_cast<__m128i*>(buffer_ptr)) * sizeof(__m128i);
            binary_data.insert(binary_data.end(), buffer_ptr, buffer_ptr + bytes);
            return;
        }

        size_t bytes = (buffer_ptr - buffer.data());
        binary_data.insert(binary_data.end(), buffer.data(), buffer.data() + bytes);
    }

    inline size_t extract_simdcomp(
            const uint8_t* buffer,
            size_t& offset,
            uint32_t* values,
            size_t length) {
        size_t start_offset = offset;
        buffer += offset;

        size_t num_blocks = length / SIMDBlockSize;
        size_t shortlength = length % SIMDBlockSize;

        uint8_t bit = 0;
        size_t block_size = 0;
        for (size_t k = 0; k < num_blocks; ++k) {
            bit = *buffer;
            buffer++; offset++;
            simdunpack(reinterpret_cast<const __m128i*>(buffer), values, bit);
            values += SIMDBlockSize;
            block_size = bit * sizeof(__m128i);
            buffer += block_size;
            offset += block_size;
        }

        if (shortlength > 0) {
            bit = *buffer;
            buffer++; offset++;
            const __m128i* endofbuf = simdunpack_shortlength(reinterpret_cast<const __m128i*>(buffer), shortlength, values, bit);
            size_t bytes = (endofbuf - reinterpret_cast<const __m128i*>(buffer)) * sizeof(__m128i);
            offset += bytes;
        }

        return offset - start_offset;
    }

}; // namespace dfh::utils

#endif // _DFH_UTILS_SIMDCOMP_HPP_INCLUDED
