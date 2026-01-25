#pragma once
#ifndef _DFH_DATA_TICK_SEQUENCE_HPP_INCLUDED
#define _DFH_DATA_TICK_SEQUENCE_HPP_INCLUDED

/// \file TickSequence.hpp
/// \brief Defines a templated sequence of ticks and metadata for decoding/processing.

#include "flags.hpp"
#include "ValueTick.hpp"
#include "QuoteTick.hpp"
#include "QuoteTickVol.hpp"
#include "QuoteTickL1.hpp"
#include "MarketTick.hpp"
#include <vector>

namespace dfh {

    /// \struct TickSequence
    /// \brief A sequence of ticks with metadata shared by the whole batch.
    ///
    /// \tparam TickType The type of tick DTO stored in \c ticks.
    template <typename TickType>
    struct TickSequence {
        std::vector<TickType> ticks{};                 ///< Tick samples (typically chronological).
        TickStatusFlags status_flags{TickStatusFlags::NONE}; ///< Flags indicating the status of the tick sequence.
        std::uint16_t symbol_index{0};                 ///< Symbol index (e.g., in a symbol table).
        std::uint16_t provider_index{0};               ///< Data provider index (exchange/feed id).
        std::uint16_t price_digits{0};                 ///< Price scale exponent (10^price_digits).
        std::uint16_t volume_digits{0};                ///< Volume scale exponent (10^volume_digits).

        /// \brief Default constructor.
        TickSequence() = default;

        /// \brief Constructs a sequence and initializes all metadata.
        /// \param ts Tick vector (moved into \c ticks).
        /// \param status_flags Flags indicating the status of the tick sequence.
        /// \param si Symbol index.
        /// \param pi Provider index.
        /// \param pd Price scale exponent (10^pd).
        /// \param vd Volume scale exponent (10^vd).
        TickSequence(
				std::vector<TickType> ts,
				TickStatusFlags status_flags,
				std::uint16_t si,
				std::uint16_t pi,
				std::uint16_t pd,
				std::uint16_t vd
			) noexcept :
			ticks(std::move(ts)),
			status_flags(status_flags),
			symbol_index(si),
			provider_index(pi),
			price_digits(pd),
			volume_digits(vd) {}
    };

    // Aliases for common tick types.
    using ValueTickSequence    = TickSequence<ValueTick>;     ///< Batch of ValueTick samples.
    using QuoteTickSequence    = TickSequence<QuoteTick>;     ///< Batch of QuoteTick samples.
    using QuoteTickVolSequence = TickSequence<QuoteTickVol>;  ///< Batch of QuoteTickVol samples.
    using QuoteTickL1Sequence  = TickSequence<QuoteTickL1>;   ///< Batch of QuoteTickL1 samples.
    using MarketTickSequence   = TickSequence<MarketTick>;    ///< Batch of MarketTick samples.

} // namespace dfh

#endif // _DFH_DATA_TICK_SEQUENCE_HPP_INCLUDED