#pragma once
#ifndef _DFH_DATA_TICK_ENUMS_HPP_INCLUDED
#define _DFH_DATA_TICK_ENUMS_HPP_INCLUDED

/// \file enums.hpp
/// \brief Enumerations shared across the tick domain.

namespace dfh {

    /// \enum BidAskModel
    /// \brief Algorithm used to restore bid/ask prices from last-trade data.
    enum class BidAskModel : std::uint16_t {
        NONE = 0,            ///< No bid/ask restoration (use raw data if available).
        FIXED_SPREAD   = 1,  ///< Apply a fixed spread to the last price.
        DYNAMIC_SPREAD = 2,  ///< Derive spread dynamically from short-term volatility.
        MEDIAN_SPREAD  = 3   ///< Use median spread estimated from historical history.
    };

    static_assert(sizeof(BidAskModel) == sizeof(std::uint16_t),
                  "BidAskModel size must remain 16-bit for compact storage.");

    /// \enum TradeSide
    /// \brief Direction of an executed trade.
    enum class TradeSide : std::uint8_t {
        Unknown = 0, ///< Trade direction is unknown or not reported.
        Buy     = 1, ///< Aggressor lifted the ask (buy).
        Sell    = 2  ///< Aggressor hit the bid (sell).
    };

    static_assert(sizeof(TradeSide) == sizeof(std::uint8_t),
                  "TradeSide size must remain 8-bit for packing.");

} // namespace dfh

#endif // _DFH_DATA_TICK_ENUMS_HPP_INCLUDED
