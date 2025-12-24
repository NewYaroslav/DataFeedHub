#pragma once
#ifndef _DFH_DATA_QUOTE_TICK_L1_HPP_INCLUDED
#define _DFH_DATA_QUOTE_TICK_L1_HPP_INCLUDED

/// \file QuoteTickL1.hpp
/// \brief Defines the QuoteTickL1 structure for L1 quote ticks with bid/ask volumes.

#include <cstdint>
#include <type_traits>

namespace dfh {

    /// \brief L1 quote tick with bid/ask prices and volumes
    struct QuoteTickL1 {
        double ask{0.0};             ///< Best ask price.
        double bid{0.0};             ///< Best bid price.
        double ask_volume{0.0};      ///< Volume at best ask.
        double bid_volume{0.0};      ///< Volume at best bid.
        std::uint64_t time_ms{0};    ///< Tick timestamp in milliseconds.
        std::uint64_t received_ms{0};///< Receive timestamp in milliseconds.

        /// \brief Default constructor. Initializes all fields to zero.
        constexpr QuoteTickL1() noexcept = default;

    /// \brief Constructor to initialize all fields.
    /// \param a Best ask price
    /// \param b Best bid price
    /// \param av Volume at best ask
        /// \param bv Volume at best bid
        /// \param ts Tick timestamp in milliseconds
        constexpr QuoteTickL1(double a,
                              double b,
                              double av,
                              double bv,
                              std::uint64_t ts,
                              std::uint64_t recv_ms = 0) noexcept
            : ask(a)
            , bid(b)
            , ask_volume(av)
            , bid_volume(bv)
            , time_ms(ts)
            , received_ms(recv_ms) {}
    };

    static_assert(std::is_trivially_copyable_v<QuoteTickL1>,
                  "QuoteTickL1 must remain trivially copyable.");
    static_assert(sizeof(QuoteTickL1) == 4 * sizeof(double) + 2 * sizeof(std::uint64_t),
                  "QuoteTickL1 layout changed unexpectedly.");

} // namespace dfh

#endif // _DFH_DATA_QUOTE_TICK_L1_HPP_INCLUDED
