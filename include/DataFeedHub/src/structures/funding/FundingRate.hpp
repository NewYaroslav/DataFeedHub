#pragma once
#ifndef _DTH_FUNDING_RATE_HPP_INCLUDED
#define _DTH_FUNDING_RATE_HPP_INCLUDED

/// \file FundingRate.hpp
/// \brief

namespace dfh {

    /// \brief Represents funding rate data for a specific timestamp.
    struct FundingRate {
        double rate;    ///< Фандинг ставка (в процентах).
        uint64_t time_ms;       ///< Время фандинга в миллисекундах Unix.

        /// \brief Default constructor for FundingRateData.
        FundingRate()
            : rate(0.0), time_ms(0) {}

        /// \brief Constructor for initializing funding rate data.
        /// \param rate Фандинг ставка.
        /// \param time Время в миллисекундах Unix.
        FundingRate(double rate, uint64_t time)
            : rate(rate), time_ms(time) {}
    };

} // namespace dfh

#endif // _DTH_FUNDING_RATE_HPP_INCLUDED
