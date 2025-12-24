#include <cassert>
#include <type_traits>

// Тонкие DTO заголовки (по одному include в секции).
#include <DataFeedHub/data/ticks/QuoteTick.hpp>
#include <DataFeedHub/data/ticks/QuoteTickVol.hpp>
#include <DataFeedHub/data/ticks/QuoteTickL1.hpp>
#include <DataFeedHub/data/ticks/TradeTick.hpp>
#include <DataFeedHub/data/ticks/MarketTick.hpp>

// Конверсии тиков.
#include <DataFeedHub/data/ticks/TickConversions.hpp>

// JSON для тиков.
#define DFH_USE_JSON 1
#define DFH_USE_NLOHMANN_JSON 1
#include <DataFeedHub/data/ticks/TickJson.hpp>

// Umbrella (подключаем после тонких заголовков).
#include <DataFeedHub/data.hpp>

int main() {
    dfh::QuoteTick quote{101.0, 100.5, 1700000000000ULL, 0};
    dfh::QuoteTickVol quote_vol{101.0, 100.5, 2.0, 1700000000000ULL, 0};
    dfh::QuoteTickL1 quote_l1{101.0, 100.5, 1.0, 1.0, 1700000000000ULL, 0};
    dfh::TradeTick trade{101.0, 3.0, 1700000000000ULL, 42ULL, dfh::TradeSide::Buy};
    dfh::MarketTick market{};

    static_assert(std::is_trivially_copyable_v<dfh::QuoteTick>);
    static_assert(std::is_trivially_copyable_v<dfh::QuoteTickVol>);
    static_assert(std::is_trivially_copyable_v<dfh::QuoteTickL1>);
    static_assert(std::is_trivially_copyable_v<dfh::TradeTick>);
    static_assert(std::is_trivially_copyable_v<dfh::MarketTick>);

    auto converted = dfh::QuoteTickConversion<dfh::QuoteTick>::to(quote);
    assert(converted.time_ms == quote.time_ms);

    nlohmann::json json_tick = market;
    auto market_back = json_tick.get<dfh::MarketTick>();
    assert(market_back.time_ms == market.time_ms);

    (void)quote_vol;
    (void)quote_l1;
    (void)trade;

    return 0;
}