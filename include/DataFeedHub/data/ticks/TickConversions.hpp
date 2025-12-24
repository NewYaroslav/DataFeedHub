#pragma once
#ifndef _DFH_DATA_TICK_CONVERSIONS_HPP_INCLUDED
#define _DFH_DATA_TICK_CONVERSIONS_HPP_INCLUDED

/// \file TickConversions.hpp
/// \brief Центральный набор конвертеров между DTO тиков и MarketTick.

#include <cstdint>
#include <vector>

#include "DataFeedHub/data/ticks/MarketTick.hpp"
#include "DataFeedHub/data/ticks/QuoteTick.hpp"
#include "DataFeedHub/data/ticks/QuoteTickVol.hpp"
#include "DataFeedHub/data/ticks/QuoteTickL1.hpp"
#include "DataFeedHub/data/ticks/TradeTick.hpp"

namespace dfh {

    /// \brief Конвертеры QuoteType ↔ MarketTick.
    /// \tparam QuoteType Тип DTO тика.
    template<typename QuoteType>
    struct QuoteTickConversion {
        static MarketTick to(const QuoteType& quote) noexcept;
        static QuoteType from(const MarketTick& tick, std::uint64_t trade_id = 0) noexcept;
        static void collect_trade_ids(const QuoteType&, std::vector<std::uint64_t>&) noexcept {}
    };

    template<>
    struct QuoteTickConversion<QuoteTick> {
        static MarketTick to(const QuoteTick& quote) noexcept {
            MarketTick tick{};
            tick.time_ms = quote.time_ms;
            tick.received_ms = 0;
            tick.ask = quote.ask;
            tick.bid = quote.bid;
            tick.last = (quote.ask + quote.bid) * 0.5;
            tick.volume = 0.0;
            tick.flags = TickUpdateFlags::NONE;
            return tick;
        }

        static QuoteTick from(const MarketTick& tick, std::uint64_t trade_id = 0) noexcept {
            double price = tick.last;
            if (price == 0.0 && tick.ask != 0.0) {
                price = tick.ask;
            } else if (price == 0.0 && tick.bid != 0.0) {
                price = tick.bid;
            }
            return QuoteTick(price, price, tick.time_ms, 0);
        }

        static void collect_trade_ids(const QuoteTick&, std::vector<std::uint64_t>&) noexcept {}
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

        static QuoteTickVol from(const MarketTick& tick, std::uint64_t trade_id = 0) noexcept {
            return QuoteTickVol(tick.ask, tick.bid, tick.volume, tick.time_ms, 0);
        }

        static void collect_trade_ids(const QuoteTickVol&, std::vector<std::uint64_t>&) noexcept {}
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

        static QuoteTickL1 from(const MarketTick& tick, std::uint64_t trade_id = 0) noexcept {
            const double half_volume = tick.volume * 0.5;
            return QuoteTickL1(tick.ask, tick.bid, half_volume, half_volume, tick.time_ms, 0);
        }

        static void collect_trade_ids(const QuoteTickL1&, std::vector<std::uint64_t>&) noexcept {}
    };

    template<>
    struct QuoteTickConversion<TradeTick> {
        static MarketTick to(const TradeTick& quote) noexcept {
            MarketTick tick{};
            tick.time_ms = quote.time_ms;
            tick.received_ms = 0;
            tick.ask = quote.price;
            tick.bid = quote.price;
            tick.last = quote.price;
            tick.volume = quote.volume;
            tick.flags = TickUpdateFlags::LAST_UPDATED;
            return tick;
        }

        static TradeTick from(const MarketTick& tick, std::uint64_t trade_id = 0) noexcept {
            return TradeTick(tick.last, tick.volume, tick.time_ms, trade_id, TradeSide::Unknown);
        }

        static void collect_trade_ids(const TradeTick& quote, std::vector<std::uint64_t>& ids) noexcept {
            ids.push_back(quote.trade_id());
        }
    };

} // namespace dfh

#endif // _DFH_DATA_TICK_CONVERSIONS_HPP_INCLUDED
