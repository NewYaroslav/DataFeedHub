#pragma once
#ifndef _DTH_FUNDING_RATE_HPP_INCLUDED
#define _DTH_FUNDING_RATE_HPP_INCLUDED

/// \file FundingRate.hpp
/// \brief Defines the structure for funding rate data including mark price and timestamp.

namespace dfh {

    /// \brief Represents funding rate data for a specific timestamp.
    struct FundingRate {
        double rate;        ///< Funding rate (as a percentage).
        double mark_price;  ///< Mark price of the asset at the funding timestamp.
        uint64_t time_ms;   ///< Funding timestamp in milliseconds since Unix epoch.
        uint64_t period_ms; ///< Duration of the funding period in milliseconds.

        /// \brief Default constructor for FundingRate.
        FundingRate()
            : rate(0.0), time_ms(0), mark_price(0.0), period_ms(0) {}

        /// \brief Constructor for initializing funding rate data.
        /// \param rate Funding rate (percentage).
        /// \param time Funding timestamp in milliseconds since Unix epoch.
        /// \param mark_price Mark price of the asset.
        /// \param period Duration of the funding period in milliseconds.
        FundingRate(double rate, uint64_t time, double mark_price, uint64_t period)
            : rate(rate), time_ms(time), mark_price(mark_price), period_ms(period) {}
    };

} // namespace dfh

#endif // _DTH_FUNDING_RATE_HPP_INCLUDED
