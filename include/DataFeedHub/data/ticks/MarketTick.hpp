#pragma once
#ifndef _DFH_DATA_MARKET_TICK_HPP_INCLUDED
#define _DFH_DATA_MARKET_TICK_HPP_INCLUDED

/// \file MarketTick.hpp
/// \brief Defines the MarketTick DTO and helper methods for derived quote metrics.

#include "flags.hpp"                        // TickUpdateFlags (+ operators)
#include <DataFeedHub/utils/math_utils.hpp> // dfh::utils::pow10_clamp
#include <cmath>                            // std::isfinite, std::round
#include <type_traits>

namespace dfh {

    /// \struct MarketTick
    /// \brief A single market tick with timestamps, prices, optional volume, and per-tick update flags.
    ///
    /// \note \c flags describes which fields were updated for this particular tick
    ///       (similar to \c MqlTick::flags in MetaTrader).
    struct MarketTick {
        std::int64_t time_ms{0};          ///< Exchange timestamp in milliseconds since Unix epoch.
        std::int64_t received_ms{0};      ///< Local receive timestamp in milliseconds since Unix epoch (0 if unknown).
        double ask{0.0};                  ///< Best ask price.
        double bid{0.0};                  ///< Best bid price.
        double last{0.0};                 ///< Last trade price.
        double volume{0.0};               ///< Trade volume (optional; may be 0 if not provided).
        TickUpdateFlags flags{TickUpdateFlags::NONE}; ///< Per-tick update flags (which fields were updated).

        /// \brief Default constructor. Initializes all fields to zero.
        constexpr MarketTick() noexcept = default;

        /// \brief Constructs a tick with explicit timestamps, prices, and update flags.
        /// \param time Exchange timestamp in milliseconds since Unix epoch.
        /// \param received Receive timestamp in milliseconds since Unix epoch (0 if unknown).
        /// \param ask_price Best ask price.
        /// \param bid_price Best bid price.
        /// \param last_price Last trade price.
        /// \param volume_value Trade volume (optional).
        /// \param flag_mask Update flags describing which fields were updated.
        constexpr MarketTick(
            std::int64_t time,
            std::int64_t received,
            double ask_price,
            double bid_price,
            double last_price,
            double volume_value,
            TickUpdateFlags flag_mask) noexcept
            : time_ms(time)
            , received_ms(received)
            , ask(ask_price)
            , bid(bid_price)
            , last(last_price)
            , volume(volume_value)
            , flags(flag_mask) {}

        /// \brief Sets (enables) an update flag.
        /// \param flag Flag to set.
        constexpr void set_flag(TickUpdateFlags flag) noexcept { flags |= flag; }

        /// \brief Sets or clears an update flag depending on \p value.
        /// \param flag Flag to modify.
        /// \param value If true, sets the flag; otherwise clears it.
        constexpr void set_flag(TickUpdateFlags flag, bool value) noexcept {
            if (value) flags |= flag;
            else flags &= ~flag;
        }

        /// \brief Checks whether an update flag is set.
        /// \param flag Flag to test.
        /// \return True if the flag is present.
        [[nodiscard]] constexpr bool has_flag(TickUpdateFlags flag) const noexcept {
            return (flags & flag) != TickUpdateFlags::NONE;
        }

        /// \brief Returns the mid price: (ask + bid) / 2.
        /// \return Mid price in price units.
        [[nodiscard]] constexpr double mid_price() const noexcept {
            return (ask + bid) * 0.5;
        }

        /// \brief Returns the mid price rounded to the specified number of decimal digits.
        ///
        /// Rounding is performed as: \c round(mid * 10^digits) / 10^digits.
        ///
        /// \param digits Number of digits after the decimal point. Values above 18 are clamped to 18.
        /// \return Rounded mid price.
        [[nodiscard]] double mid_price(std::size_t digits) const noexcept {
            const double scale = dfh::utils::pow10_clamp<double>(digits);
            const double m = mid_price();
            return std::round(m * scale) / scale;
        }

        /// \brief Returns the spread: ask - bid.
        /// \return Spread in price units.
        [[nodiscard]] constexpr double spread() const noexcept {
            return ask - bid;
        }

        /// \brief Returns the relative spread scaled by \p multiplier using mid price as the denominator.
        ///
        /// Formula: \c spread_scaled = (ask - bid) / mid_price() * multiplier.
        ///
        /// Typical multipliers:
        /// - 10000 -> basis points (bps)
        /// - 100   -> percent
        /// - 1e6   -> ppm
        ///
        /// \param multiplier Scale factor applied to the relative spread.
        /// \return Scaled relative spread. Returns 0.0 if mid price is not positive.
        [[nodiscard]] constexpr double spread_scaled(double multiplier) const noexcept {
            const double m = mid_price();
            return (m > 0.0) ? (spread() / m) * multiplier : 0.0;
        }

        /// \brief Returns the relative spread scaled by 10^digits using mid price as the denominator.
        ///
        /// This is a generic “digits-space” scaling of a dimensionless relative spread.
        /// Example: \p digits = 5 returns \c (ask-bid)/mid * 100000.
        ///
        /// \param digits Power-of-ten exponent used as the scale factor (10^digits). Values above 18 are clamped to 18.
        /// \return Scaled relative spread. Returns 0.0 if mid price is not positive.
        [[nodiscard]] constexpr double spread_digits(std::size_t digits) const noexcept {
            return spread_scaled(dfh::utils::pow10_clamp<double>(digits));
        }

        /// \brief Returns the relative spread in basis points (bps).
        /// \return Relative spread in bps. Returns 0.0 if mid price is not positive.
        [[nodiscard]] constexpr double spread_bps() const noexcept {
            return spread_scaled(10000.0);
        }

        /// \brief Checks whether prices/volume are numerically sane and the quote is not crossed.
        ///
        /// A tick is considered valid if:
        /// - \c ask, \c bid, \c last, \c volume are finite numbers,
        /// - \c ask >= \c bid (not crossed),
        /// - prices are non-negative, and volume is non-negative.
        ///
        /// \note Some feeds may legitimately provide \c ask==0 or \c bid==0 for missing sides.
        ///       If you want strict positivity, tighten the checks accordingly.
        ///
        /// \return True if the tick looks valid.
        [[nodiscard]] bool is_valid() const noexcept {
            return std::isfinite(ask) && std::isfinite(bid)
                && std::isfinite(last) && std::isfinite(volume)
                && ask >= 0.0 && bid >= 0.0 && last >= 0.0 && volume >= 0.0
                && ask >= bid;
        }
    };

    static_assert(std::is_trivially_copyable_v<MarketTick>,
                  "MarketTick must remain trivially copyable for zero-copy I/O and binary serialization.");

    static_assert(sizeof(MarketTick) == 56,
                  "MarketTick size changed unexpectedly (ABI/layout impact).");
                  
    static_assert(alignof(MarketTick) == alignof(double),
                  "MarketTick alignment changed unexpectedly (ABI/layout impact).");

} // namespace dfh

#endif // _DFH_DATA_MARKET_TICK_HPP_INCLUDED

