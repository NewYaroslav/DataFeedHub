#pragma once
#ifndef _DFH_DATA_BAR_METADATA_HPP_INCLUDED
#define _DFH_DATA_BAR_METADATA_HPP_INCLUDED

/// \file BarMetadata.hpp
/// \brief Defines the metadata structure for bar data.

namespace dfh {

    /// \struct BarMetadata
    /// \brief Metadata describing bar data configuration for a symbol and exchange.
    struct BarMetadata {
        uint64_t   start_time_ms;           ///< Start timestamp of bar series in milliseconds.
        uint64_t   end_time_ms;             ///< End timestamp of bar series in milliseconds.
        uint64_t   expiration_time_ms;      ///< Expiration time for futures (0 for perpetual or spot).
        uint64_t   next_expiration_time_ms; ///< Expiration time of the next contract (0 if not defined).
        double     tick_size;               ///< Minimum price increment (tick size).
        TimeFrame  time_frame;              ///< Timeframe used for aggregation.
        BarStorageFlags flags;              ///< Bitmask controlling bar format and compression.
        uint32_t   count;                   ///< Number of bars in the dataset.
        uint16_t   symbol_id;               ///< Symbol identifier.
        uint16_t   exchange_id;             ///< Exchange identifier.
        MarketType market_type;             ///< Market type (spot, futures, etc.)
        uint8_t    price_digits;            ///< Number of decimal places for price.
        uint8_t    volume_digits;           ///< Number of decimal places for volume.
        uint8_t    quote_volume_digits;     ///< Number of decimal places for quote volume.
        uint8_t    reserved[4];             ///< Reserved for future use; ensures structure size is 64 bytes and 8-byte aligned.

        /// \brief Default constructor. Initializes all fields to zero or defaults.
        BarMetadata()
            : start_time_ms(0), end_time_ms(0),
              expiration_time_ms(0), next_expiration_time_ms(0),
              tick_size(0.0),
              time_frame(TimeFrame::S1),
              flags(BarStorageFlags::NONE), count(0),
              symbol_id(0), exchange_id(0), market_type(MarketType::UNKNOWN),
              price_digits(0), volume_digits(0), quote_volume_digits(0) {
            std::fill(std::begin(reserved), std::end(reserved), '\0');
        }

        /// \brief Constructs BarMetadata with all fields explicitly set.
        /// \param start_time_ms Start timestamp in milliseconds.
        /// \param end_time_ms End timestamp in milliseconds.
        /// \param time_frame Timeframe for bar aggregation.
        /// \param flags Bitmask controlling bar format and compression.
        /// \param count Number of bars in the dataset.
        /// \param market_type Market type (e.g., spot, futures, options).
        /// \param exchange_id Exchange identifier.
        /// \param symbol_id Symbol identifier.
        /// \param price_digits Number of decimal places for price.
        /// \param volume_digits Number of decimal places for volume.
        /// \param quote_volume_digits Number of decimal places for quote volume.
        /// \param tick_size Minimum price increment (0.0 if unknown).
        /// \param expiration_time_ms Expiration time for futures (0 for perpetual or spot).
        /// \param next_expiration_time_ms Expiration time of the next contract (0 if unknown).
        BarMetadata(
            uint64_t start_time_ms,
            uint64_t end_time_ms,
            TimeFrame time_frame,
            BarStorageFlags flags,
            uint32_t count,
            MarketType market_type,
            uint16_t exchange_id,
            uint16_t symbol_id,
            uint8_t price_digits,
            uint8_t volume_digits,
            uint8_t quote_volume_digits,
            double tick_size = 0.0,
            uint64_t expiration_time_ms = 0,
            uint64_t next_expiration_time_ms = 0)
            : start_time_ms(start_time_ms),
              end_time_ms(end_time_ms),
              expiration_time_ms(expiration_time_ms),
              next_expiration_time_ms(next_expiration_time_ms),
              tick_size(tick_size),
              time_frame(time_frame),
              flags(flags),
              count(count),
              symbol_id(symbol_id),
              exchange_id(exchange_id),
              market_type(market_type),
              price_digits(price_digits),
              volume_digits(volume_digits),
              quote_volume_digits(quote_volume_digits) {
            std::fill(std::begin(reserved), std::end(reserved), '\0');
        }

        /// \brief Sets a flag in the metadata.
        /// \param flag Flag to set.
        void set_flag(BarStorageFlags flag) {
            flags = static_cast<BarStorageFlags>(
                static_cast<uint32_t>(flags) | static_cast<uint32_t>(flag));
        }

        /// \brief Checks if a specific flag is set.
        /// \param flag Flag to check.
        /// \return True if the flag is set, false otherwise.
        bool has_flag(BarStorageFlags flag) const {
            return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
        }
    };

} // namespace dfh

#endif // _DFH_DATA_BAR_METADATA_HPP_INCLUDED
