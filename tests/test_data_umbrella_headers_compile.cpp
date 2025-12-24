#include <cassert>

#define DFH_USE_JSON 1
#define DFH_USE_NLOHMANN_JSON 1

#include <nlohmann/json.hpp>

#include "DataFeedHub/data.hpp"
#include "DataFeedHub/data/ticks/TickConversions.hpp"
#include "DataFeedHub/data/ticks/TickJson.hpp"

int main() {
    dfh::QuoteTick quote{101.0, 100.5, 1700000000000ULL, 1700000000100ULL};
    dfh::MarketTick market = dfh::QuoteTickConversion<dfh::QuoteTick>::to(quote);
    assert(market.time_ms == quote.time_ms);

    nlohmann::json j = market;
    auto decoded = j.get<dfh::MarketTick>();
    assert(decoded.time_ms == market.time_ms);

    return 0;
}