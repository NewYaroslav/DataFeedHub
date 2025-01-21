#pragma once
#ifndef _DFH_MARKET_TICK_HPP_INCLUDED
#define _DFH_MARKET_TICK_HPP_INCLUDED

/// \file MarketTick.hpp
/// \brief Defines the MarketTick structure and TickType enum

#include <cstdint>
#include "flags.hpp"

namespace dfh {

    /// \struct MarketTick
    /// \brief Represents a detailed market tick with flags
    struct MarketTick {
        double ask;              ///< Ask price
        double bid;              ///< Bid price
        double last;             ///< Price of last trade (Last)
        double volume;           ///< Trade volume (can store both whole units and high precision)
        uint64_t time_ms;        ///< Tick timestamp in milliseconds
        uint64_t received_ms;    ///< Time when tick was received from the server
        uint64_t flags;          ///< Flags representing tick characteristics (combination of TickUpdateFlags)

        /// \brief Default constructor that initializes all fields to zero or equivalent values
        MarketTick()
            : ask(0.0), bid(0.0), last(0.0),
              volume(0.0), time_ms(0),
              received_ms(0), flags(0) {}

        /// \brief Constructor to initialize all fields
        /// \param a Ask price
        /// \param b Bid price
        /// \param l Price of last trade (Last)
        /// \param v Trade volume in whole units
        /// \param vr Trade volume with high precision
        /// \param ts Tick timestamp in milliseconds
        /// \param rt Time when tick was received from the server
        /// \param f Flags representing tick characteristics
        MarketTick(double a, double b, double l, double v,
                   uint64_t ts, uint64_t rt, uint64_t f)
            : ask(a), bid(b), last(l), volume(v),
              time_ms(ts), received_ms(rt), flags(f) {}

        /// \brief Sets a specific flag in the tick's flags.
        /// \param flag The flag to set (from TickUpdateFlags).
        void set_flag(TickUpdateFlags flag) {
            flags |= static_cast<uint64_t>(flag);
        }

        void set_flag(TickUpdateFlags flag, bool value) {
            flags |= value ? static_cast<uint64_t>(flag) : 0x00;
        }

        /// \brief Checks if a specific flag is set in the tick's flags.
        /// \param flag The flag to check (from TickUpdateFlags).
        /// \return True if the flag is set, otherwise false.
        bool has_flag(TickUpdateFlags flag) const {
            return (flags & static_cast<uint64_t>(flag)) != 0;
        }
    };

} // namespace dfh

#endif // _DFH_MARKET_TICK_HPP_INCLUDED
