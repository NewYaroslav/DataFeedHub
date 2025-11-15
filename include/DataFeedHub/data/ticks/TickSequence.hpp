#pragma once
#ifndef _DFH_DATA_TICK_SEQUENCE_HPP_INCLUDED
#define _DFH_DATA_TICK_SEQUENCE_HPP_INCLUDED

/// \file TickSequence.hpp
/// \brief Defines a generic structure for a sequence of ticks with metadata.

namespace dfh {

    /// \brief Generic structure for a sequence of ticks with additional metadata.
    template <typename TickType>
    struct TickSequence {
        std::vector<TickType> ticks{}; ///< Sequence of tick data of the specified type.
        TickUpdateFlags flags{TickUpdateFlags::NONE}; ///< Tick data flags bitmask.
        std::uint16_t symbol_index{0};       ///< Index of the symbol.
        std::uint16_t provider_index{0};     ///< Index of the data provider.
        std::uint16_t price_digits{0};       ///< Number of decimal places for price.
        std::uint16_t volume_digits{0};      ///< Number of decimal places for volume.

        /// \brief Default constructor.
        TickSequence() = default;

        /// \brief Constructor to initialize all fields.
        /// \param ts Sequence of tick data.
        /// \param f Tick data flags.
        /// \param si Index of the symbol.
        /// \param pi Index of the data provider.
        /// \param d Number of decimal places for price.
        /// \param vd Number of decimal places for volume.
        TickSequence(std::vector<TickType> ts,
                     TickUpdateFlags f,
                     std::uint16_t si,
                     std::uint16_t pi,
                     std::uint16_t d,
                     std::uint16_t vd)
            : ticks(std::move(ts))
            , flags(f)
            , symbol_index(si)
            , provider_index(pi)
            , price_digits(d)
            , volume_digits(vd) {}
    };

    // Tick sequence structures for specific tick types.
    using ValueTickSequence = TickSequence<ValueTick>;
    using QuoteTickSequence = TickSequence<QuoteTick>;
    using MarketTickSequence = TickSequence<MarketTick>;

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Serializes TickSequence to JSON.
    template <typename TickType>
    inline void to_json(nlohmann::json& j, const TickSequence<TickType>& value) {
        j = nlohmann::json{
            {"ticks", value.ticks},
            {"flags", static_cast<std::uint64_t>(value.flags)},
            {"symbol_index", value.symbol_index},
            {"provider_index", value.provider_index},
            {"price_digits", value.price_digits},
            {"volume_digits", value.volume_digits}
        };
    }

    /// \brief Deserializes TickSequence from JSON.
    template <typename TickType>
    inline void from_json(const nlohmann::json& j, TickSequence<TickType>& value) {
        j.at("ticks").get_to(value.ticks);
        std::uint64_t raw_flags = 0;
        j.at("flags").get_to(raw_flags);
        value.flags = static_cast<TickUpdateFlags>(raw_flags);
        j.at("symbol_index").get_to(value.symbol_index);
        j.at("provider_index").get_to(value.provider_index);
        j.at("price_digits").get_to(value.price_digits);
        j.at("volume_digits").get_to(value.volume_digits);
    }

#endif // DFH_USE_JSON && DFH_USE_NLOHMANN_JSON

} // namespace dfh

#endif // _DFH_DATA_TICK_SEQUENCE_HPP_INCLUDED
