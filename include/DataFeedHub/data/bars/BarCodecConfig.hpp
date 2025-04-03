#pragma once
#ifndef _DTH_DATA_BAR_CODEC_CONFIG_HPP_INCLUDED
#define _DTH_DATA_BAR_CODEC_CONFIG_HPP_INCLUDED

/// \file BarCodecConfig.hpp
/// \brief Defines configuration options for bar codec serialization.

namespace dfh {

    /// \struct BarCodecConfig
    /// \brief Configuration structure for encoding and decoding bar data.
    struct BarCodecConfig {
        double          tick_size;               ///< Minimum price increment (tick size).
        uint64_t        expiration_time_ms;      ///< Expiration time for futures (0 for perpetual or spot).
        uint64_t        next_expiration_time_ms; ///< Expiration time of the next contract (0 if unknown).
        TimeFrame       time_frame;              ///< Timeframe used for bar aggregation.
        BarStorageFlags flags;                   ///< Flags controlling bar storage and features.
        uint8_t         price_digits;            ///< Number of decimal digits for prices.
        uint8_t         volume_digits;           ///< Number of decimal digits for volume and buy_volume.
        uint8_t         quote_volume_digits;     ///< Number of decimal digits for quote_volume and buy_quote_volume.
        uint8_t         reserved[5];             ///< Reserved for future use; ensures structure size is 40 bytes and 8-byte aligned.

        /// \brief Default constructor.
        /// \details Initializes all fields with default values.
        BarCodecConfig()
            : tick_size(0.0),
              expiration_time_ms(0),
              next_expiration_time_ms(0),
              time_frame(TimeFrame::UNKNOWN),
              flags(BarStorageFlags::NONE),
              price_digits(0),
              volume_digits(0),
              quote_volume_digits(0) {
            std::fill(std::begin(reserved), std::end(reserved), '\0');
        }

        /// \brief Constructor to initialize all fields explicitly.
        /// \param time_frame Bar aggregation timeframe.
        /// \param flags Bitmask for storage/encoding flags.
        /// \param price_digits Number of decimal digits for prices.
        /// \param volume_digits Number of decimal digits for volume and buy volume (base).
        /// \param quote_volume_digits Number of decimal digits for quote volume and buy quote volume.
        /// \param tick_size Minimum price increment (0 = auto).
        /// \param expiration_time_ms Futures expiration timestamp (0 if perpetual).
        /// \param next_expiration_time_ms Expiration time of the next contract (0 if unknown).
        BarCodecConfig(
            TimeFrame time_frame,
            BarStorageFlags flags,
            uint8_t price_digits,
            uint8_t volume_digits,
            uint8_t quote_volume_digits,
            double tick_size = 0.0,
            uint64_t expiration_time_ms = 0,
            uint64_t next_expiration_time_ms = 0)
            : tick_size(tick_size),
              expiration_time_ms(expiration_time_ms),
              next_expiration_time_ms(next_expiration_time_ms),
              time_frame(time_frame),
              flags(flags),
              price_digits(price_digits),
              volume_digits(volume_digits),
              quote_volume_digits(quote_volume_digits) {
            std::fill(std::begin(reserved), std::end(reserved), '\0');
        }

        /// \brief Sets a flag in the configuration.
        /// \param flag The flag to enable.
        void set_flag(BarStorageFlags flag) {
            flags = static_cast<BarStorageFlags>(
                static_cast<uint64_t>(flags) | static_cast<uint64_t>(flag));
        }

        /// \brief Sets or clears a flag in the configuration.
        /// \param flag The flag to modify.
        /// \param value If true, the flag is set; if false, the flag is cleared.
        void set_flag(BarStorageFlags flag, bool value) {
            if (value) {
                flags = static_cast<BarStorageFlags>(
                    static_cast<uint64_t>(flags) | static_cast<uint64_t>(flag));
            } else {
                flags = static_cast<BarStorageFlags>(
                    static_cast<uint64_t>(flags) & ~static_cast<uint64_t>(flag));
            }
        }

        /// \brief Clears a flag in the configuration.
        /// \param flag The flag to disable.
        void clear_flag(BarStorageFlags flag) {
            flags = static_cast<BarStorageFlags>(
                static_cast<uint64_t>(flags) & ~static_cast<uint64_t>(flag));
        }

        /// \brief Checks if a specific flag is set.
        /// \param flag The flag to check.
        /// \return True if the flag is enabled, otherwise false.
        bool has_flag(BarStorageFlags flag) const {
            return (static_cast<uint64_t>(flags) & static_cast<uint64_t>(flag)) != 0;
        }
    };

} // namespace dfh

#endif // _DTH_DATA_BAR_CODEC_CONFIG_HPP_INCLUDED
