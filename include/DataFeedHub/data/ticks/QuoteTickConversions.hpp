#pragma once
#ifndef _DFH_DATA_TICK_QUOTE_CONVERSIONS_HPP_INCLUDED
#define _DFH_DATA_TICK_QUOTE_CONVERSIONS_HPP_INCLUDED

/// \file QuoteTickConversions.hpp
/// \brief Declares conversion helpers between quote DTOs and MarketTick.

#include "MarketTick.hpp"
#include <cstdint>
#include <vector>

namespace dfh {

    template<typename QuoteType>
    struct QuoteTickConversion {
        static MarketTick to(const QuoteType& quote) noexcept;
        static QuoteType from(const MarketTick& tick, std::uint64_t trade_id = 0) noexcept;
        static void collect_trade_ids(const QuoteType&, std::vector<uint64_t>&) noexcept {}
    };

} // namespace dfh

#endif // _DFH_DATA_TICK_QUOTE_CONVERSIONS_HPP_INCLUDED
