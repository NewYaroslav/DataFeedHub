#pragma once
#ifndef _DFH_DATA_SINGLE_TICK_HPP_INCLUDED
#define _DFH_DATA_SINGLE_TICK_HPP_INCLUDED

/// \file SingleTick.hpp
/// \brief Defines a generic structure for a single tick with metadata.

#include <cstdint>
#include <utility>

#include "DataFeedHub/data/ticks/flags.hpp"
#include "DataFeedHub/data/ticks/MarketTick.hpp"
#include "DataFeedHub/data/ticks/QuoteTick.hpp"
#include "DataFeedHub/data/ticks/ValueTick.hpp"

namespace dfh {

    /// \brief Generic structure for a single tick enriched with metadata.
    template <typename TickType>
    struct SingleTick {
        TickType tick{};                       ///< Tick payload of the specified type.
        TickUpdateFlags flags{TickUpdateFlags::NONE}; ///< Update flags bitmask.
        std::uint16_t symbol_index{0};         ///< Index of the symbol.
        std::uint16_t provider_index{0};       ///< Index of the data provider.
        std::uint16_t price_digits{0};         ///< Number of decimal places for price fields.
        std::uint16_t volume_digits{0};        ///< Number of decimal places for volume fields.

        /// \brief Default constructor.
        constexpr SingleTick() noexcept = default;

        /// \brief Constructor to initialize all fields.
        /// \param t Tick data payload.
        /// \param f Tick update flags.
        /// \param si Index of the symbol.
        /// \param pi Index of the data provider.
        /// \param d Number of decimal places for price.
        /// \param vd Number of decimal places for volume.
        constexpr SingleTick(TickType t,
                             TickUpdateFlags f,
                             std::uint16_t si,
                             std::uint16_t pi,
                             std::uint16_t d,
                             std::uint16_t vd) noexcept
            : tick(std::move(t))
            , flags(f)
            , symbol_index(si)
            , provider_index(pi)
            , price_digits(d)
            , volume_digits(vd) {}
    };

    // Single tick structures for specific tick types.
    using SingleValueTick = SingleTick<ValueTick>;
    using SingleQuoteTick = SingleTick<QuoteTick>;
    using SingleMarketTick = SingleTick<MarketTick>;

} // namespace dfh

#endif // _DFH_DATA_SINGLE_TICK_HPP_INCLUDED
