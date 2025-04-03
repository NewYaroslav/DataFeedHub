#pragma once
#ifndef _DTH_DATA_MARKET_BAR_HPP_INCLUDED
#define _DTH_DATA_MARKET_BAR_HPP_INCLUDED

/// \file MarketBar.hpp
/// \brief Defines a compact structure representing a market bar with OHLCV, tick volume, and spread in points.

namespace dfh {

    /// \struct MarketBar
    /// \brief Represents a single market bar with price, volume, spread, and tick volume.
    struct MarketBar {
        uint64_t time_ms;          ///< Start time of the bar (in milliseconds since Unix epoch)
        double   open;             ///< Open price
        double   high;             ///< Highest price during the bar
        double   low;              ///< Lowest price during the bar
        double   close;            ///< Close price
        double   volume;           ///< Traded volume during the bar
        double   quote_volume;     ///< Quote volume during the bar
        double   buy_volume;       ///< Taker buy volume (in base units)
        double   buy_quote_volume; ///< Taker buy volume (in quote units)
        uint32_t spread;           ///< Spread in points (at bar close)
        uint32_t tick_volume;      ///< Number of price updates during the bar

        /// \brief Default constructor. Initializes all fields to zero.
        MarketBar()
            : time_ms(0), open(0.0), high(0.0), low(0.0), close(0.0),
              volume(0.0), quote_volume(0.0), buy_volume(0.0), buy_quote_volume(0.0),
              spread(0), tick_volume(0) {}

        /// \brief Constructs a bar with specified field values.
        /// \param time_ms Start time of the bar (in ms since Unix epoch)
        /// \param open Open price
        /// \param high High price
        /// \param low Low price
        /// \param close Close price
        /// \param volume Traded volume
        /// \param quote_volume Quote volume
        /// \param buy_volume Taker buy volume
        /// \param buy_quote_volume Taker buy quote volume
        /// \param spread Spread in points
        /// \param tick_volume Number of price updates
        MarketBar(
            uint64_t time_ms,
            double open,
            double high,
            double low,
            double close,
            double volume,
            double quote_volume,
            double buy_volume,
            double buy_quote_volume,
            uint32_t spread,
            uint32_t tick_volume)
            : time_ms(time_ms),
              open(open), high(high), low(low), close(close),
              volume(volume), quote_volume(quote_volume),
              buy_volume(buy_volume), buy_quote_volume(buy_quote_volume),
              spread(spread), tick_volume(tick_volume) {}
    };

} // namespace dfh

#endif // _DTH_DATA_MARKET_BAR_HPP_INCLUDED
