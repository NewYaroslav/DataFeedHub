#pragma once
#ifndef _DFH_UTILS_STRING_UTILS_HPP_INCLUDED
#define _DFH_UTILS_STRING_UTILS_HPP_INCLUDED

/// \file string_utils.hpp
/// \brief Utility functions for string transformations (case conversion, formatting, etc.).

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <string>

namespace dfh::utils {

    /// \brief Converts a string to uppercase.
    /// \param str Input string.
    /// \return Uppercase version of the input string.
    inline std::string to_upper_case(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char ch) {
            return std::toupper(ch);
        });
        return str;
    }

    /// \brief Converts a string to lowercase.
    /// \param str Input string.
    /// \return Lowercase version of the input string.
    inline std::string to_lower_case(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char ch) {
            return std::tolower(ch);
        });
        return str;
    }

    /// \brief Converts an 8-bit value to a hexadecimal string (e.g., 0x2A).
    /// \param value 8-bit unsigned integer.
    /// \return Hexadecimal representation as a string.
    inline std::string convert_hex_to_string(uint8_t value) {
        char hex_string[32] = {};
        std::snprintf(hex_string, sizeof(hex_string), "0x%.2X", value);
        return std::string(hex_string);
    }
}; // namespace dfh::utils

#endif // _DFH_UTILS_STRING_UTILS_HPP_INCLUDED
