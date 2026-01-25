#pragma once
#ifndef _DFH_DATA_QUOTE_TICK_HPP_INCLUDED
#define _DFH_DATA_QUOTE_TICK_HPP_INCLUDED

/// \file QuoteTick.hpp
/// \brief Defines the QuoteTick structure for tick data without volume and helpers.

#include <DataFeedHub/utils/math_utils.hpp> // dfh::utils::pow10_clamp
#include <cmath>                            // std::isfinite, std::round
#include <type_traits>

namespace dfh {

    /// \struct QuoteTick
    /// \brief Simplified quote tick that stores bid/ask prices and timestamps.
    struct QuoteTick {
        std::int64_t time_ms{0};  ///< Tick timestamp in milliseconds.
        double ask{0.0};          ///< Ask price.
        double bid{0.0};          ///< Bid price.

        /// \brief Constructs a quote tick with both timestamps.
        /// \param ts Exchange timestamp in milliseconds.
        /// \param a Ask price.
        /// \param b Bid price.
        constexpr QuoteTick(
                std::int64_t ts, 
                double a, 
                double b) noexcept : 
            time_ms(ts),
            ask(a),
            bid(b) {}
            
        /// \brief Returns the mid price: (ask + bid) / 2.
        /// \return Mid price.
        constexpr double mid_price() const noexcept {
            return (ask + bid) * 0.5;
        }
        
        /// \brief Returns the mid price rounded to the specified number of decimal digits.
        ///
        /// Rounding is performed as: \c round(mid * 10^digits) / 10^digits.
        ///
        /// \param digits Number of digits after the decimal point. Values above 18 are clamped to 18.
        /// \return Rounded mid price.
        double mid_price(std::size_t digits) const noexcept {
            const double scale = dfh::utils::pow10_clamp<double>(digits);
            return std::round(mid_price() * scale) / scale;
        }

        /// \brief Returns the spread: ask - bid.
        /// \return Spread in price units.
        constexpr double spread() const noexcept {
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
        constexpr double spread_scaled(double multiplier) const noexcept {
            const double m = mid_price();
            return (m > 0.0) ? (spread() / m) * multiplier : 0.0;
        }
        
        /// \brief Returns the relative spread scaled by 10^digits using mid price as the denominator.
        ///
        /// This is a generic "digits-space" scaling of a dimensionless relative spread.
        /// Example: \p digits = 5 returns \c (ask-bid)/mid * 100000.
        ///
        /// \param digits Power-of-ten exponent used as the scale factor (10^digits). Values above 18 are clamped to 18.
        /// \return Scaled relative spread. Returns 0.0 if mid price is not positive.
        constexpr double spread_digits(std::size_t digits) const noexcept {
            return spread_scaled(dfh::utils::pow10_clamp<double>(digits));
        }

        /// \brief Returns the relative spread in basis points (bps).
        /// \return Relative spread in bps. Returns 0.0 if mid price is not positive.
        constexpr double spread_bps() const noexcept {
            return spread_scaled(10000.0);
        }

        /// \brief Checks whether the quote is numerically sane and not crossed.
        ///
        /// The quote is considered valid if:
        /// - \c ask and \c bid are finite numbers,
        /// - \c ask > 0 and \c bid > 0,
        /// - \c ask >= \c bid (not crossed).
        ///
        /// \return True if the quote is valid.
        bool is_valid() const noexcept {
            return std::isfinite(ask) && std::isfinite(bid)
                && ask > 0.0 && bid > 0.0
                && ask >= bid;
        }
    };
          
    static_assert(std::is_trivially_copyable_v<QuoteTick>,
                  "QuoteTick must remain trivially copyable for zero-copy I/O and binary serialization.");

    static_assert(sizeof(QuoteTick) == 24,
                  "QuoteTick size changed unexpectedly (ABI/layout impact).");
                  
    static_assert(alignof(QuoteTick) == alignof(double),
                  "QuoteTick alignment changed unexpectedly (ABI/layout impact).");

} // namespace dfh

#endif // _DFH_DATA_QUOTE_TICK_HPP_INCLUDED
