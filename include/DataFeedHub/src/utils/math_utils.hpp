#pragma once
#ifndef _DATAFEEDHUB_UTILS_MATH_UTILS_HPP_INCLUDED
#define _DATAFEEDHUB_UTILS_MATH_UTILS_HPP_INCLUDED

/// \file math_utils.hpp
/// \brief Provides utility math functions for use within the DataFeedHub library.

#include <cmath>
#include <stdexcept>

namespace dfh::utils {

    /// \brief Returns a power of 10 based on the number of digits.
    /// \tparam T Return type, which can be any numeric type (e.g., int64_t, uint64_t, double).
    /// \param digits Number of decimal places (must be in the range 0-18).
    /// \return Power of 10 corresponding to the given number of digits.
    /// \throws std::out_of_range If `digits` is not in the range [0, 18].
    template <typename T>
    inline const T pow10(size_t digits) {
        // Precomputed powers of 10 for digits from 0 to 18
        static const int64_t powers_of_ten[19] = {
            1LL,                 // 10^0
            10LL,                // 10^1
            100LL,               // 10^2
            1000LL,              // 10^3
            10000LL,             // 10^4
            100000LL,            // 10^5
            1000000LL,           // 10^6
            10000000LL,          // 10^7
            100000000LL,         // 10^8
            1000000000LL,        // 10^9
            10000000000LL,       // 10^10
            100000000000LL,      // 10^11
            1000000000000LL,     // 10^12
            10000000000000LL,    // 10^13
            100000000000000LL,   // 10^14
            1000000000000000LL,  // 10^15
            10000000000000000LL, // 10^16
            100000000000000000LL,// 10^17
            1000000000000000000LL// 10^18
        };

        if (digits > 18) throw std::out_of_range("Digits must be in the range 0-18.");
        return static_cast<T>(powers_of_ten[digits]);
    }

    /// \brief Возвращает медианное значение из трёх входных.
    /// \param a Первое значение.
    /// \param b Второе значение.
    /// \param c Третье значение.
    /// \return Медианное значение из трёх.
    template <typename T>
    T median_filter(T a, T b, T c) {
        if ((a >= b && a <= c) || (a <= b && a >= c)) {
            return a;
        } else
        if ((b >= a && b <= c) || (b <= a && b >= c)) {
            return b;
        }
        return c;
    }
} // namespace dfh::utils

#endif // _DATAFEEDHUB_UTILS_MATH_UTILS_HPP_INCLUDED
