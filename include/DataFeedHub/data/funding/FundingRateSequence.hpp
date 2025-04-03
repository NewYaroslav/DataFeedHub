#pragma once
#ifndef _DTH_FUNDING_RATE_SEQUENCE_HPP_INCLUDED
#define _DTH_FUNDING_RATE_SEQUENCE_HPP_INCLUDED

/// \file FundingRateSequence.hpp
/// \brief

namespace dfh {

    /// \brief Represents a sequence of funding rates with symbol and provider identifiers.
    struct FundingRateSequence {
        std::vector<FundingRate> rates; ///< Массив данных фандинга.
        uint16_t symbol_index;          ///< Идентификатор символа.
        uint16_t provider_index;        ///< Идентификатор поставщика.

        FundingRateSequence()
            : rates(), symbol_index(0), provider_index(0) {}

        FundingRateSequence(const std::vector<FundingRate>& r, uint16_t symbol, uint16_t provider)
            : rates(r), symbol_index(symbol), provider_index(provider) {}
    };

} // namespace dfh

#endif // _DH_FUNDING_RATE_SEQUENCE_HPP_INCLUDED
