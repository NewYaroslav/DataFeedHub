#pragma once
#ifndef _DFH_DATA_QUOTE_TICK_L1_HPP_INCLUDED
#define _DFH_DATA_QUOTE_TICK_L1_HPP_INCLUDED

/// \file QuoteTickL1.hpp
/// \brief Defines the QuoteTickL1 structure for L1 quote ticks with bid/ask volumes.

#include <DataFeedHub/utils/math_utils.hpp> // dfh::utils::pow10_clamp
#include <cmath>                            // std::isfinite, std::round
#include <type_traits>

namespace dfh {

    /// \brief L1 quote tick with bid/ask prices and volumes
    struct QuoteTickL1 {
        std::int64_t time_ms{0};    ///< Tick timestamp in milliseconds.
        double ask{0.0};            ///< Best ask price.
        double bid{0.0};            ///< Best bid price.
        double ask_volume{0.0};     ///< Volume at best ask.
        double bid_volume{0.0};     ///< Volume at best bid.

        /// \brief Default constructor. Initializes all fields to zero.
        constexpr QuoteTickL1() noexcept = default;

        /// \brief Constructor to initialize all fields.
        /// \param ts Tick timestamp in milliseconds
        /// \param a Best ask price
        /// \param b Best bid price
        /// \param av Volume at best ask
        /// \param bv Volume at best bid
        constexpr QuoteTickL1(
                std::int64_t ts,
                double a,
                double b,
                double av,
                double bv) noexcept : 
            time_ms(ts), 
            ask(a), 
            bid(b), 
            ask_volume(av), 
            bid_volume(bv) {}
            
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

        /// \brief Checks whether the quote fields are finite, positive, and not crossed.
        ///
        /// The quote is considered valid if:
        /// - \c ask, \c bid, \c ask_volume, \c bid_volume are finite numbers,
        /// - \c ask > 0 and \c bid > 0,
        /// - \c ask_volume >= 0 and \c bid_volume >= 0,
        /// - \c ask >= \c bid (not crossed).
        ///
        /// \return True if the quote is valid.
        bool is_valid() const noexcept {
            return std::isfinite(ask) && std::isfinite(bid)
                && std::isfinite(ask_volume) && std::isfinite(bid_volume)
                && ask > 0.0 && bid > 0.0
                && ask_volume >= 0.0 && bid_volume >= 0.0
                && ask >= bid;
        }
    };

    static_assert(std::is_trivially_copyable_v<QuoteTickL1>,
                  "QuoteTickL1 must remain trivially copyable for zero-copy I/O and binary serialization.");

    static_assert(sizeof(QuoteTickL1) == 40,
                  "QuoteTickL1 size changed unexpectedly (ABI/layout impact).");
                  
    static_assert(alignof(QuoteTickL1) == alignof(double),
                  "QuoteTickL1 alignment changed unexpectedly (ABI/layout impact).");

} // namespace dfh

#endif // _DFH_DATA_QUOTE_TICK_L1_HPP_INCLUDED
