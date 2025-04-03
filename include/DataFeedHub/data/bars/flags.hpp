#pragma once
#ifndef _DTH_DATA_BAR_FLAGS_HPP_INCLUDED
#define _DTH_DATA_BAR_FLAGS_HPP_INCLUDED

/// \file flags.hpp
/// \brief Defines flags for bar data status and update events

namespace dfh {

    /// \enum BarStatusFlags
    /// \brief Flags indicating the status of bar data (real-time, historical, etc.)
    enum class BarStatusFlags : uint32_t {
        NONE        = 0,        ///< No flags set
        REALTIME    = 1 << 0,   ///< Data received in real-time
        HISTORICAL  = 1 << 1,   ///< Data has been initialized from history
        INCOMPLETE  = 1 << 2,   ///< Bar is still forming
        FINALIZED   = 1 << 3    ///< Bar is finalized and will not change
    };

    /// \enum BarStorageFlags
    /// \brief Flags controlling bar data encoding, compression, and interpretation
    enum class BarStorageFlags : uint32_t {
        NONE                    = 0,        ///< No special flags
        BID_BASED               = 1 << 0,   ///< Bar prices are based on bid price
        ASK_BASED               = 1 << 1,   ///< Bar prices are based on ask price
        LAST_BASED              = 1 << 2,   ///< Bar prices are based on last trade price
        ENABLE_VOLUME           = 1 << 3,   ///< Store volume field (actual traded volume)
        ENABLE_QUOTE_VOLUME     = 1 << 4,   ///< Store quote_volume field (volume in quote currency)
        ENABLE_TICK_VOLUME      = 1 << 5,   ///< Store tick volume field (price change count)
        ENABLE_BUY_VOLUME       = 1 << 6,   ///< Store buy_volume field (taker buy volume in base currency)
        ENABLE_BUY_QUOTE_VOLUME = 1 << 7,   ///< Store buy_quote_volume field (taker buy volume in quote currency)
        ENABLE_SPREAD           = 1 << 8,   ///< Store spread data (in points)
        SPREAD_LAST             = 1 << 9,   ///< Spread is stored as the last value in interval
        SPREAD_AVG              = 1 << 10,  ///< Spread is stored as average over interval
        SPREAD_MAX              = 1 << 11,  ///< Spread is stored as maximum in interval
        STORE_RAW_BINARY        = 1 << 12,  ///< Store data in raw binary format (no compression)
        FINALIZED_BARS          = 1 << 13   ///< All bars in the dataset are fully finalized (no incomplete bar at end)
    };

} // namespace dfh

#endif // _DTH_DATA_BAR_FLAGS_HPP_INCLUDED
