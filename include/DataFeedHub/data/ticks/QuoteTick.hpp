#pragma once
#ifndef _DFH_DATA_QUOTE_TICK_HPP_INCLUDED
#define _DFH_DATA_QUOTE_TICK_HPP_INCLUDED

/// \file QuoteTick.hpp
/// \brief Defines the QuoteTick structure for tick data without volume

namespace dfh {

    /// \brief Simplified tick structure without volume
    struct QuoteTick {
        double ask;           ///< Ask price
        double bid;           ///< Bid price
        uint64_t time_ms;     ///< Tick timestamp in milliseconds

        /// \brief Constructor to initialize the tick
        /// \param a Ask price
        /// \param b Bid price
        /// \param ts Tick timestamp in milliseconds
        QuoteTick(double a, double b, uint64_t ts)
            : ask(a), bid(b), time_ms(ts), received_ms(rt) {}
    };
	
	static_assert(std::is_standard_layout<QuoteTick>::value,
              "QuoteTick must be standard-layout");
	static_assert(sizeof(QuoteTick) == 2 * sizeof(double) + sizeof(std::uint64_t),
              "QuoteTick has unexpected size");

} // namespace dfh

#endif // _DFH_DATA_QUOTE_TICK_HPP_INCLUDED
