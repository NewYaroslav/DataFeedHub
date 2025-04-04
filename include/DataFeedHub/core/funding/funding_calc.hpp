#pragma once
#ifndef _DFH_CORE_FUNDING_CALCULATOR_HPP_INCLUDED
#define _DFH_CORE_FUNDING_CALCULATOR_HPP_INCLUDED

/// \file funding_calc.hpp
/// \brief Provides funding payment calculation functions for different exchanges.
///
/// This file contains default funding payment calculation functions for Binance and Bybit.
/// If premium_index is non-zero, it is incorporated into the calculation as follows:
/// - For Binance: effective_rate = funding_rate * (1 + premium_index)
/// - For Bybit:   effective_rate = funding_rate + premium_index
/// The calculated funding payment is based on the position's notional value (position_size * mark_price)
/// and is adjusted proportionally to the actual funding period relative to a standard 8-hour period.

namespace dfh {

    /// \brief Default calculation for Binance funding payment.
    /// \param fd FundingDetails instance containing all funding-related data.
    /// \param position_size Size of the position.
    /// \return Calculated funding payment.
    inline double binance_calc_payment(const FundingDetails &fd, double position_size) {
        constexpr uint64_t standard_period_ms = 8 * 3600 * 1000; // 8 hours in milliseconds.
        // Если premium_index не равен 0, корректируем funding_rate.
        double effective_rate = fd.premium_index != 0.0 ? fd.funding_rate * (1.0 + fd.premium_index) : fd.funding_rate;
        return (position_size * fd.mark_price) * effective_rate *
               (static_cast<double>(fd.period_ms) / static_cast<double>(standard_period_ms));
    }

    /// \brief Default calculation for Bybit funding payment.
    /// \param fd FundingDetails instance containing all funding-related data.
    /// \param position_size Size of the position.
    /// \return Calculated funding payment.
    inline double bybit_calc_payment(const FundingDetails &fd, double position_size) {
        constexpr uint64_t standard_period_ms = 8 * 3600 * 1000; // 8 hours in milliseconds.
        // Для Bybit, если premium_index не равен 0, добавляем его к funding_rate.
        double effective_rate = fd.premium_index != 0.0 ? fd.funding_rate + fd.premium_index : fd.funding_rate;
        return (position_size * fd.mark_price) * effective_rate *
               (static_cast<double>(fd.period_ms) / static_cast<double>(standard_period_ms));
    }

    /// \brief Initializes the calc_payment function pointer in FundingDetails.
    ///
    /// \param fd The FundingDetails instance to initialize.
    /// \param type The funding calculation type (BINANCE, BYBIT, or CUSTOM).
    /// \param custom_calc Optional pointer to a user-defined calculation function;
    ///                    used only if type is CUSTOM.
    inline void init_funding_calc(FundingDetails &fd, FundingCalcType type,
                                  double (*custom_calc)(const FundingDetails&, double) = nullptr) {
        switch (type) {
            case FundingCalcType::BINANCE:
                fd.calc_payment = binance_calc_payment;
                break;
            case FundingCalcType::BYBIT:
                fd.calc_payment = bybit_calc_payment;
                break;
            case FundingCalcType::CUSTOM:
                fd.calc_payment = custom_calc;
                break;
            default:
                fd.calc_payment = nullptr;
                break;
        }
    }

} // namespace dfh

#endif // _DFH_CORE_FUNDING_CALCULATOR_HPP_INCLUDED
