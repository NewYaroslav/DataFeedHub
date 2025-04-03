#pragma once
#ifndef _DFH_DATA_TICK_ENUMS_HPP_INCLUDED
#define _DFH_DATA_TICK_ENUMS_HPP_INCLUDED

/// \file enums.hpp
/// \brief

namespace dfh {

    /// \enum BidAskModel
    /// \brief Algorithm for restoring bid/ask prices from last trade prices
    enum class BidAskModel : uint16_t {
        NONE = 0,            ///< No bid/ask restoration (use raw data if available)
        FIXED_SPREAD   = 1,  ///< Apply fixed spread to last price
        DYNAMIC_SPREAD = 2,  ///< Calculate spread based on price volatility
        MEDIAN_SPREAD  = 3   ///< Use median spread from historical data
    };

};

#endif // _DFH_DATA_TICK_ENUMS_HPP_INCLUDED
