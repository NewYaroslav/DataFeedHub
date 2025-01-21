#pragma once
#ifndef _DTH_DATA_TICK_ENCODING_CONFIG_HPP_INCLUDED
#define _DTH_DATA_TICK_ENCODING_CONFIG_HPP_INCLUDED

/// \file TickEncodingConfig.hpp
/// \brief Defines configuration for encoding and decoding tick sequences.

#include <cstdint>

namespace dfh {

    /// \enum TickEncodingMode
    /// \brief Defines modes for handling `bid` and `ask` prices during encoding and decoding.
    enum class TickEncodingMode {

        /// \brief Only the `last` price is stored. `bid` and `ask` prices are not reconstructed.
        /// \details This mode minimizes data storage by excluding any computation or storage of `bid` and `ask` prices.
        LAST_ONLY           = 0,

        /// \brief `bid` and `ask` prices are generated using a fixed spread around the `last` price.
        /// \details In this mode, `ask` is calculated as `last + (spread / 2)`, and `bid` is calculated as `last - (spread / 2)`.
        /// The `spread` value is defined in the `TickEncodingConfig` structure in pips.
        FIXED_SPREAD        = 0x1,

        /// \brief Key points with known spreads are identified, and spreads are extrapolated forward.
        /// \details
        /// - Between key points, the spread is linearly extrapolated into the future until the next key point is reached.
        /// - Before the first key point, the spread is calculated as the average of the known spreads.
        /// - This mode is suitable when `ask` and `bid` prices are dynamically calculated, ensuring reasonable spread estimates.
        TRADE_BASED_FORWARD = 0x2,

        /// \brief Key points with known spreads are identified, and a mixed extrapolation approach is used.
        /// \details
        /// - For each period between two key points, the maximum spread between the two points is used.
        /// - Before the first key point and after the last key point:
        ///   - The spread is calculated as the maximum of the configured spread, the average spread, and the spread at the nearest key point.
        /// - This mode prioritizes conservative spread estimates and is suitable for ensuring reliable `ask` and `bid` reconstruction.
        TRADE_BASED_MIXED = 0x3
    };

    /// \struct TickEncodingConfig
    /// \brief Configuration for encoding and decoding tick sequences.
    struct TickEncodingConfig {
        size_t price_digits;             ///< Number of decimal places for prices.
        size_t volume_digits;            ///< Number of decimal places for volumes.
        size_t fixed_spread;             ///< Fixed spread in pips (used only in `FIXED_SPREAD` mode).
        TickEncodingMode encoding_mode;  ///< Mode for handling bid and ask prices during encoding/decoding.
        bool enable_trade_based_encoding;///< Optimize encoding for trade-based data where only `last` prices are available.
        bool enable_tick_flags;          ///< Enable processing of `TickUpdateFlags` during encoding and decoding.
        bool enable_received_time;       ///< Include the `received_time` field in the encoded data.

        /// \brief Default constructor for `TickEncodingConfig`.
        /// \details Initializes all fields with default values.
        TickEncodingConfig()
            : price_digits(2),              ///< Default to 2 decimal places for prices.
              volume_digits(0),             ///< Default to integer volume representation.
              fixed_spread(0),
              encoding_mode(TickEncodingMode::LAST_ONLY),
              enable_trade_based_encoding(false), ///< Default to standard tick encoding.
              enable_tick_flags(false),           ///< Default to not processing `TickUpdateFlags`.
              enable_received_time(false) {}      ///< Default to excluding `received_time`.

        /// \brief Constructor to initialize all fields explicitly.
        /// \param pd Number of decimal places for prices.
        /// \param vd Number of decimal places for volumes.
        /// \param mode Encoding mode to define how bid and ask prices are handled.
        /// \param spread Fixed spread in pips (only used in `FIXED_SPREAD` mode).
        /// \param trade_based Enable trade-based encoding.
        /// \param tick_flags Enable processing of `TickUpdateFlags`.
        /// \param received_time Include the `received_time` field in encoded data.
        TickEncodingConfig(
            int pd,
            int vd,
            TickEncodingMode mode,
            int spread,
            bool trade_based,
            bool tick_flags,
            bool received_time)
            : price_digits(pd),
              volume_digits(vd),
              fixed_spread(spread),
              encoding_mode(mode),
              enable_trade_based_encoding(trade_based),
              enable_tick_flags(tick_flags),
              enable_received_time(received_time) {}
    };

} // namespace dfh

#endif // _DTH_DATA_TICK_ENCODING_CONFIG_HPP_INCLUDED
