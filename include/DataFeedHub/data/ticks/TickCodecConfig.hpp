#pragma once
#ifndef _DTH_DATA_TICK_ENCODING_CONFIG_HPP_INCLUDED
#define _DTH_DATA_TICK_ENCODING_CONFIG_HPP_INCLUDED

/// \file TickCodecConfig.hpp
/// \brief Defines configuration for encoding and decoding tick sequences.

#include <cstdint>

namespace dfh {

    /// \struct TickCodecConfig
    /// \brief Parameters for tick data compression, serialization, and storage.
    ///
    /// This structure defines the encoding and decoding configuration for tick data.
    /// The `flags` field controls optional features such as trade-based encoding, tick flags,
    /// received timestamp storage, and binary format selection.
    struct TickCodecConfig {
        double   tick_size;      ///< Minimum price increment (tick size).
        uint64_t expiration_time_ms;      ///< Expiration time for futures (0 for perpetual or spot).
        uint64_t next_expiration_time_ms; ///< Expiration time of the next contract (0 if not defined).
        TickStorageFlags flags;  ///< Encoding flags (bitmask of `TickStorageFlags`).
        uint8_t  price_digits;   ///< Number of decimal places for prices.
        uint8_t  volume_digits;  ///< Number of decimal places for volumes.
        uint8_t  reserved[5];    ///< Reserved for future use; ensures structure size is 32 bytes and 8-byte aligned.

        /// \brief Default constructor for TickCodecConfig.
        /// \details Initializes all fields with default values.
        TickCodecConfig()
            : tick_size(0.0),
              expiration_time_ms(0),
              next_expiration_time_ms(0),
              flags(TickStorageFlags::NONE),
              price_digits(0),
              volume_digits(0) {
            std::fill(std::begin(reserved), std::end(reserved), '\0');
        }

        /// \brief Constructor to initialize all fields explicitly.
        /// \param flags Bitmask of `TickStorageFlags`.
        /// \param price_digits Number of decimal places for prices.
        /// \param volume_digits Number of decimal places for volumes.
        /// \param tick_size Minimum price increment (0 = auto from price_digits).
        /// \param expiration_time_ms Expiration time for this contract (0 for perpetual or spot).
        /// \param next_expiration_time_ms Expiration time of the next contract (0 if unknown).
        TickCodecConfig(
                TickStorageFlags flags,
                uint8_t  price_digits,
                uint8_t  volume_digits,
                double   tick_size = 0.0,
                uint64_t expiration_time_ms = 0,
                uint64_t next_expiration_time_ms = 0)
            : tick_size(tick_size),
              expiration_time_ms(expiration_time_ms),
              next_expiration_time_ms(next_expiration_time_ms),
              flags(flags),
              price_digits(price_digits),
              volume_digits(volume_digits) {
            std::fill(std::begin(reserved), std::end(reserved), '\0');
        }

        /// \brief Sets a flag in the configuration.
        /// \param flag Flag to enable.
        void set_flag(TickStorageFlags flag) {
            flags |= flag;
        }

        /// \brief Sets or clears a flag in the configuration.
        /// \param flag Flag to modify.
        /// \param value If true, the flag is set; if false, the flag is cleared.
        void set_flag(TickStorageFlags flag, bool value) {
            if (value) flags |= flag;
            else flags &= ~flag;
        }

        /// \brief Clears a flag in the configuration.
        /// \param flag Flag to set.
        void clear_flag(TickStorageFlags flag) {
            flags &= ~flag;
        }

        /// \brief Checks if a specific flag is set.
        /// \param flag Flag to check.
        /// \return True if the flag is enabled, otherwise false.
        bool has_flag(TickStorageFlags flag) const {
            return (flags & flag) != 0;
        }
    };

} // namespace dfh

#endif // _DTH_DATA_TICK_ENCODING_CONFIG_HPP_INCLUDED
