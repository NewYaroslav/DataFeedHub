#include <cassert>
#include <type_traits>

#include "DataFeedHub/data/ticks/MarketTick.hpp"
#include "DataFeedHub/data/ticks/QuoteTick.hpp"
#include "DataFeedHub/data/ticks/QuoteTickVol.hpp"
#include "DataFeedHub/data/ticks/QuoteTickL1.hpp"
#include "DataFeedHub/data/ticks/TradeTick.hpp"
#include "DataFeedHub/data/ticks/ValueTick.hpp"
#include "DataFeedHub/data/ticks/TickCodecConfig.hpp"
#include "DataFeedHub/data/ticks/TickMetadata.hpp"
#include "DataFeedHub/data/ticks/TickSequence.hpp"
#include "DataFeedHub/data/ticks/SingleTick.hpp"
#include "DataFeedHub/data/ticks/TickSpan.hpp"
#include "DataFeedHub/data/ticks/BidAskRestoreConfig.hpp"
#include "DataFeedHub/data/ticks/enums.hpp"
#include "DataFeedHub/data/ticks/flags.hpp"

int main() {
    static_assert(sizeof(dfh::MarketTick) > 0);
    static_assert(sizeof(dfh::QuoteTick) > 0);
    static_assert(sizeof(dfh::QuoteTickVol) > 0);
    static_assert(sizeof(dfh::QuoteTickL1) > 0);
    static_assert(sizeof(dfh::TradeTick) > 0);
    static_assert(sizeof(dfh::ValueTick) > 0);
    static_assert(sizeof(dfh::TickCodecConfig) > 0);
    static_assert(sizeof(dfh::TickMetadata) > 0);
    static_assert(sizeof(dfh::BidAskRestoreConfig) > 0);

    static_assert(std::is_trivially_copyable_v<dfh::MarketTick>);
    static_assert(std::is_trivially_copyable_v<dfh::QuoteTick>);
    static_assert(std::is_trivially_copyable_v<dfh::QuoteTickVol>);
    static_assert(std::is_trivially_copyable_v<dfh::QuoteTickL1>);
    static_assert(std::is_trivially_copyable_v<dfh::TradeTick>);
    static_assert(std::is_trivially_copyable_v<dfh::ValueTick>);
    static_assert(std::is_trivially_copyable_v<dfh::TickCodecConfig>);
    static_assert(std::is_trivially_copyable_v<dfh::TickMetadata>);
    static_assert(std::is_trivially_copyable_v<dfh::BidAskRestoreConfig>);

    dfh::TickSequence<dfh::QuoteTick> seq{};
    assert(seq.ticks.empty());

    dfh::SingleTick<dfh::MarketTick> single{};
    assert(single.symbol_index == 0);

    dfh::QuoteTickSpan span{};
    assert(span.empty());

    return 0;
}
