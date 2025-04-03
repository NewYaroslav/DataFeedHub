#pragma once
#ifndef _DFH_DATA_FUNDING_RATE_HPP_INCLUDED
#define _DFH_DATA_FUNDING_RATE_HPP_INCLUDED

/// \file FundingRate.hpp
/// \brief Defines the structure for funding rate data, including mark price and timestamp.

namespace dfh {

    /// \struct FundingRate
    /// \brief Represents a funding rate entry with associated timestamp and mark price.
    struct FundingRate {
        double   rate;         ///< Funding rate (as a percentage, e.g., 0.01 = 1%).
        double   mark_price;   ///< Mark price of the asset at the funding time.
        uint64_t time_ms;      ///< Timestamp of the funding event (milliseconds since Unix epoch).
        uint64_t period_ms;    ///< Duration of the funding period (in milliseconds).

        /// \brief Default constructor. Initializes all fields to zero.
        FundingRate()
            : rate(0.0), mark_price(0.0), time_ms(0), period_ms(0) {}

        /// \brief Constructs a funding rate entry with all fields explicitly set.
        /// \param rate Funding rate as a percentage (e.g., 0.01 = 1%).
        /// \param time Funding timestamp in milliseconds since Unix epoch.
        /// \param mark_price Mark price of the asset at the funding time.
        /// \param period Duration of the funding interval in milliseconds.
        FundingRate(double rate, uint64_t time, double mark_price, uint64_t period)
            : rate(rate), mark_price(mark_price), time_ms(time), period_ms(period) {}
    };

} // namespace dfh

#endif // _DFH_DATA_FUNDING_RATE_HPP_INCLUDED
