#pragma once
#ifndef _DFH_DATA_TICK_METADATA_HPP_INCLUDED
#define _DFH_DATA_TICK_METADATA_HPP_INCLUDED

/// \file TickMetadata.hpp
/// \brief Defines the metadata structure for tick data.

namespace dfh {

    /// \struct TickMetadata
    /// \brief Tick data metadata for a trading symbol and provider.
    struct TickMetadata {
        uint64_t start_time_ms;      ///< Start timestamp of tick series in milliseconds.
        uint64_t end_time_ms;        ///< End timestamp of tick series in milliseconds.
        uint64_t expiration_time_ms; ///< Expiration time for futures (0 for perpetual or spot).
        uint64_t next_expiration_time_ms; ///< Expiration time of the next contract (0 if not defined).
        uint64_t count;              ///< Number of ticks in the dataset.
        double   tick_size;          ///< Minimum price increment (tick size).
        uint16_t symbol_id;          ///< Symbol identifier.
        uint16_t exchange_id;        ///< Exchange identifier.
        MarketType market_type;      ///< Market type (spot, futures, etc.).
        uint8_t  price_digits;       ///< Number of decimal places for price.
        uint8_t  volume_digits;      ///< Number of decimal places for volume.
        TickStorageFlags flags;      ///< Tick metadata flags (bitmask of `TickStorageFlags`).

        /// \brief Default constructor. Initializes all fields to zero/defaults.
        TickMetadata()
            : start_time_ms(0),
              end_time_ms(0),
              expiration_time_ms(0),
              next_expiration_time_ms(0),
              count(0),
              tick_size(0.0),
              symbol_id(0),
              exchange_id(0),
              market_type(MarketType::UNKNOWN),
              price_digits(0),
              volume_digits(0),
              flags(TickStorageFlags::NONE) {
        }

        /// \brief Constructs TickMetadata with all fields explicitly set.
        /// \param start_time_ms Start time of the tick data in milliseconds.
        /// \param end_time_ms End time of the tick data in milliseconds.
        /// \param count Number of ticks in the dataset.
        /// \param market_type Type of trading market (spot, futures, etc.).
        /// \param exchange_id Exchange identifier.
        /// \param symbol_id Symbol identifier.
        /// \param price_digits Number of decimal places for price.
        /// \param volume_digits Number of decimal places for volume.
        /// \param flags Bitmask of TickStorageFlags.
        /// \param tick_size Minimum price increment (tick size).
        /// \param expiration_time_ms Expiration time for this contract (0 for perpetual or spot).
        /// \param next_expiration_time_ms Expiration time of the next contract (0 if unknown).
        TickMetadata(
            uint64_t start_time_ms,
            uint64_t end_time_ms,
            uint64_t count,
            MarketType market_type,
            uint16_t exchange_id,
            uint16_t symbol_id,
            uint8_t price_digits,
            uint8_t volume_digits,
            TickStorageFlags flags,
            double tick_size = 0.0,
            uint64_t expiration_time_ms = 0,
            uint64_t next_expiration_time_ms = 0)
            : start_time_ms(start_time_ms),
              end_time_ms(end_time_ms),
              expiration_time_ms(expiration_time_ms),
              next_expiration_time_ms(next_expiration_time_ms),
              count(count),
              tick_size(tick_size),
              symbol_id(symbol_id),
              exchange_id(exchange_id),
              market_type(market_type),
              price_digits(price_digits),
              volume_digits(volume_digits),
              flags(flags) {
        }

        /// \brief Sets a flag in the metadata.
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
        bool has_flag(TickStorageFlags flag) const {
            return (flags & flag) != 0;
        }
    };

} // namespace dfh

#endif // _DFH_DATA_TICK_METADATA_HPP_INCLUDED
