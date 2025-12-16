#pragma once
#ifndef _DFH_DATA_TICK_QUOTE_CONVERSIONS_HPP_INCLUDED
#define _DFH_DATA_TICK_QUOTE_CONVERSIONS_HPP_INCLUDED

/// \file QuoteTickConversions.hpp
/// \brief Declares conversion helpers between quote DTOs and MarketTick.

#include "MarketTick.hpp"

namespace dfh {

    /// \brief Traits for converting between a QuoteType and MarketTick.
    template<typename QuoteType>
    struct QuoteTickConversion;

} // namespace dfh

#endif // _DFH_DATA_TICK_QUOTE_CONVERSIONS_HPP_INCLUDED
