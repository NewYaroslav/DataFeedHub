#pragma once
#ifndef _DFH_COMPRESSION_UTILS_ZIG_ZAG_HPP_INCLUDED
#define _DFH_COMPRESSION_UTILS_ZIG_ZAG_HPP_INCLUDED

/// \file zig_zag.hpp
/// \brief

#include <cstdint>

namespace dfh::compression {

    /// \brief Encodes an array of int32_t using Zig-Zag encoding with optional SIMD optimization.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    void encode_zig_zag_int32(const int32_t* input, uint32_t* output, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            output[i] = (input[i] << 1) ^ (input[i] >> 31);
        }
    }

    /// \brief Decodes an array of uint32_t from Zig-Zag encoding back to int32_t with optional SIMD optimization.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    void decode_zig_zag_int32(const uint32_t* input, int32_t* output, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            output[i] = (input[i] >> 1) ^ -(input[i] & 1);
        }
    }

    void encode_zig_zag_int64(const int64_t* input, uint64_t* output, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            output[i] = (input[i] << 1) ^ (input[i] >> 63);
        }
    }

    void decode_zig_zag_int64(const uint64_t* input, int64_t* output, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            output[i] = (input[i] >> 1) ^ -(input[i] & 1);
        }
    }

};

#endif // _DFH_COMPRESSION_UTILS_ZIG_ZAG_HPP_INCLUDED
