#pragma once
#ifndef _DFH_UTILS_VBYTE_HPP_INCLUDED
#define _DFH_UTILS_VBYTE_HPP_INCLUDED

/// \file vbyte.hpp
/// \brief Utility functions for VByte compression and decompression (32/64-bit).

#include <cstddef>
#include <cstdint>
#include <vector>

#include <vbyte.h>

namespace dfh::utils {

    /// \brief Appends a single compressed integer to a byte buffer.
    /// \tparam T Integer type (uint32_t or uint64_t).
    /// \param binary_data Output buffer to append the compressed bytes.
    /// \param value Integer value to compress and append.
    template <typename T>
    inline void append_vbyte(std::vector<uint8_t>& binary_data, T value);

    /// \brief Specialization for uint32_t compression.
    template <>
    inline void append_vbyte<uint32_t>(std::vector<uint8_t>& binary_data, uint32_t value) {
        uint8_t buffer[5];
        size_t bytes = vbyte_compress_unsorted32(&value, &buffer[0], 1);
        const uint8_t* ptr = &buffer[0];
        binary_data.insert(binary_data.end(), ptr, ptr + bytes);
    }

    /// \brief Specialization for uint64_t compression.
    template <>
    inline void append_vbyte<uint64_t>(std::vector<uint8_t>& binary_data, uint64_t value) {
        uint8_t buffer[10];
        size_t bytes = vbyte_compress_unsorted64(&value, &buffer[0], 1);
        const uint8_t* ptr = &buffer[0];
        binary_data.insert(binary_data.end(), ptr, ptr + bytes);
    }

    /// \brief Extracts a single compressed integer from a byte buffer.
    /// \tparam T Integer type (uint32_t or uint64_t).
    /// \param buffer Pointer to the input buffer.
    /// \param offset Input/output offset in the buffer.
    /// \return Decompressed integer value.
    template <typename T>
    inline T extract_vbyte(const uint8_t* buffer, size_t& offset);

    /// \brief Specialization for uint32_t decompression.
    template <>
    inline uint32_t extract_vbyte<uint32_t>(const uint8_t* buffer, size_t& offset) {
        uint32_t value;
        offset += vbyte_uncompress_unsorted32(buffer + offset, &value, 1);
        return value;
    }

    /// \brief Specialization for uint64_t decompression.
    template <>
    inline uint64_t extract_vbyte<uint64_t>(const uint8_t* buffer, size_t& offset) {
        uint64_t value;
        offset += vbyte_uncompress_unsorted64(buffer + offset, &value, 1);
        return value;
    }

//------------------------------------------------------------------------------

    /// \brief Appends a compressed array of integers to a byte buffer.
    /// \tparam T Integer type (uint32_t or uint64_t).
    /// \param binary_data Output buffer to append the compressed bytes.
    /// \param values Pointer to the array of integers.
    /// \param length Number of values in the array.
    template <typename T>
    inline void append_vbyte(std::vector<uint8_t>& binary_data, const T* values, size_t length);

    /// \brief Specialization for compressing a uint32_t array.
    template <>
    inline void append_vbyte<uint32_t>(std::vector<uint8_t>& binary_data, const uint32_t* values, size_t length) {
        size_t old_size = binary_data.size();
        binary_data.resize(old_size + length * 5);
        size_t bytes = vbyte_compress_unsorted32(values, &binary_data[old_size], length);
        binary_data.resize(old_size + bytes);
    }

    /// \brief Specialization for compressing a uint64_t array.
    template <>
    inline void append_vbyte<uint64_t>(std::vector<uint8_t>& binary_data, const uint64_t* values, size_t length) {
        size_t old_size = binary_data.size();
        binary_data.resize(old_size + length * 10);
        size_t bytes = vbyte_compress_unsorted64(values, &binary_data[old_size], length);
        binary_data.resize(old_size + bytes);
    }

    /// \brief Extracts a compressed array of uint32_t values from a byte buffer.
    /// \param buffer Pointer to the input buffer.
    /// \param offset Input/output offset in the buffer.
    /// \param values Output array to store decompressed values.
    /// \param length Number of values to decompress.
    /// \return Number of bytes read from the buffer.
    inline size_t extract_vbyte(const uint8_t* buffer, size_t& offset, uint32_t* values, size_t length) {
        size_t bytes = vbyte_uncompress_unsorted32(buffer + offset, values, length);
        offset += bytes;
        return bytes;
    }

    /// \brief Extracts a compressed array of uint64_t values from a byte buffer.
    /// \param buffer Pointer to the input buffer.
    /// \param offset Input/output offset in the buffer.
    /// \param values Output array to store decompressed values.
    /// \param length Number of values to decompress.
    /// \return Number of bytes read from the buffer.
    inline size_t extract_vbyte(const uint8_t* buffer, size_t& offset, uint64_t* values, size_t length) {
        size_t bytes = vbyte_uncompress_unsorted64(buffer + offset, values, length);
        offset += bytes;
        return bytes;
    }

//------------------------------------------------------------------------------

}; // namespace dfh::utils

#endif // _DFH_UTILS_VBYTE_HPP_INCLUDED
