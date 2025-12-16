#pragma once
#ifndef _DFH_DATA_QUOTE_TICK_VOL_HPP_INCLUDED
#define _DFH_DATA_QUOTE_TICK_VOL_HPP_INCLUDED

/// \file QuoteTickVol.hpp
/// \brief Defines QuoteTickVol structure for quote ticks with single volume value.

#include "flags.hpp"
#include "MarketTick.hpp"
#include "QuoteTickConversions.hpp"

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

    template<>
    struct QuoteTickConversion<QuoteTickVol> {
        static MarketTick to(const QuoteTickVol& quote) noexcept {
            MarketTick tick{};
            tick.time_ms = quote.time_ms;
            tick.received_ms = 0;
            tick.ask = quote.ask;
            tick.bid = quote.bid;
            tick.last = (quote.ask + quote.bid) * 0.5;
            tick.volume = quote.volume;
            tick.flags = TickUpdateFlags::NONE;
            return tick;
        }

        static QuoteTickVol from(const MarketTick& tick) noexcept {
            return QuoteTickVol(tick.ask, tick.bid, tick.volume, tick.time_ms, 0);
        }
    };

    static_assert(std::is_trivially_copyable_v<QuoteTickVol>,
                  "QuoteTickVol must remain trivially copyable for zero-copy transports.");
    static_assert(sizeof(QuoteTickVol) == 3 * sizeof(double) + 2 * sizeof(std::uint64_t),
                  "QuoteTickVol layout changed unexpectedly.");

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Serializes QuoteTickVol to JSON.
    /// \param j JSON destination.
    /// \param tick Tick to serialize.
    inline void to_json(nlohmann::json& j, const QuoteTickVol& tick) {
        j = nlohmann::json{{"ask", tick.ask},
                           {"bid", tick.bid},
                           {"volume", tick.volume},
                           {"time_ms", tick.time_ms}};
        if (tick.received_ms != 0) {
            j["received_ms"] = tick.received_ms;
        }
    }

    /// \brief Deserializes QuoteTickVol from JSON.
    /// \param j JSON source.
    /// \param tick Tick to populate.
    inline void from_json(const nlohmann::json& j, QuoteTickVol& tick) {
        j.at("ask").get_to(tick.ask);
        j.at("bid").get_to(tick.bid);
        j.at("volume").get_to(tick.volume);
        j.at("time_ms").get_to(tick.time_ms);
        tick.received_ms = j.value("received_ms", std::uint64_t{0});
    }

#endif // DFH_USE_JSON && DFH_USE_NLOHMANN_JSON

} // namespace dfh

#endif // _DFH_DATA_QUOTE_TICK_VOL_HPP_INCLUDED
