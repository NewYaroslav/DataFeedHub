#pragma once
#ifndef _DFH_DATA_TICK_CODEC_CONFIG_HPP_INCLUDED
#define _DFH_DATA_TICK_CODEC_CONFIG_HPP_INCLUDED

/// \file TickCodecConfig.hpp
/// \brief Defines configuration parameters for encoding and decoding tick sequences.

#include "flags.hpp"
#include <cstddef>     // offsetof
#include <type_traits>

namespace dfh {

    /// \struct TickCodecConfig
    /// \brief Parameters for tick data encoding, compression, and storage.
    ///
    /// Binary layout notes:
    /// - Intended to be stored/used as a fixed-size 32-byte binary header.
    /// - Field offsets are validated to detect ABI/padding regressions in typical builds.
    /// - Alignment is expected to match \c std::uint64_t to keep 64-bit fields naturally aligned.
    ///
    /// The \c flags field controls optional features such as trade-based encoding, tick update flags,
    /// received timestamp storage, and raw-binary mode.
    struct TickCodecConfig {
        TickStorageFlags flags{TickStorageFlags::NONE};///< Encoding/storage feature flags.
        double tick_size{0.0};                         ///< Minimum price increment (tick size).
        std::int64_t expiration_time_ms{0};            ///< Contract expiration time (ms since Unix epoch). 0 for spot/perpetual.
        std::int64_t next_expiration_time_ms{0};       ///< Next contract expiration time (ms since Unix epoch). 0 if unknown.
        std::uint8_t price_digits{0};                  ///< Decimal places for price fields.
        std::uint8_t volume_digits{0};                 ///< Decimal places for volume fields.
        std::uint8_t reserved[6]{};                    ///< Reserved for future use. Must be zeroed.

        /// \brief Default constructor.
        constexpr TickCodecConfig() noexcept = default;
        
        /// \brief Constructs a config with all fields explicitly specified.
        /// \param storage_flags Feature flags mask.
        /// \param price_digits Decimal places for prices (0..18 recommended).
        /// \param volume_digits Decimal places for volumes (0..18 recommended).
        /// \param tick_size Minimum price increment (0.0 means "unspecified/auto", if applicable).
        /// \param expiration_time_ms_ Expiration time for this contract (ms since Unix epoch). 0 for spot/perpetual.
        /// \param next_expiration_time_ms_ Expiration time of the next contract (ms since Unix epoch). 0 if unknown.
        constexpr TickCodecConfig(
                TickStorageFlags storage_flags,
                std::uint8_t price_digits,
                std::uint8_t volume_digits,
                double tick_size = 0.0,
                std::int64_t expiration_time_ms = 0,
                std::int64_t next_expiration_time_ms = 0
            ) noexcept :
            flags(storage_flags),
            tick_size(tick_size),
            expiration_time_ms(expiration_time_ms),
            next_expiration_time_ms(next_expiration_time_ms),
            price_digits(pd),
            volume_digits(vd) {}

        /// \brief Enables a flag.
        /// \param flag Flag to enable.
        constexpr void set_flag(TickStorageFlags flag) noexcept { flags |= flag; }

        /// \brief Enables or disables a flag depending on \p value.
        /// \param flag Flag to modify.
        /// \param value If true, the flag is enabled; otherwise disabled.
        constexpr void set_flag(TickStorageFlags flag, bool value) noexcept {
            if (value) flags |= flag;
            else flags &= ~flag;
        }

        /// \brief Disables a flag.
        /// \param flag Flag to disable.
        constexpr void clear_flag(TickStorageFlags flag) noexcept { flags &= ~flag; }

        /// \brief Checks whether a flag is enabled.
        /// \param flag Flag to check.
        /// \return True if enabled.
        [[nodiscard]] constexpr bool has_flag(TickStorageFlags flag) const noexcept {
            return (flags & flag) != TickStorageFlags::NONE;
        }
    };

    static_assert(sizeof(TickCodecConfig) == 40,
                  "TickCodecConfig must remain 40 bytes for the binary header.");

    static_assert(alignof(TickCodecConfig) == alignof(std::uint64_t),
                  "TickCodecConfig alignment must match std::uint64_t.");
                  
    static_assert(offsetof(TickCodecConfig, flags) == 0,
                  "TickCodecConfig layout changed: flags offset mismatch.");

    static_assert(offsetof(TickCodecConfig, tick_size) == 8,
                  "TickCodecConfig layout changed: tick_size offset mismatch.");

    static_assert(offsetof(TickCodecConfig, expiration_time_ms) == 16,
                  "TickCodecConfig layout changed: expiration_time_ms offset mismatch.");

    static_assert(offsetof(TickCodecConfig, next_expiration_time_ms) == 24,
                  "TickCodecConfig layout changed: next_expiration_time_ms offset mismatch.");

    static_assert(std::is_trivially_copyable_v<TickCodecConfig>,
                  "TickCodecConfig must remain trivially copyable.");

} // namespace dfh

#endif // _DFH_DATA_TICK_CODEC_CONFIG_HPP_INCLUDED

