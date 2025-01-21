#pragma once
#ifndef _DTH_SIMPLE_TICK_HPP_INCLUDED
#define _DTH_SIMPLE_TICK_HPP_INCLUDED

/// \file SimpleTick.hpp
/// \brief Defines the SimpleTick structure for tick data without volume

#include <cstdint>

namespace dfh {

    /// \brief Simplified tick structure without volume
    struct SimpleTick {
        double ask;           ///< Ask price
        double bid;           ///< Bid price
        uint64_t time_ms;     ///< Tick timestamp in milliseconds
		uint64_t received_ms; ///< Time when tick was received from the server

        /// \brief Constructor to initialize the tick
        /// \param a Ask price
        /// \param b Bid price
        /// \param ts Tick timestamp in milliseconds
        /// \param rt Time when tick was received from the server
        SimpleTick(double a, double b, uint64_t ts, uint64_t rt)
            : ask(a), bid(b), time_ms(ts), received_ms(rt) {}
    };

} // namespace dfh

#endif // _DTH_SIMPLE_TICK_HPP_INCLUDED
