#include <cassert>
#include <cstdint>

#include "DataFeedHub/data/ticks/TickConversions.hpp"

int main() {
    dfh::QuoteTick quote{101.25, 101.0, 1700000000001ULL, 0};
    dfh::MarketTick market_from_quote = dfh::QuoteTickConversion<dfh::QuoteTick>::to(quote);
    assert(market_from_quote.time_ms == quote.time_ms);
    assert(market_from_quote.ask == quote.ask);
    assert(market_from_quote.bid == quote.bid);

    dfh::QuoteTickVol quote_vol{102.0, 101.5, 12.5, 1700000000002ULL, 0};
    dfh::MarketTick market_from_vol = dfh::QuoteTickConversion<dfh::QuoteTickVol>::to(quote_vol);
    assert(market_from_vol.volume == quote_vol.volume);

    dfh::QuoteTickL1 quote_l1{103.0, 102.0, 5.0, 7.0, 1700000000003ULL, 0};
    dfh::MarketTick market_from_l1 = dfh::QuoteTickConversion<dfh::QuoteTickL1>::to(quote_l1);
    assert(market_from_l1.volume == quote_l1.ask_volume + quote_l1.bid_volume);

    dfh::TradeTick trade{104.0, 3.5, 1700000000004ULL, 42ULL, dfh::TradeSide::Buy};
    dfh::MarketTick market_from_trade = dfh::QuoteTickConversion<dfh::TradeTick>::to(trade);
    assert(market_from_trade.last == trade.price);
    assert(market_from_trade.volume == trade.volume);
    assert(market_from_trade.flags == dfh::TickUpdateFlags::LAST_UPDATED);

    dfh::MarketTick source{};
    source.time_ms = 1700000000005ULL;
    source.ask = 105.5;
    source.bid = 105.0;
    source.last = 105.25;
    source.volume = 9.0;
    source.flags = dfh::TickUpdateFlags::ASK_UPDATED;

    dfh::QuoteTick from_market_quote = dfh::QuoteTickConversion<dfh::QuoteTick>::from(source, 0);
    assert(from_market_quote.time_ms == source.time_ms);

    dfh::QuoteTickVol from_market_vol = dfh::QuoteTickConversion<dfh::QuoteTickVol>::from(source, 0);
    assert(from_market_vol.volume == source.volume);

    dfh::QuoteTickL1 from_market_l1 = dfh::QuoteTickConversion<dfh::QuoteTickL1>::from(source, 0);
    assert(from_market_l1.ask == source.ask);
    assert(from_market_l1.bid == source.bid);

    dfh::TradeTick from_market_trade = dfh::QuoteTickConversion<dfh::TradeTick>::from(source, 99ULL);
    assert(from_market_trade.time_ms == source.time_ms);
    assert(from_market_trade.trade_id() == 99ULL);

    return 0;
}