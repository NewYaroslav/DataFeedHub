#pragma once
#ifndef _DTH_FUNDING_RATE_INFO_HPP_INCLUDED
#define _DTH_FUNDING_RATE_INFO_HPP_INCLUDED

/// \file FundingRateInfo.hpp
/// \brief

namespace dfh {

    /// \brief Represents funding rate data with symbol and provider information.
    struct FundingRateInfo {
        FundingRate rate_data;      ///< Данные фандинга (rate и time_ms).
        uint16_t symbol_index;      ///< Идентификатор символа.
        uint16_t provider_index;    ///< Идентификатор поставщика.

        /// \brief Default constructor for FundingRateInfo.
        FundingRateInfo()
            : rate_data(), symbol_index(0), provider_index(0) {}

        /// \brief Constructor for initializing funding rate info.
        /// \param data Данные фандинга.
        /// \param symbol Идентификатор символа.
        /// \param provider Идентификатор поставщика.
        FundingRateInfo(const FundingRate& data, uint16_t symbol, uint16_t provider)
            : rate_data(data), symbol_index(symbol), provider_index(provider) {}
    };

} // namespace dfh

#endif // _DTH_FUNDING_RATE_INFO_HPP_INCLUDED
