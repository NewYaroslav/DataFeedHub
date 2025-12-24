#pragma once
#ifndef _DFH_DATA_QUOTE_TICK_HPP_INCLUDED
#define _DFH_DATA_QUOTE_TICK_HPP_INCLUDED

#include "flags.hpp"
#include "MarketTick.hpp"

/// \file QuoteTick.hpp
/// \brief Defines the QuoteTick structure for tick data without volume and helpers.

namespace dfh {

    /// \brief Forward declaration of MarketTick to keep conversions inline.
    struct MarketTick;

    /// \struct QuoteTick
    /// \brief Simplified quote tick that stores bid/ask prices and timestamps.
    struct QuoteTick {
        double ask{0.0};           ///< Ask price.
        double bid{0.0};           ///< Bid price.
        std::uint64_t time_ms{0};  ///< Tick timestamp in milliseconds.
        std::uint64_t received_ms{0}; ///< When the quote was received (ms since epoch).

        /// \brief Constructs a quote tick with both timestamps.
        /// \param a Ask price.
        /// \param b Bid price.
        /// \param ts Exchange timestamp in milliseconds.
        /// \param recv_ms Receive timestamp in milliseconds (0 if unknown).
        constexpr QuoteTick(double a, double b, std::uint64_t ts, std::uint64_t recv_ms = 0) noexcept
            : ask(a), bid(b), time_ms(ts), received_ms(recv_ms) {}
    };

    static_assert(std::is_trivially_copyable_v<QuoteTick>,
                  "QuoteTick must remain trivially copyable for zero-copy transports.");
    static_assert(sizeof(QuoteTick) == 2 * sizeof(double) + 2 * sizeof(std::uint64_t),
                  "QuoteTick layout changed unexpectedly.");

} // namespace dfh

#include "DataFeedHub/data/ticks/TickConversions.hpp"

#endif // _DFH_DATA_QUOTE_TICK_HPP_INCLUDED
