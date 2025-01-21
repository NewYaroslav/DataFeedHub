#pragma once
#ifndef _DFH_COMPRESSION_UTILS_STRING_UTILS_HPP_INCLUDED
#define _DFH_COMPRESSION_UTILS_STRING_UTILS_HPP_INCLUDED

/// \file string_utils.hpp
/// \brief

namespace dfh::utils {

    /// \brief Converts a string to uppercase.
    /// \param str Input string.
    /// \return Uppercase version of the string.
    std::string to_upper_case(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char ch) {
            return std::toupper(ch);
        });
        return str;
    }

    /// \brief Converts a string to lowercase.
    /// \param str Input string.
    /// \return Lowercase version of the string.
    std::string to_lower_case(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char ch) {
            return std::tolower(ch);
        });
        return str;
    }

    std::string convert_hex_to_string(uint8_t value) {
		char hex_string[32] = {};
		sprintf(hex_string,"0x%.2X", value);
		return std::string(hex_string);
	}
};

#endif // STRING_UTILS_HPP_INCLUDED
