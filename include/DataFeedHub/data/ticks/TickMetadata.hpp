#pragma once
#ifndef _DFH_DATA_TICK_METADATA_HPP_INCLUDED
#define _DFH_DATA_TICK_METADATA_HPP_INCLUDED

/// \file TickMetadata.hpp
/// \brief Defines the metadata structure for tick data.

namespace dfh {

    /// \struct TickMetadata
    /// \brief Tick data metadata for a trading symbol and provider.
    struct TickMetadata {
        std::uint64_t start_time_ms{0};       ///< Start timestamp of tick series in milliseconds.
        std::uint64_t end_time_ms{0};         ///< End timestamp of tick series in milliseconds.
        std::uint64_t expiration_time_ms{0};  ///< Expiration time for futures (0 for perpetual or spot).
        std::uint64_t next_expiration_time_ms{0}; ///< Expiration time of the next contract (0 if not defined).
        std::uint64_t count{0};               ///< Number of ticks in the dataset.
        double tick_size{0.0};                ///< Minimum price increment (tick size).
        std::uint16_t symbol_id{0};           ///< Symbol identifier.
        std::uint16_t exchange_id{0};         ///< Exchange identifier.
        MarketType market_type{MarketType::UNKNOWN}; ///< Market type (spot, futures, etc.).
        std::uint8_t price_digits{0};         ///< Number of decimal places for price.
        std::uint8_t volume_digits{0};        ///< Number of decimal places for volume.
        TickStorageFlags flags{TickStorageFlags::NONE}; ///< Tick metadata flags.

        /// \brief Default constructor.
        constexpr TickMetadata() noexcept = default;

        /// \brief Constructs TickMetadata with all fields explicitly set.
        /// \param start Start time of the tick data in milliseconds.
        /// \param end End time of the tick data in milliseconds.
        /// \param total Number of ticks in the dataset.
        /// \param mkt_type Type of trading market (spot, futures, etc.).
        /// \param exch_id Exchange identifier.
        /// \param sym_id Symbol identifier.
        /// \param price_digits_ Number of decimal places for price.
        /// \param volume_digits_ Number of decimal places for volume.
        /// \param mask Bitmask of TickStorageFlags.
        /// \param tick_size_ Minimum price increment (tick size).
        /// \param expiration Expiration time for this contract (0 for perpetual or spot).
        /// \param next_expiration Expiration time of the next contract (0 if unknown).
        constexpr TickMetadata(std::uint64_t start,
                               std::uint64_t end,
                               std::uint64_t total,
                               MarketType mkt_type,
                               std::uint16_t exch_id,
                               std::uint16_t sym_id,
                               std::uint8_t price_digits_,
                               std::uint8_t volume_digits_,
                               TickStorageFlags mask,
                               double tick_size_ = 0.0,
                               std::uint64_t expiration = 0,
                               std::uint64_t next_expiration = 0) noexcept
            : start_time_ms(start)
            , end_time_ms(end)
            , expiration_time_ms(expiration)
            , next_expiration_time_ms(next_expiration)
            , count(total)
            , tick_size(tick_size_)
            , symbol_id(sym_id)
            , exchange_id(exch_id)
            , market_type(mkt_type)
            , price_digits(price_digits_)
            , volume_digits(volume_digits_)
            , flags(mask) {}

        /// \brief Sets a flag in the metadata.
        /// \param flag Flag to enable.
        constexpr void set_flag(TickStorageFlags flag) noexcept { flags |= flag; }

        /// \brief Sets or clears a flag in the configuration.
        /// \param flag Flag to modify.
        /// \param value If true, the flag is set; if false, the flag is cleared.
        constexpr void set_flag(TickStorageFlags flag, bool value) noexcept {
            if (value) flags |= flag;
            else flags &= ~flag;
        }

        /// \brief Clears a flag in the configuration.
        /// \param flag Flag to clear.
        constexpr void clear_flag(TickStorageFlags flag) noexcept { flags &= ~flag; }

        /// \brief Checks if a specific flag is set.
        /// \param flag Flag to check.
        /// \return True if the flag is enabled.
        [[nodiscard]] constexpr bool has_flag(TickStorageFlags flag) const noexcept {
            return (flags & flag) != TickStorageFlags::NONE;
        }
    };

    static_assert(std::is_trivially_copyable_v<TickMetadata>,
                  "TickMetadata must remain trivially copyable for storage serialization.");

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Serializes TickMetadata to JSON.
    inline void to_json(nlohmann::json& j, const TickMetadata& metadata) {
        j = nlohmann::json{
            {"start_time_ms", metadata.start_time_ms},
            {"end_time_ms", metadata.end_time_ms},
            {"expiration_time_ms", metadata.expiration_time_ms},
            {"next_expiration_time_ms", metadata.next_expiration_time_ms},
            {"count", metadata.count},
            {"tick_size", metadata.tick_size},
            {"symbol_id", metadata.symbol_id},
            {"exchange_id", metadata.exchange_id},
            {"market_type", static_cast<std::uint32_t>(metadata.market_type)},
            {"price_digits", metadata.price_digits},
            {"volume_digits", metadata.volume_digits},
            {"flags", static_cast<std::uint64_t>(metadata.flags)}
        };
    }

    /// \brief Deserializes TickMetadata from JSON.
    inline void from_json(const nlohmann::json& j, TickMetadata& metadata) {
        j.at("start_time_ms").get_to(metadata.start_time_ms);
        j.at("end_time_ms").get_to(metadata.end_time_ms);
        j.at("expiration_time_ms").get_to(metadata.expiration_time_ms);
        j.at("next_expiration_time_ms").get_to(metadata.next_expiration_time_ms);
        j.at("count").get_to(metadata.count);
        j.at("tick_size").get_to(metadata.tick_size);
        j.at("symbol_id").get_to(metadata.symbol_id);
        j.at("exchange_id").get_to(metadata.exchange_id);
        std::uint32_t raw_market_type = 0;
        j.at("market_type").get_to(raw_market_type);
        metadata.market_type = static_cast<MarketType>(raw_market_type);
        j.at("price_digits").get_to(metadata.price_digits);
        j.at("volume_digits").get_to(metadata.volume_digits);
        std::uint64_t raw_flags = 0;
        j.at("flags").get_to(raw_flags);
        metadata.flags = static_cast<TickStorageFlags>(raw_flags);
    }

#endif // DFH_USE_JSON && DFH_USE_NLOHMANN_JSON

} // namespace dfh

#endif // _DFH_DATA_TICK_METADATA_HPP_INCLUDED
