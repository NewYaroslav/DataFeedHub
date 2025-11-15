#pragma once
#ifndef _DFH_DATA_QUOTE_TICK_L1_HPP_INCLUDED
#define _DFH_DATA_QUOTE_TICK_L1_HPP_INCLUDED

/// \file QuoteTickL1.hpp
/// \brief Defines the QuoteTickL1 structure for L1 quote ticks with bid/ask volumes

namespace dfh {

    /// \brief L1 quote tick with bid/ask prices and volumes
    struct QuoteTickL1 {
        double   ask;         ///< Best ask price
        double   bid;         ///< Best bid price
        double   ask_volume;  ///< Volume at best ask
        double   bid_volume;  ///< Volume at best bid
        uint64_t time_ms;     ///< Tick timestamp in milliseconds

        /// \brief Default constructor. Initializes all fields to zero.
        QuoteTickL1() noexcept
            : ask(0.0)
            , bid(0.0)
            , ask_volume(0.0)
            , bid_volume(0.0)
            , time_ms(0) {}

        /// \brief Constructor to initialize all fields.
        /// \param a Best ask price
        /// \param b Best bid price
        /// \param av Volume at best ask
        /// \param bv Volume at best bid
        /// \param ts Tick timestamp in milliseconds
        QuoteTickL1(double a,
                    double b,
                    double av,
                    double bv,
                    uint64_t ts) noexcept
            : ask(a)
            , bid(b)
            , ask_volume(av)
            , bid_volume(bv)
            , time_ms(ts) {}
    };

    static_assert(std::is_standard_layout<QuoteTickL1>::value,
                  "QuoteTickL1 must be standard-layout");

} // namespace dfh

#endif // _DFH_DATA_QUOTE_TICK_L1_HPP_INCLUDED
