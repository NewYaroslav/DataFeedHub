#pragma once
#ifndef _DFH_DATA_VALUE_TICK_HPP_INCLUDED
#define _DFH_DATA_VALUE_TICK_HPP_INCLUDED

/// \file ValueTick.hpp
/// \brief Defines the ValueTick structure for simplified tick data

#include <cstdint>

namespace dfh {

    /// \brief Simplified tick structure with a single value and timestamp
    struct ValueTick {
        double value;       ///< Single value (e.g., price or indicator)
        uint64_t time_ms;   ///< Tick timestamp in milliseconds

        /// \brief Constructor to initialize the tick
        /// \param v Single value (e.g., price or indicator)
        /// \param ts Tick timestamp in milliseconds
        ValueTick(double v, uint64_t ts)
            : value(v), time_ms(ts) {}
    };

} // namespace dfh

#endif // _DFH_DATA_VALUE_TICK_HPP_INCLUDED
