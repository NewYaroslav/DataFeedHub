#pragma once
#ifndef _DFH_DATA_FUNDING_ENUMS_HPP_INCLUDED
#define _DFH_DATA_FUNDING_ENUMS_HPP_INCLUDED

/// \file enums.hpp
/// \brief

namespace dfh {

    /// \enum FundingCalcType
    /// \brief Specifies the funding payment calculation type.
    enum class FundingCalcType {
        BINANCE,  ///< Use Binance default calculation.
        BYBIT,    ///< Use Bybit default calculation.
        CUSTOM    ///< Use a custom user-defined calculation.
    };

};

#endif // _DFH_DATA_FUNDING_ENUMS_HPP_INCLUDED
