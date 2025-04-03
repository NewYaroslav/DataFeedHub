#pragma once
#ifndef _DFH_DATA_MARKET_TICK_HPP_INCLUDED
#define _DFH_DATA_MARKET_TICK_HPP_INCLUDED

/// \file MarketTick.hpp
/// \brief Defines the MarketTick structure and TickType enum

namespace dfh {

    /// \struct MarketTick
    /// \brief Represents a detailed market tick with flags
    struct MarketTick {
        double ask;              ///< Ask price.
        double bid;              ///< Bid price.
        double last;             ///< Price of last trade (Last).
        double volume;           ///< Trade volume in base asset (can store both whole units and high precision).
        uint64_t time_ms;        ///< Tick timestamp in milliseconds.
        uint64_t received_ms;    ///< Time when tick was received from the server.
        TickUpdateFlags flags;   ///< Flags representing tick characteristics.

        /// \brief Default constructor that initializes all fields to zero or equivalent values.
        MarketTick()
            : ask(0.0), bid(0.0), last(0.0), volume(0.0),
              time_ms(0), received_ms(0), flags(TickUpdateFlags::NONE) {}

        /// \brief Constructor to initialize all fields.
        /// \param a Ask price.
        /// \param b Bid price.
        /// \param l Price of last trade.
        /// \param v Trade volume in base asset.
        /// \param ts Tick timestamp in milliseconds.
        /// \param rt Time when tick was received from the server.
        /// \param f Tick flags.
        MarketTick(double a, double b, double l, double v,
                   uint64_t ts, uint64_t rt, TickUpdateFlags f)
            : ask(a), bid(b), last(l), volume(v),
              time_ms(ts), received_ms(rt), flags(f) {}

        /// \brief Sets a specific flag in the tick's flags.
        /// \param flag The flag to set.
        void set_flag(TickUpdateFlags flag) {
            flags |= flag;
        }

        /// \brief Conditionally sets or clears a flag.
        /// \param flag The flag to modify.
        /// \param value True to set the flag, false to clear it.
        void set_flag(TickUpdateFlags flag, bool value) {
            if (value) flags |= flag;
            else flags &= ~flag;
        }

        /// \brief Checks if a specific flag is set in the tick's flags.
        /// \param flag The flag to check.
        /// \return True if the flag is set, otherwise false.
        bool has_flag(TickUpdateFlags flag) const {
            return (flags & flag) != 0;
        }
    };

} // namespace dfh

#endif // _DFH_DATA_MARKET_TICK_HPP_INCLUDED
