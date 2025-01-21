#pragma once
#ifndef _DTH_SINGLE_TICK_HPP_INCLUDED
#define _DTH_SINGLE_TICK_HPP_INCLUDED

/// \file SingleTick.hpp
/// \brief Defines a generic structure for a single tick with metadata

#include <cstdint>
#include <string>
#include "Flags.hpp"
#include "ValueTick.hpp"
#include "SimpleTick.hpp"
#include "MarketTick.hpp"

namespace dfh {

    /// \brief Generic structure for a single tick with additional metadata
    template <typename TickType>
    struct SingleTick {
        TickType tick;          ///< Tick data of the specified type
        uint64_t flags;         ///< Tick data flags (bitmask of UpdateFlags)
        uint16_t symbol_index;  ///< Index of the symbol
        uint16_t provider_index;///< Index of the data provider
        uint16_t price_digits;  ///< Number of decimal places for price
        uint16_t volume_digits; ///< Number of decimal places for volume

        /// \brief Constructor to initialize all fields
        /// \param t Tick data
        /// \param f Tick data flags
        /// \param si Index of the symbol
        /// \param pi Index of the data provider
        /// \param d Number of decimal places for price
        /// \param vd Number of decimal places for volume
        SingleTick(TickType t, uint64_t f, uint16_t si, uint16_t pi, uint16_t d, uint16_t vd)
            : tick(std::move(t)), flags(f), symbol_index(si), provider_index(pi), price_digits(d), volume_digits(vd) {}
    };

	// Single tick structures for specific tick types
    using SingleValueTick = SingleTick<ValueTick>;
    using SingleSimpleTick = SingleTick<SimpleTick>;
    using SingleMarketTick = SingleTick<MarketTick>;

} // namespace dfh

#endif // _DTH_SINGLE_TICK_HPP_INCLUDED
