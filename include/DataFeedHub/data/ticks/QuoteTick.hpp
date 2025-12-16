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

    /// \brief Converts a quote tick into the internal MarketTick representation.
    /// \param quote Source quote tick.
    /// \return MarketTick populated with available quote data.
    [[nodiscard]] inline MarketTick to_market_tick(const QuoteTick& quote) noexcept {
        MarketTick tick;
        tick.time_ms = quote.time_ms;
        tick.received_ms = 0;
        tick.ask = quote.ask;
        tick.bid = quote.bid;
        tick.last = (quote.ask + quote.bid) * 0.5;
        tick.volume = 0.0;
        tick.flags = TickUpdateFlags::NONE;
        return tick;
    }

    /// \brief Reconstructs a QuoteTick from a MarketTick decoded by the compressor.
    /// \param tick Source market tick.
    /// \return QuoteTick restored from the best available data.
    [[nodiscard]] inline QuoteTick from_market_tick(const MarketTick& tick) noexcept {
        double price = tick.last;
        if (price == 0.0 && tick.ask != 0.0) {
            price = tick.ask;
        } else if (price == 0.0 && tick.bid != 0.0) {
            price = tick.bid;
        }
        return QuoteTick(price, price, tick.time_ms, 0);
    }

    static_assert(std::is_trivially_copyable_v<QuoteTick>,
                  "QuoteTick must remain trivially copyable for zero-copy transports.");
    static_assert(sizeof(QuoteTick) == 2 * sizeof(double) + 2 * sizeof(std::uint64_t),
                  "QuoteTick layout changed unexpectedly.");

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Serializes QuoteTick to JSON.
    /// \param j JSON destination.
    /// \param tick Tick to serialize.
    inline void to_json(nlohmann::json& j, const QuoteTick& tick) {
        j = nlohmann::json{{"ask", tick.ask}, {"bid", tick.bid}, {"time_ms", tick.time_ms}};
        if (tick.received_ms != 0) {
            j["received_ms"] = tick.received_ms;
        }
    }

    /// \brief Deserializes QuoteTick from JSON.
    /// \param j JSON source.
    /// \param tick Tick to populate.
    inline void from_json(const nlohmann::json& j, QuoteTick& tick) {
        j.at("ask").get_to(tick.ask);
        j.at("bid").get_to(tick.bid);
        j.at("time_ms").get_to(tick.time_ms);
        tick.received_ms = j.value("received_ms", std::uint64_t{0});
    }

#endif // DFH_USE_JSON && DFH_USE_NLOHMANN_JSON

} // namespace dfh

#endif // _DFH_DATA_QUOTE_TICK_HPP_INCLUDED
