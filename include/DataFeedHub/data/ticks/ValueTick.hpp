#pragma once
#ifndef _DFH_DATA_VALUE_TICK_HPP_INCLUDED
#define _DFH_DATA_VALUE_TICK_HPP_INCLUDED

/// \file ValueTick.hpp
/// \brief Defines the ValueTick structure for simplified tick data.

#include <cstdint>
#include <cmath>
#include <type_traits>

namespace dfh {

    /// \struct ValueTick
    /// \brief Tick that stores a single numeric value and the originating timestamp.
    struct ValueTick {
        std::int64_t time_ms{0};  ///< Tick timestamp in milliseconds.
        double value{0.0};        ///< Single value (e.g., price or indicator).

        /// \brief Default constructor. Initializes all fields to zero.
        constexpr ValueTick() noexcept = default;

        /// \brief Constructs a value tick.
        /// \param ts Tick timestamp in milliseconds since Unix epoch.
        /// \param v Value (e.g., price or indicator).
        constexpr ValueTick(std::int64_t ts, double v) noexcept
            : time_ms(ts), value(v) {}

        /// \brief Checks whether \c value is finite.
        /// \return True if \c value is finite.
        bool is_valid() const noexcept {
            return std::isfinite(value);
        }
    };

    static_assert(std::is_trivially_copyable_v<ValueTick>,
                  "ValueTick must remain trivially copyable for zero-copy I/O and binary serialization.");

    static_assert(sizeof(ValueTick) == 16,
                  "ValueTick size changed unexpectedly (ABI/layout impact).");
                  
    static_assert(alignof(ValueTick) == alignof(double),
                  "ValueTick alignment changed unexpectedly (ABI/layout impact).");

} // namespace dfh

#endif // _DFH_DATA_VALUE_TICK_HPP_INCLUDED
