#pragma once
#ifndef _DFH_UTILS_STRING_UTILS_HPP_INCLUDED
#define _DFH_UTILS_STRING_UTILS_HPP_INCLUDED

/// \file string_utils.hpp
/// \brief Вспомогательные функции для преобразования символов и строк в утилитах DataFeedHub.

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <string>

namespace dfh::utils {

    /// \brief Приводит все символы строки к верхнему регистру без учёта локали.
    /// \param str Входящая строка, которая модифицируется in-place.
    /// \return Копия строки, где каждый символ приведён к верхнему регистру.
    inline std::string to_upper_case(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char ch) {
            return std::toupper(ch);
        });
        return str;
    }

    /// \brief Приводит все символы строки к нижнему регистру без учёта локали.
    /// \param str Входящая строка.
    /// \return Копия строки в нижнем регистре.
    inline std::string to_lower_case(std::string str) {
        std::transform(str.begin(), str.end(), str.begin(), [](unsigned char ch) {
            return std::tolower(ch);
        });
        return str;
    }

    /// \brief Преобразует восьмибитное значение в строку с шестнадцатеричным представлением (`0xNN`).
    /// \param value Беззнаковый байт, который требуется отобразить.
    /// \return Строка вида `0xXX`, где XX — заглавные шестнадцатеричные цифры.
    inline std::string convert_hex_to_string(uint8_t value) {
        char hex_string[32] = {};
        std::snprintf(hex_string, sizeof(hex_string), "0x%.2X", value);
        return std::string(hex_string);
    }
}; // namespace dfh::utils

#endif // _DFH_UTILS_STRING_UTILS_HPP_INCLUDED
