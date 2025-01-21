#pragma once
#ifndef _DFH_UTILS_VBYTE_HPP_INCLUDED
#define _DFH_UTILS_VBYTE_HPP_INCLUDED

/// \file vbyte.hpp
/// \brief

#include <vector>
#include <vbyte.h>

namespace dfh::utils {

    template <typename T>
    inline void append_vbyte(std::vector<uint8_t>& binary_data, T value);

    template <>
    inline void append_vbyte<uint32_t>(std::vector<uint8_t>& binary_data, uint32_t value) {
        uint8_t buffer[5];
        size_t bytes = vbyte_compress_unsorted32(&value, &buffer[0], 1);
        const uint8_t* ptr = &buffer[0];
        binary_data.insert(binary_data.end(), ptr, ptr + bytes);
    }

    template <>
    inline void append_vbyte<uint64_t>(std::vector<uint8_t>& binary_data, uint64_t value) {
        uint8_t buffer[10];
        size_t bytes = vbyte_compress_unsorted64(&value, &buffer[0], 1);
        const uint8_t* ptr = &buffer[0];
        binary_data.insert(binary_data.end(), ptr, ptr + bytes);
    }

    // Обобщённая функция для извлечения одного значения из буфера
    template <typename T>
    inline T extract_vbyte(const uint8_t* buffer, size_t& offset);

    template <>
    inline uint32_t extract_vbyte<uint32_t>(const uint8_t* buffer, size_t& offset) {
        uint32_t value;
        offset += vbyte_uncompress_unsorted32(buffer + offset, &value, 1);
        return value;
    }

    template <>
    inline uint64_t extract_vbyte<uint64_t>(const uint8_t* buffer, size_t& offset) {
        uint64_t value;
        offset += vbyte_uncompress_unsorted64(buffer + offset, &value, 1);
        return value;
    }

//------------------------------------------------------------------------------

    template <typename T>
    inline void append_vbyte(std::vector<uint8_t>& binary_data, const T* values, size_t length);

    template <>
    inline void append_vbyte<uint32_t>(std::vector<uint8_t>& binary_data, const uint32_t* values, size_t length) {
        size_t old_size = binary_data.size();
        binary_data.resize(old_size + length * 5);
        size_t bytes = vbyte_compress_unsorted32(values, &binary_data[old_size], length);
        binary_data.resize(old_size + bytes);
    }

    template <>
    inline void append_vbyte<uint64_t>(std::vector<uint8_t>& binary_data, const uint64_t* values, size_t length) {
        size_t old_size = binary_data.size();
        binary_data.resize(old_size + length * 10);
        size_t bytes = vbyte_compress_unsorted64(values, &binary_data[old_size], length);
        binary_data.resize(old_size + bytes);
    }

    size_t extract_vbyte(const uint8_t* buffer, size_t& offset, uint32_t* values, size_t length) {
        size_t bytes = vbyte_uncompress_unsorted32(buffer + offset, values, length);
        offset += bytes;
        return bytes;
    }

    size_t extract_vbyte(const uint8_t* buffer, size_t& offset, uint64_t* values, size_t length) {
        size_t bytes = vbyte_uncompress_unsorted64(buffer + offset, values, length);
        offset += bytes;
        return bytes;
    }

//------------------------------------------------------------------------------

}; // namespace dfh::utils

#endif // _DFH_UTILS_VBYTE_HPP_INCLUDED
