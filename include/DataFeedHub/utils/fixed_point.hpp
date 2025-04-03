#pragma once
#ifndef _DFH_UTILS_FIXED_POINT_HPP_INCLUDED
#define _DFH_UTILS_FIXED_POINT_HPP_INCLUDED

/// \file fixed_point.hpp
/// \brief Utilities for fixed-point arithmetic and precision control.

namespace dfh::utils {

    /// \brief Normalizes a floating-point value to specified decimal precision.
    /// \details Rounds the value to the nearest representable number with given
    ///          decimal digits using precomputed scaling factors for efficiency.
    /// \param value Input value to normalize
    /// \param digits Number of decimal places to preserve (0-18)
    /// \return Normalized value with specified precision
    /// \throw std::invalid_argument If digits exceed maximum supported precision
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

    /// \brief Computes comparison tolerance for specified decimal precision.
    /// \details Returns 0.5 * 10^(-digits) to account for rounding errors.
    /// \param digits Number of significant decimal places (0-18)
    /// \return Tolerance value for floating-point comparisons
    /// \throw std::invalid_argument If digits exceed maximum supported precision
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

    /// \brief Converts floating-point value to fixed-point integer representation.
    /// \details Uses scaling factor to preserve decimal precision. For optimal results,
    ///          scaling_factor should be a power of 10 matching the desired precision.
    /// \param value Floating-point value to convert
    /// \param scaling_factor Multiplier for decimal preservation (e.g., 1000 for 3 decimals)
    /// \return Fixed-point integer value
    inline int64_t to_fixed_point(double value, int64_t scaling_factor) {
        return static_cast<int64_t>(std::round(value * static_cast<double>(scaling_factor)));
    }

	/// \overload
    /// \param scaling_factor Double precision scaling factor
    inline int64_t to_fixed_point(double value, double scaling_factor) {
        return static_cast<int64_t>(std::round(value * scaling_factor));
    }

	/// \brief Converts fixed-point integer back to floating-point value.
    /// \param value Fixed-point integer to decode
    /// \param scale Scaling factor used during encoding (must match original scaling)
    /// \return Reconstructed floating-point value
	inline double from_fixed_point(int64_t value, int64_t scale) {
		return static_cast<double>(value) / static_cast<double>(scale);
	}

	/// \brief Compares two floating-point values with specified precision.
    /// \details Uses precomputed tolerance values to account for rounding errors.
    /// \param value1 First comparison operand
    /// \param value2 Second comparison operand
    /// \param digits Number of decimal places to consider
    /// \return true if |value1 - value2| <= tolerance, false otherwise
    /// \throw std::invalid_argument If digits exceed maximum supported precision
    inline bool compare_with_precision(double value1, double value2, size_t digits) {
        if (digits > 18) {
            throw std::invalid_argument("Digits exceed maximum precision (18).");
        }
        double tolerance = precision_tolerance(digits);
        return std::fabs(value1 - value2) <= tolerance;
    }

} // namespace dfh::utils

#endif // _DFH_UTILS_FIXED_POINT_HPP_INCLUDED
