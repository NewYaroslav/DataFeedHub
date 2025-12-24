#pragma once
#ifndef _DFH_DATA_BID_ASK_RESTORE_CONFIG_HPP_INCLUDED
#define _DFH_DATA_BID_ASK_RESTORE_CONFIG_HPP_INCLUDED

/// \file BidAskRestoreConfig.hpp
/// \brief Bid/ask price restoration configuration for historical market data.

#include <cstdint>
#include <type_traits>

#include "DataFeedHub/data/ticks/enums.hpp"

namespace dfh {

    /// \struct BidAskRestoreConfig
    /// \brief Parameters used to reconstruct bid/ask series when only last prices are stored.
    struct BidAskRestoreConfig {
        BidAskModel mode{BidAskModel::NONE}; ///< Restoration algorithm selection.
        std::uint16_t fixed_spread{0};       ///< Fixed spread in price points for FIXED_SPREAD mode.
        std::uint16_t price_digits{0};       ///< Decimal precision for normalized prices.
    };

    static_assert(std::is_trivially_copyable_v<BidAskRestoreConfig>,
                  "BidAskRestoreConfig must remain a POD for shared-memory use.");

} // namespace dfh

#endif // _DFH_DATA_BID_ASK_RESTORE_CONFIG_HPP_INCLUDED
