#pragma once
#ifndef _DFH_DATA_OHLC_BAR_HPP_INCLUDED
#define _DFH_DATA_OHLC_BAR_HPP_INCLUDED

/// \file OHLCBar.hpp
/// \brief Defines a basic OHLC bar (Open, High, Low, Close) without volume.

namespace dfh {

    /// \struct OHLCBar
    /// \brief Represents a market bar with only OHLC prices (without volume).
    struct OHLCBar {
        uint64_t time_ms; ///< Start time of the bar (in milliseconds since Unix epoch)
        double open;      ///< Opening price
        double high;      ///< Highest price
        double low;       ///< Lowest price
        double close;     ///< Closing price

        /// \brief Default constructor that initializes all fields to zero.
        OHLCBar() : time_ms(0), open(0.0), high(0.0), low(0.0), close(0.0) {}

        /// \brief Constructor to initialize all fields.
        /// \param o Open price
        /// \param h High price
        /// \param l Low price
        /// \param c Close price
        /// \param ts Start time of the bar in milliseconds
        OHLCBar(double o, double h, double l, double c, uint64_t ts)
            : time_ms(ts), open(o), high(h), low(l), close(c) {}
    };

} // namespace dfh

#endif // _DFH_DATA_OHLC_BAR_HPP_INCLUDED
