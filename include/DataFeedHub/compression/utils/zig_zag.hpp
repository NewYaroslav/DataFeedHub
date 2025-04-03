#pragma once
#ifndef _DFH_COMPRESSION_UTILS_ZIG_ZAG_HPP_INCLUDED
#define _DFH_COMPRESSION_UTILS_ZIG_ZAG_HPP_INCLUDED

/// \file zig_zag.hpp
/// \brief Provides Zig-Zag encoding and decoding utilities for signed integers.

namespace dfh::compression {

    /// \brief Encodes an array of int32_t using Zig-Zag encoding.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    void encode_zig_zag_int32(const int32_t* input, uint32_t* output, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            output[i] = (input[i] << 1) ^ (input[i] >> 31);
        }
    }

    /// \brief Decodes an array of uint32_t from Zig-Zag encoding back to int32_t.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    void decode_zig_zag_int32(const uint32_t* input, int32_t* output, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            output[i] = (input[i] >> 1) ^ -(input[i] & 1);
        }
    }

    /// \brief Encodes an array of int64_t using Zig-Zag encoding.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    void encode_zig_zag_int64(const int64_t* input, uint64_t* output, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            output[i] = (input[i] << 1) ^ (input[i] >> 63);
        }
    }

    /// \brief Decodes an array of uint64_t from Zig-Zag encoding back to int64_t.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    void decode_zig_zag_int64(const uint64_t* input, int64_t* output, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            output[i] = (input[i] >> 1) ^ -(input[i] & 1);
        }
    }

    /// \brief Encodes a single int32_t using Zig-Zag encoding.
    /// \param value Signed 32-bit integer.
    /// \return Unsigned 32-bit Zig-Zag encoded value.
    constexpr uint32_t encode_zig_zag_int32(int32_t value) {
        return (static_cast<uint32_t>(value) << 1) ^ static_cast<uint32_t>(value >> 31);
    }

    /// \brief Decodes a single uint32_t Zig-Zag encoded value back to int32_t.
    /// \param value Unsigned 32-bit Zig-Zag encoded value.
    /// \return Decoded signed 32-bit integer.
    constexpr int32_t decode_zig_zag_int32(uint32_t value) {
        return static_cast<int32_t>(value >> 1) ^ -static_cast<int32_t>(value & 1);
    }

    /// \brief Encodes a single int64_t using Zig-Zag encoding.
    /// \param value Signed 64-bit integer.
    /// \return Unsigned 64-bit Zig-Zag encoded value.
    constexpr uint64_t encode_zig_zag_int64(int64_t value) {
        return (static_cast<uint64_t>(value) << 1) ^ static_cast<uint64_t>(value >> 63);
    }

    /// \brief Decodes a single uint64_t Zig-Zag encoded value back to int64_t.
    /// \param value Unsigned 64-bit Zig-Zag encoded value.
    /// \return Decoded signed 64-bit integer.
    constexpr int64_t decode_zig_zag_int64(uint64_t value) {
        return static_cast<int64_t>(value >> 1) ^ -static_cast<int64_t>(value & 1);
    }

} // namespace dfh::compression

#endif // _DFH_COMPRESSION_UTILS_ZIG_ZAG_HPP_INCLUDED
