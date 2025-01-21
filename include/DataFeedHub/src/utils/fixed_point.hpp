#ifndef _DATAFEEDHUB_UTILS_FIXED_POINT_HPP_INCLUDED
#define _DATAFEEDHUB_UTILS_FIXED_POINT_HPP_INCLUDED

/// \file fixed_point.hpp
/// \brief

#include <cstdint>
#include <cmath>
#include <array>
#include <stdexcept>

namespace dfh::utils {

    /// \brief Normalizes a floating-point number to a specified number of decimal places.
    /// \param value The value to normalize.
    /// \param digits The number of decimal places to retain.
    /// \return The normalized value.
    const double normalize_double(double value, size_t digits) {
        if (digits > 18) {
            throw std::invalid_argument("Digits exceed maximum precision (18).");
        }
        static const std::array<double, 19> scale = {
            1.0,                 // 10^0
            10.0,                // 10^1
            100.0,               // 10^2
            1000.0,              // 10^3
            10000.0,             // 10^4
            100000.0,            // 10^5
            1000000.0,           // 10^6
            10000000.0,          // 10^7
            100000000.0,         // 10^8
            1000000000.0,        // 10^9
            10000000000.0,       // 10^10
            100000000000.0,      // 10^11
            1000000000000.0,     // 10^12
            10000000000000.0,    // 10^13
            100000000000000.0,   // 10^14
            1000000000000000.0,  // 10^15
            10000000000000000.0, // 10^16
            100000000000000000.0,// 10^17
            1000000000000000000.0// 10^18
        };
        return std::round(value * scale[digits]) / scale[digits];
    }

    /// \brief Computes the tolerance for comparing floating-point numbers based on decimal precision.
    /// \param digits The number of decimal places to consider.
    /// \return The tolerance value.
    double precision_tolerance(size_t digits) {
        if (digits > 18) {
            throw std::invalid_argument("Digits exceed maximum precision (18).");
        }
        static const std::array<double, 19> tolerance = {
            1.0,                // 10^0
            0.1,                // 10^1
            0.01,               // 10^2
            0.001,              // 10^3
            0.0001,             // 10^4
            0.00001,            // 10^5
            0.000001,           // 10^6
            0.0000001,          // 10^7
            0.00000001,         // 10^8
            0.000000001,        // 10^9
            0.0000000001,       // 10^10
            0.00000000001,      // 10^11
            0.000000000001,     // 10^12
            0.0000000000001,    // 10^13
            0.00000000000001,   // 10^14
            0.000000000000001,  // 10^15
            0.0000000000000001, // 10^16
            0.00000000000000001,// 10^17
            0.000000000000000001// 10^18
        };
        return tolerance[digits];
    }

    /// \brief Converts a floating-point value to a fixed-point integer representation.
    /// \details This function multiplies the given value by a precomputed scaling factor
    ///          (typically a power of 10) to shift the decimal point and rounds the result
    ///          to the nearest integer.
    /// \param value The floating-point value to convert.
    /// \param scaling_factor A precomputed scaling factor, such as a power of 10.
    /// \return The fixed-point integer representation of the input value.
    inline int64_t to_fixed_point(double value, int64_t scaling_factor) {
        return static_cast<int64_t>(std::round(value * static_cast<double>(scaling_factor)));
    }

    inline int64_t to_fixed_point(double value, double scaling_factor) {
        return static_cast<int64_t>(std::round(value * scaling_factor));
    }

	/// \brief Converts a fixed-point integer to a floating-point value.
	/// \param value The fixed-point integer.
	/// \param scale The scaling factor used during encoding (e.g., 10^digits).
	/// \return The corresponding floating-point value.
	inline double from_fixed_point(int64_t value, int64_t scale) {
		return static_cast<double>(value) / static_cast<double>(scale);
	}

} // namespace DataFeedHub

#endif // _DATAFEEDHUB_UTILS_FIXED_POINT_HPP_INCLUDED
