#pragma once
#ifndef _DFH_DATA_TICK_SEQUENCE_HPP_INCLUDED
#define _DFH_DATA_TICK_SEQUENCE_HPP_INCLUDED

/// \file TickSequence.hpp
/// \brief Defines a generic structure for a sequence of ticks with metadata

#include <vector>
#include <string>
#include "Flags.hpp"
#include "ValueTick.hpp"
#include "SimpleTick.hpp"
#include "MarketTick.hpp"

namespace dfh {

    /// \brief Generic structure for a sequence of ticks with additional metadata
    template <typename TickType>
    struct TickSequence {
        std::vector<TickType> ticks; ///< Sequence of tick data of the specified type
        uint64_t flags;              ///< Tick data flags (bitmask of UpdateFlags)
        uint16_t symbol_index;       ///< Index of the symbol
        uint16_t provider_index;     ///< Index of the data provider
        uint16_t price_digits;       ///< Number of decimal places for price
        uint16_t volume_digits;      ///< Number of decimal places for volume

        TickSequence() :
            flags(0), symbol_index(0), provider_index(0),
            price_digits(0), volume_digits(0) {
        }

        /// \brief Constructor to initialize all fields
        /// \param ts Sequence of tick data
        /// \param f Tick data flags
        /// \param si Index of the symbol
        /// \param pi Index of the data provider
        /// \param d Number of decimal places for price
        /// \param vd Number of decimal places for volume
        TickSequence(std::vector<TickType> ts, uint64_t f, uint16_t si, uint16_t pi, uint16_t d, uint16_t vd)
            : ticks(std::move(ts)), flags(f),
            symbol_index(si), provider_index(pi),
            price_digits(d), volume_digits(vd) {
        }
    };

    // Tick sequence structures for specific tick types
    using ValueTickSequence = TickSequence<ValueTick>;
    using SimpleTickSequence = TickSequence<SimpleTick>;
    using MarketTickSequence = TickSequence<MarketTick>;

} // namespace dfh

#endif // _DFH_DATA_TICK_SEQUENCE_HPP_INCLUDED
