#pragma once
#ifndef _DFH_DATA_SINGLE_TICK_HPP_INCLUDED
#define _DFH_DATA_SINGLE_TICK_HPP_INCLUDED

/// \file SingleTick.hpp
/// \brief Defines a generic structure for a single tick with metadata.

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

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Serializes a SingleTick wrapper to JSON.
    template <typename TickType>
    inline void to_json(nlohmann::json& j, const SingleTick<TickType>& value) {
        j = nlohmann::json{
            {"tick", value.tick},
            {"flags", static_cast<std::uint64_t>(value.flags)},
            {"symbol_index", value.symbol_index},
            {"provider_index", value.provider_index},
            {"price_digits", value.price_digits},
            {"volume_digits", value.volume_digits}
        };
    }

    /// \brief Deserializes a SingleTick wrapper from JSON.
    template <typename TickType>
    inline void from_json(const nlohmann::json& j, SingleTick<TickType>& value) {
        j.at("tick").get_to(value.tick);
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

#endif // _DFH_DATA_SINGLE_TICK_HPP_INCLUDED
