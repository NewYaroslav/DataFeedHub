#pragma once
#ifndef _DFH_DATA_VALUE_TICK_HPP_INCLUDED
#define _DFH_DATA_VALUE_TICK_HPP_INCLUDED

/// \file ValueTick.hpp
/// \brief Defines the ValueTick structure for simplified tick data.

namespace dfh {

    /// \struct ValueTick
    /// \brief Tick that stores a single numeric value and the originating timestamp.
    struct ValueTick {
        double value{0.0};        ///< Single value (e.g., price or indicator).
        std::uint64_t time_ms{0}; ///< Tick timestamp in milliseconds.

        /// \brief Constructor to initialize the tick.
        /// \param v Single value (e.g., price or indicator).
        /// \param ts Tick timestamp in milliseconds.
        constexpr ValueTick(double v, std::uint64_t ts) noexcept : value(v), time_ms(ts) {}
    };

    static_assert(std::is_trivially_copyable_v<ValueTick>,
                  "ValueTick must remain trivially copyable.");

} // namespace dfh

#endif // _DFH_DATA_VALUE_TICK_HPP_INCLUDED
