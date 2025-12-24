#include <cassert>
#include <cstdint>

#define DFH_USE_JSON 1
#define DFH_USE_NLOHMANN_JSON 1

#include <nlohmann/json.hpp>

#include "DataFeedHub/data/ticks/TickJson.hpp"

int main() {
    dfh::MarketTick market{};
    market.time_ms = 1700000000000ULL;
    market.received_ms = 1700000000100ULL;
    market.ask = 101.5;
    market.bid = 101.0;
    market.last = 101.25;
    market.volume = 2.5;
    market.flags = dfh::TickUpdateFlags::ASK_UPDATED;

    nlohmann::json market_json = market;
    auto market_back = market_json.get<dfh::MarketTick>();
    assert(market_back.time_ms == market.time_ms);
    assert(market_back.received_ms == market.received_ms);
    assert(market_back.ask == market.ask);
    assert(market_back.bid == market.bid);
    assert(market_back.last == market.last);
    assert(market_back.volume == market.volume);
    assert(market_back.flags == market.flags);

    dfh::QuoteTick quote{102.5, 102.0, 1700000000001ULL, 1700000000101ULL};
    nlohmann::json quote_json = quote;
    dfh::QuoteTick quote_back{0.0, 0.0, 0ULL, 0ULL};
    quote_json.get_to(quote_back);
    assert(quote_back.ask == quote.ask);
    assert(quote_back.bid == quote.bid);
    assert(quote_back.time_ms == quote.time_ms);
    assert(quote_back.received_ms == quote.received_ms);

    dfh::QuoteTickVol quote_vol{103.5, 103.0, 6.5, 1700000000002ULL, 1700000000102ULL};
    nlohmann::json quote_vol_json = quote_vol;
    dfh::QuoteTickVol quote_vol_back{0.0, 0.0, 0.0, 0ULL, 0ULL};
    quote_vol_json.get_to(quote_vol_back);
    assert(quote_vol_back.ask == quote_vol.ask);
    assert(quote_vol_back.bid == quote_vol.bid);
    assert(quote_vol_back.volume == quote_vol.volume);
    assert(quote_vol_back.time_ms == quote_vol.time_ms);
    assert(quote_vol_back.received_ms == quote_vol.received_ms);

    dfh::QuoteTickL1 quote_l1{104.5, 104.0, 1.0, 2.0, 1700000000003ULL, 1700000000103ULL};
    nlohmann::json quote_l1_json = quote_l1;
    auto quote_l1_back = quote_l1_json.get<dfh::QuoteTickL1>();
    assert(quote_l1_back.ask == quote_l1.ask);
    assert(quote_l1_back.bid == quote_l1.bid);
    assert(quote_l1_back.ask_volume == quote_l1.ask_volume);
    assert(quote_l1_back.bid_volume == quote_l1.bid_volume);
    assert(quote_l1_back.time_ms == quote_l1.time_ms);
    assert(quote_l1_back.received_ms == quote_l1.received_ms);

    dfh::TradeTick trade{105.5, 4.0, 1700000000004ULL, 1234ULL, dfh::TradeSide::Sell};
    nlohmann::json trade_json = trade;
    auto trade_back = trade_json.get<dfh::TradeTick>();
    assert(trade_back.price == trade.price);
    assert(trade_back.volume == trade.volume);
    assert(trade_back.time_ms == trade.time_ms);
    assert(trade_back.trade_id() == trade.trade_id());
    assert(trade_back.trade_side() == trade.trade_side());

    dfh::ValueTick value_tick{106.5, 1700000000005ULL};
    nlohmann::json value_json = value_tick;
    dfh::ValueTick value_back{0.0, 0ULL};
    value_json.get_to(value_back);
    assert(value_back.value == value_tick.value);
    assert(value_back.time_ms == value_tick.time_ms);

    dfh::TickSequence<dfh::MarketTick> sequence{};
    sequence.ticks.push_back(market);
    sequence.flags = dfh::TickUpdateFlags::BID_UPDATED;
    sequence.symbol_index = 7;
    sequence.provider_index = 3;
    sequence.price_digits = 2;
    sequence.volume_digits = 4;
    nlohmann::json seq_json = sequence;
    auto seq_back = seq_json.get<dfh::TickSequence<dfh::MarketTick>>();
    assert(seq_back.ticks.size() == 1);
    assert(seq_back.ticks[0].ask == market.ask);
    assert(seq_back.flags == sequence.flags);
    assert(seq_back.symbol_index == sequence.symbol_index);

    dfh::SingleTick<dfh::MarketTick> single{};
    single.tick = market;
    single.flags = dfh::TickUpdateFlags::ASK_UPDATED;
    single.symbol_index = 9;
    single.provider_index = 4;
    single.price_digits = 2;
    single.volume_digits = 3;
    nlohmann::json single_json = single;
    auto single_back = single_json.get<dfh::SingleTick<dfh::MarketTick>>();
    assert(single_back.tick.ask == market.ask);
    assert(single_back.flags == single.flags);
    assert(single_back.symbol_index == single.symbol_index);

    dfh::TickCodecConfig cfg{};
    cfg.tick_size = 0.01;
    cfg.expiration_time_ms = 1700001000000ULL;
    cfg.next_expiration_time_ms = 1700002000000ULL;
    cfg.flags = dfh::TickStorageFlags::ENABLE_VOLUME;
    cfg.price_digits = 2;
    cfg.volume_digits = 4;
    cfg.reserved = {1, 2};
    nlohmann::json cfg_json = cfg;
    auto cfg_back = cfg_json.get<dfh::TickCodecConfig>();
    assert(cfg_back.tick_size == cfg.tick_size);
    assert(cfg_back.expiration_time_ms == cfg.expiration_time_ms);
    assert(cfg_back.next_expiration_time_ms == cfg.next_expiration_time_ms);
    assert(cfg_back.flags == cfg.flags);
    assert(cfg_back.price_digits == cfg.price_digits);
    assert(cfg_back.volume_digits == cfg.volume_digits);
    assert(cfg_back.reserved[0] == cfg.reserved[0]);

    dfh::TickMetadata meta{};
    meta.start_time_ms = 1700000000000ULL;
    meta.end_time_ms = 1700000000100ULL;
    meta.expiration_time_ms = 1700001000000ULL;
    meta.next_expiration_time_ms = 1700002000000ULL;
    meta.count = 12;
    meta.tick_size = 0.5;
    meta.symbol_id = 1;
    meta.exchange_id = 2;
    meta.market_type = dfh::MarketType::SPOT;
    meta.price_digits = 2;
    meta.volume_digits = 4;
    meta.flags = dfh::TickStorageFlags::ENABLE_VOLUME;
    nlohmann::json meta_json = meta;
    auto meta_back = meta_json.get<dfh::TickMetadata>();
    assert(meta_back.start_time_ms == meta.start_time_ms);
    assert(meta_back.end_time_ms == meta.end_time_ms);
    assert(meta_back.market_type == meta.market_type);
    assert(meta_back.flags == meta.flags);

    dfh::BidAskRestoreConfig restore{};
    restore.mode = dfh::BidAskModel::FIXED_SPREAD;
    restore.fixed_spread = 12;
    restore.price_digits = 2;
    nlohmann::json restore_json = restore;
    auto restore_back = restore_json.get<dfh::BidAskRestoreConfig>();
    assert(restore_back.mode == restore.mode);
    assert(restore_back.fixed_spread == restore.fixed_spread);
    assert(restore_back.price_digits == restore.price_digits);

    return 0;
}
