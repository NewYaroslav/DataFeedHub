#pragma once
#ifndef _DTH_BID_ASK_RESTORE_CONFIG_HPP_INCLUDED
#define _DTH_BID_ASK_RESTORE_CONFIG_HPP_INCLUDED

/// \file BidAskRestoreConfig.hpp
/// \brief Bid/Ask price restoration configuration for historical market data

#include <cstdint>

namespace dfh {

    /// \enum BidAskModel
    /// \brief Algorithm for restoring bid/ask prices from last trade prices
    enum class BidAskModel {
        NONE = 0,            ///< No bid/ask restoration (use raw data if available)
        FIXED_SPREAD   = 1,  ///< Apply fixed spread to last price
        DYNAMIC_SPREAD = 2,  ///< Calculate spread based on price volatility
        MEDIAN_SPREAD  = 3   ///< Use median spread from historical data
    };

    /// \struct BidAskRestoreConfig
    /// \brief Parameters for reconstructing bid/ask prices in historical data
    struct BidAskRestoreConfig {
        BidAskModel mode;        ///< Restoration algorithm selection
        size_t fixed_spread;     ///< Fixed spread value in price points (when using FIXED_SPREAD mode)
        size_t price_digits;     ///< Number of decimal places for price normalization
    };

}; // namespace dfh

#endif // _DTH_BID_ASK_RESTORE_CONFIG_HPP_INCLUDED
