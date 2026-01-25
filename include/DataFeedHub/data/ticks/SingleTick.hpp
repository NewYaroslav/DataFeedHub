#pragma once
#ifndef _DFH_DATA_SINGLE_TICK_HPP_INCLUDED
#define _DFH_DATA_SINGLE_TICK_HPP_INCLUDED

/// \file SingleTick.hpp
/// \brief Defines a generic structure for a single tick payload enriched with common metadata.

#include "flags.hpp"
#include "ValueTick.hpp"
#include "QuoteTick.hpp"
#include "QuoteTickVol.hpp"
#include "QuoteTickL1.hpp"
#include "MarketTick.hpp"

#include <utility> // std::move

namespace dfh {

    /// \brief A single tick payload enriched with common metadata.
    ///
    /// This wrapper is useful when you want to pass around one tick together with
    /// symbol/provider identifiers and precision information.
    template <typename TickType>
    struct SingleTick {
        TickType tick{}; ///< Tick payload of type \p TickType.
        TickStatusFlags status_flags{TickStatusFlags::NONE}; ///< Status flags (e.g. REALTIME, INITIALIZED).
        std::uint16_t symbol_index{0};   ///< Symbol index in your symbol table.
        std::uint16_t provider_index{0}; ///< Data provider index.
        std::uint8_t price_digits{0};    ///< Number of decimal places for price fields.
        std::uint8_t volume_digits{0};   ///< Number of decimal places for volume fields.

        /// \brief Default constructor. Initializes all fields with default values.
        constexpr SingleTick() noexcept = default;

        /// \brief Constructs a SingleTick and initializes all fields.
        /// \param t Tick payload.
        /// \param sf Status flags for this tick (not per-field update flags).
        /// \param si Symbol index.
        /// \param pi Provider index.
        /// \param pd Number of decimal places for price.
        /// \param vd Number of decimal places for volume.
        constexpr SingleTick(
                TickType t,
                TickStatusFlags sf,
                std::uint16_t si,
                std::uint16_t pi,
                std::uint8_t pd,
                std::uint8_t vd
            ) noexcept : 
            tick(std::move(t)),
            status_flags(sf), 
            symbol_index(si), 
            provider_index(pi), 
            price_digits(pd), 
            volume_digits(vd) {}
    };

    // Convenience aliases for common tick types.
    using SingleValueTick    = SingleTick<ValueTick>;
    using SingleQuoteTick    = SingleTick<QuoteTick>;
    using SingleQuoteTickVol = SingleTick<QuoteTickVol>;
    using SingleQuoteTickL1  = SingleTick<QuoteTickL1>;
    using SingleMarketTick   = SingleTick<MarketTick>;

} // namespace dfh

#endif // _DFH_DATA_SINGLE_TICK_HPP_INCLUDED
