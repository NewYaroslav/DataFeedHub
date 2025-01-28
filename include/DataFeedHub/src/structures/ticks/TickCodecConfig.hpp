#pragma once
#ifndef _DTH_DATA_TICK_ENCODING_CONFIG_HPP_INCLUDED
#define _DTH_DATA_TICK_ENCODING_CONFIG_HPP_INCLUDED

/// \file TickCodecConfig.hpp
/// \brief Defines configuration for encoding and decoding tick sequences.

#include <cstdint>

namespace dfh {

    /// \struct TickCodecConfig
    /// \brief Parameters for tick data compression/decompression.
    struct TickCodecConfig {
        size_t price_digits;        ///< Number of decimal places for prices.
        size_t volume_digits;       ///< Number of decimal places for volumes.
        bool enable_trade_based;    ///< Optimize encoding for trade-based data where only `last` prices are available.
        bool enable_tick_flags;     ///< Enable processing of `TickUpdateFlags` during encoding and decoding.
        bool enable_received_time;  ///< Include the `received_time` field in the encoded data.

        /// \brief Default constructor for `TickCodecConfig`.
        /// \details Initializes all fields with default values.
        TickCodecConfig()
            : price_digits(0),              ///< Default to decimal places for prices.
              volume_digits(0),             ///< Default to integer volume representation.
              enable_trade_based(false), ///< Default to standard tick encoding.
              enable_tick_flags(false),           ///< Default to not processing `TickUpdateFlags`.
              enable_received_time(false) {}      ///< Default to excluding `received_time`.

        /// \brief Constructor to initialize all fields explicitly.
        /// \param price_digits Number of decimal places for prices.
        /// \param volume_digits Number of decimal places for volumes.
        /// \param trade_based Enable trade-based encoding.
        /// \param tick_flags Enable processing of `TickUpdateFlags`.
        /// \param received_time Include the `received_time` field in encoded data.
        TickCodecConfig(
            size_t price_digits,
            size_t volume_digits,
            bool trade_based,
            bool tick_flags,
            bool received_time)
            : price_digits(price_digits),
              volume_digits(volume_digits),
              enable_trade_based(trade_based),
              enable_tick_flags(tick_flags),
              enable_received_time(received_time) {}
    };

} // namespace dfh

#endif // _DTH_DATA_TICK_ENCODING_CONFIG_HPP_INCLUDED
