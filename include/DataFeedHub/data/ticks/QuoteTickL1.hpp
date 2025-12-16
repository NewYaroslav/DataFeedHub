#pragma once
#ifndef _DFH_DATA_QUOTE_TICK_L1_HPP_INCLUDED
#define _DFH_DATA_QUOTE_TICK_L1_HPP_INCLUDED

/// \file QuoteTickL1.hpp
/// \brief Defines the QuoteTickL1 structure for L1 quote ticks with bid/ask volumes.

#include "flags.hpp"
#include "MarketTick.hpp"
#include "QuoteTickConversions.hpp"

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

    template<>
    struct QuoteTickConversion<QuoteTickL1> {
        static MarketTick to(const QuoteTickL1& quote) noexcept {
            MarketTick tick{};
            tick.time_ms = quote.time_ms;
            tick.received_ms = 0;
            tick.ask = quote.ask;
            tick.bid = quote.bid;
            tick.last = (quote.ask + quote.bid) * 0.5;
            tick.volume = quote.ask_volume + quote.bid_volume;
            tick.flags = TickUpdateFlags::NONE;
            return tick;
        }

        static QuoteTickL1 from(const MarketTick& tick) noexcept {
            const double half_volume = tick.volume * 0.5;
            return QuoteTickL1(tick.ask, tick.bid, half_volume, half_volume, tick.time_ms, 0);
        }
    };

    static_assert(std::is_trivially_copyable_v<QuoteTickL1>,
                  "QuoteTickL1 must remain trivially copyable.");
    static_assert(sizeof(QuoteTickL1) == 4 * sizeof(double) + 2 * sizeof(std::uint64_t),
                  "QuoteTickL1 layout changed unexpectedly.");

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Serializes QuoteTickL1 to JSON.
    inline void to_json(nlohmann::json& j, const QuoteTickL1& tick) {
        j = nlohmann::json{
            {"ask", tick.ask},
            {"bid", tick.bid},
            {"ask_volume", tick.ask_volume},
            {"bid_volume", tick.bid_volume},
            {"time_ms", tick.time_ms}
        };
        if (tick.received_ms != 0) {
            j["received_ms"] = tick.received_ms;
        }
    }

    /// \brief Deserializes QuoteTickL1 from JSON.
    inline void from_json(const nlohmann::json& j, QuoteTickL1& tick) {
        j.at("ask").get_to(tick.ask);
        j.at("bid").get_to(tick.bid);
        j.at("ask_volume").get_to(tick.ask_volume);
        j.at("bid_volume").get_to(tick.bid_volume);
        j.at("time_ms").get_to(tick.time_ms);
        tick.received_ms = j.value("received_ms", std::uint64_t{0});
    }

#endif // DFH_USE_JSON && DFH_USE_NLOHMANN_JSON

} // namespace dfh

#endif // _DFH_DATA_QUOTE_TICK_L1_HPP_INCLUDED
