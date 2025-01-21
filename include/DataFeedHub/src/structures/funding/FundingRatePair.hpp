#pragma once
#ifndef _DTH_FUNDING_RATE_PAIR_HPP_INCLUDED
#define _DTH_FUNDING_RATE_PAIR_HPP_INCLUDED

/// \file FundingRatePair.hpp
/// \brief

namespace dfh {

    /// \brief Represents a pair of funding rate info and their difference.
    struct FundingRatePair {
        FundingRateInfo buy_info;  ///< Данные фандинга для позиции Buy.
        FundingRateInfo sell_info; ///< Данные фандинга для позиции Sell.
        double rate_difference;    ///< Разница ставок фандинга (buy - sell).

        /// \brief Default constructor for FundingRatePair.
        FundingRatePair()
            : buy_info(), sell_info(), rate_difference(0.0) {}

        /// \brief Constructor to initialize a funding rate pair.
        /// \param buy Данные фандинга для позиции Buy.
        /// \param sell Данные фандинга для позиции Sell.
        FundingRatePair(const FundingRateInfo& buy, const FundingRateInfo& sell)
            : buy_info(buy), sell_info(sell),
              rate_difference(buy.rate_data.rate - sell.rate_data.rate) {}
    };

} // namespace dfh

#endif // _DTH_FUNDING_RATE_PAIR_HPP_INCLUDED
