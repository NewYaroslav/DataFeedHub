#pragma once
#ifndef _DTH_DATA_TICK_ENCODING_CONFIG_HPP_INCLUDED
#define _DTH_DATA_TICK_ENCODING_CONFIG_HPP_INCLUDED

/// \file TickCodecConfig.hpp
/// \brief Defines configuration for encoding and decoding tick sequences.

namespace dfh {

    /// \struct TickCodecConfig
    /// \brief Parameters for tick data compression, serialization, and storage.
    ///
    /// The `flags` field controls optional features such as trade-based encoding, tick flags,
    /// received timestamp storage, and binary format selection.
    struct TickCodecConfig {
        double tick_size{0.0};                  ///< Minimum price increment (tick size).
        std::uint64_t expiration_time_ms{0};    ///< Expiration time for futures (0 for perpetual or spot).
        std::uint64_t next_expiration_time_ms{0}; ///< Expiration time of the next contract (0 if not defined).
        TickStorageFlags flags{TickStorageFlags::NONE}; ///< Encoding flags.
        std::uint8_t price_digits{0};           ///< Number of decimal places for prices.
        std::uint8_t volume_digits{0};          ///< Number of decimal places for volumes.
        std::array<std::uint8_t, 6> reserved{}; ///< Reserved for future use; keeps structure 32 bytes long.

        /// \brief Default constructor for TickCodecConfig.
        constexpr TickCodecConfig() noexcept = default;

        /// \brief Constructor to initialize all fields explicitly.
        /// \param mask Bitmask of `TickStorageFlags`.
        /// \param price_digits_ Number of decimal places for prices.
        /// \param volume_digits_ Number of decimal places for volumes.
        /// \param tick_size_ Minimum price increment (0 = auto from price_digits).
        /// \param expiration_time Expiration time for this contract (0 for perpetual or spot).
        /// \param next_expiration_time Expiration time of the next contract (0 if unknown).
        constexpr TickCodecConfig(TickStorageFlags mask,
                                  std::uint8_t price_digits_,
                                  std::uint8_t volume_digits_,
                                  double tick_size_ = 0.0,
                                  std::uint64_t expiration_time = 0,
                                  std::uint64_t next_expiration_time = 0) noexcept
            : tick_size(tick_size_)
            , expiration_time_ms(expiration_time)
            , next_expiration_time_ms(next_expiration_time)
            , flags(mask)
            , price_digits(price_digits_)
            , volume_digits(volume_digits_) {}

        /// \brief Sets a flag in the configuration.
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
        /// \param flag Flag to set.
        constexpr void clear_flag(TickStorageFlags flag) noexcept { flags &= ~flag; }

        /// \brief Checks if a specific flag is set.
        /// \param flag Flag to check.
        /// \return True if the flag is enabled, otherwise false.
        [[nodiscard]] constexpr bool has_flag(TickStorageFlags flag) const noexcept {
            return (flags & flag) != TickStorageFlags::NONE;
        }
    };

    static_assert(sizeof(TickCodecConfig) == 32,
                  "TickCodecConfig must remain compact for binary headers.");
    static_assert(std::is_trivially_copyable_v<TickCodecConfig>,
                  "TickCodecConfig must stay trivially copyable.");

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Serializes TickCodecConfig to JSON.
    inline void to_json(nlohmann::json& j, const TickCodecConfig& cfg) {
        j = nlohmann::json{
            {"tick_size", cfg.tick_size},
            {"expiration_time_ms", cfg.expiration_time_ms},
            {"next_expiration_time_ms", cfg.next_expiration_time_ms},
            {"flags", static_cast<std::uint64_t>(cfg.flags)},
            {"price_digits", cfg.price_digits},
            {"volume_digits", cfg.volume_digits}
        };

        nlohmann::json reserved = nlohmann::json::array();
        for (auto byte : cfg.reserved) {
            reserved.push_back(byte);
        }
        j["reserved"] = std::move(reserved);
    }

    /// \brief Deserializes TickCodecConfig from JSON.
    inline void from_json(const nlohmann::json& j, TickCodecConfig& cfg) {
        j.at("tick_size").get_to(cfg.tick_size);
        j.at("expiration_time_ms").get_to(cfg.expiration_time_ms);
        j.at("next_expiration_time_ms").get_to(cfg.next_expiration_time_ms);
        std::uint64_t raw_flags = 0;
        j.at("flags").get_to(raw_flags);
        cfg.flags = static_cast<TickStorageFlags>(raw_flags);
        j.at("price_digits").get_to(cfg.price_digits);
        j.at("volume_digits").get_to(cfg.volume_digits);

        auto reserved = j.value("reserved", std::vector<std::uint8_t>(cfg.reserved.size(), 0));
        for (std::size_t i = 0; i < cfg.reserved.size(); ++i) {
            cfg.reserved[i] = i < reserved.size() ? reserved[i] : 0;
        }
    }

#endif // DFH_USE_JSON && DFH_USE_NLOHMANN_JSON

} // namespace dfh

#endif // _DTH_DATA_TICK_ENCODING_CONFIG_HPP_INCLUDED
