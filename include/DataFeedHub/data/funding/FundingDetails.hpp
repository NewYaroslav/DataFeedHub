#pragma once
#ifndef _DFH_DATA_FUNDING_DETAILS_HPP_INCLUDED
#define _DFH_DATA_FUNDING_DETAILS_HPP_INCLUDED

/// \file FundingDetails.hpp
/// \brief Defines the structure for funding rate data, including mark price and timestamp.

namespace dfh {

    /// \struct FundingDetails
    /// \brief Aggregated funding data required for funding payment calculations.
    ///
    /// This structure stores all necessary funding details and a function pointer
    /// to calculate the funding payment for a given position.
    struct FundingDetails {
        double   funding_rate;   ///< Funding rate as a decimal (e.g., 0.01 for 1%).
        double   mark_price;     ///< Mark price at funding time.
        double   premium_index;  ///< Premium index for adjustments (optional).
        uint64_t time_ms;        ///< Funding event timestamp (ms since epoch).
        uint64_t prev_time_ms;   ///< Previous funding event timestamp (ms).
        uint64_t next_time_ms;   ///< Next funding event timestamp (ms).
        uint64_t period_ms;      ///< Duration of the funding period (ms).

        /// \brief Pointer to a function that calculates the funding payment.
        /// The function takes a const reference to FundingDetails and the position size,
        /// and returns the calculated payment.
        double (*calc_payment_fn)(const FundingDetails&, double position_size);

        /// \brief Default constructor. Initializes all members to zero/nullptr.
        FundingDetails()
            : funding_rate(0.0), mark_price(0.0), premium_index(0.0),
              time_ms(0), prev_time_ms(0), next_time_ms(0),
              period_ms(0), calc_payment_fn(nullptr) {}

        /// \brief Calculates the funding payment for the given position size.
        /// \param position_size Size of the position.
        /// \note The used leverage is already reflected in the position size.
        /// \return The calculated funding payment (positive if received, negative if paid).
        double calc_payment(double position_size) const {
            if (calc_payment_fn) return calc_payment_fn(*this, position_size);
            return mark_price ? position_size * mark_price * funding_rate : position_size * funding_rate;
        }
    };

} // namespace dfh

#endif // _DFH_DATA_FUNDING_DETAILS_HPP_INCLUDED
