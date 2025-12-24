#pragma once
#ifndef _DFH_DATA_QUOTE_TICK_VOL_HPP_INCLUDED
#define _DFH_DATA_QUOTE_TICK_VOL_HPP_INCLUDED

/// \file QuoteTickVol.hpp
/// \brief Defines QuoteTickVol structure for quote ticks with single volume value.

#include "flags.hpp"
#include "MarketTick.hpp"

namespace dfh {

    /// \struct QuoteTickVol
    /// \brief Quote tick with bid/ask, a single provider-specific volume, and timestamps.
    struct QuoteTickVol {
        double ask{0.0};                 ///< Ask price.
        double bid{0.0};                 ///< Bid price.
        double volume{0.0};              ///< Provider-specific volume metric.
        std::uint64_t time_ms{0};        ///< Exchange timestamp in milliseconds.
        std::uint64_t received_ms{0};    ///< Receive timestamp in milliseconds (0 when unknown).

        /// \brief Constructs a tick with both timestamps.
        /// \param a Ask price.
        /// \param b Bid price.
        /// \param v Provider-specific volume metric.
        /// \param ts Exchange timestamp in milliseconds.
        /// \param recv_ms Receive timestamp in milliseconds (0 if unavailable).
        constexpr QuoteTickVol(double a,
                               double b,
                               double v,
                               std::uint64_t ts,
                               std::uint64_t recv_ms = 0) noexcept
            : ask(a)
            , bid(b)
            , volume(v)
            , time_ms(ts)
            , received_ms(recv_ms) {}
    };

    static_assert(std::is_trivially_copyable_v<QuoteTickVol>,
                  "QuoteTickVol must remain trivially copyable for zero-copy transports.");
    static_assert(sizeof(QuoteTickVol) == 3 * sizeof(double) + 2 * sizeof(std::uint64_t),
                  "QuoteTickVol layout changed unexpectedly.");

} // namespace dfh

#include "DataFeedHub/data/ticks/TickConversions.hpp"

#endif // _DFH_DATA_QUOTE_TICK_VOL_HPP_INCLUDED
