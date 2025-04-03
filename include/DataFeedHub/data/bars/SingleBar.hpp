#pragma once
#ifndef _DTH_DATA_SINGLE_BAR_HPP_INCLUDED
#define _DTH_DATA_SINGLE_BAR_HPP_INCLUDED

/// \file SingleBar.hpp
/// \brief

namespace dfh {

    /// \struct SingleBar
    /// \brief Represents a single market bar of any type along with its metadata.
    /// \tparam BarType Type of bar (e.g., OHLCBar, OHLCVBar, MarketBar).
    template <typename BarType>
    struct SingleBar {
        BarType    bar;                     ///< Bar data of the specified type.
        uint64_t   expiration_time_ms;      ///< Expiration time for futures (0 for perpetual or spot).
        uint64_t   next_expiration_time_ms; ///< Expiration time of the next contract (0 if not defined).
        double     tick_size;               ///< Minimum price increment (tick size).
        TimeFrame  time_frame;              ///< Timeframe used for aggregation.
        BarStatusFlags flags;               ///< Flags indicating the status of the bar (e.g., realtime, finalized).
        uint16_t   symbol_id;               ///< Symbol identifier.
        uint16_t   exchange_id;             ///< Exchange identifier.
        MarketType market_type;             ///< Market type (e.g., SPOT, FUTURES, OPTIONS).
        uint8_t    price_digits;            ///< Number of decimal places used for price values.
        uint8_t    volume_digits;           ///< Number of decimal places used for volume values.
        uint8_t    quote_volume_digits;     ///< Number of decimal places used for quote volume values.

        /// \brief Default constructor initializes all fields to zero or defaults.
        SingleBar()
            : expiration_time_ms(0),
              next_expiration_time_ms(0),
              tick_size(0.0),
              time_frame(TimeFrame::UNKNOWN),
              flags(BarStatusFlags::NONE),
              symbol_id(0),
              exchange_id(0),
              market_type(MarketType::UNKNOWN),
              price_digits(0),
              volume_digits(0),
              quote_volume_digits(0) {}
    };

    using SingleOHLCBar   = SingleBar<OHLCBar>;    ///< Single OHLC bar with metadata.
    using SingleOHLCVBar  = SingleBar<OHLCVBar>;   ///< Single OHLCV bar with metadata.
    using SingleMarketBar = SingleBar<MarketBar>;  ///< Single MarketBar (OHLCV + tick/spread) with metadata.

} // namespace dfh

#endif // _DTH_DATA_SINGLE_BAR_HPP_INCLUDED
