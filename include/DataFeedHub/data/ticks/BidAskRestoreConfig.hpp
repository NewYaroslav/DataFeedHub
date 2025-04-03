#pragma once
#ifndef _DFH_DATA_BID_ASK_RESTORE_CONFIG_HPP_INCLUDED
#define _DFH_DATA_BID_ASK_RESTORE_CONFIG_HPP_INCLUDED

/// \file BidAskRestoreConfig.hpp
/// \brief Bid/Ask price restoration configuration for historical market data

namespace dfh {

    /// \struct BidAskRestoreConfig
    /// \brief Parameters for reconstructing bid/ask prices in historical data
    struct BidAskRestoreConfig {
        BidAskModel mode;        ///< Restoration algorithm selection
        uint16_t fixed_spread;   ///< Fixed spread value in price points (when using FIXED_SPREAD mode)
        uint16_t price_digits;   ///< Number of decimal places for price normalization
    };

}; // namespace dfh

#endif // _DFH_DATA_BID_ASK_RESTORE_CONFIG_HPP_INCLUDED
