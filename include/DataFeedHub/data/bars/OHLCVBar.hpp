#pragma once
#ifndef _DFH_DATA_OHLCV_BAR_HPP_INCLUDED
#define _DFH_DATA_OHLCV_BAR_HPP_INCLUDED

/// \file OHLCVBar.hpp
/// \brief Defines a basic OHLCV bar (Open, High, Low, Close, Volume).

namespace dfh {

    /// \struct OHLCVBar
    /// \brief Represents a market bar with OHLC prices and volume.
    struct OHLCVBar {
        uint64_t time_ms; ///< Start time of the bar (in milliseconds since Unix epoch)
        double open;      ///< Opening price
        double high;      ///< Highest price
        double low;       ///< Lowest price
        double close;     ///< Closing price
        double volume;    ///< Volume traded during the bar

        /// \brief Default constructor that initializes all fields to zero.
        OHLCVBar() : time_ms(0), open(0.0), high(0.0), low(0.0), close(0.0), volume(0.0) {}

        /// \brief Constructor to initialize all fields.
        /// \param o Open price
        /// \param h High price
        /// \param l Low price
        /// \param c Close price
        /// \param v Traded volume
        /// \param ts Start time of the bar in milliseconds
        OHLCVBar(double o, double h, double l, double c, double v, uint64_t ts)
            : time_ms(ts), open(o), high(h), low(l), close(c), volume(v) {}
    };

} // namespace dfh

#endif // _DFH_DATA_OHLCV_BAR_HPP_INCLUDED
