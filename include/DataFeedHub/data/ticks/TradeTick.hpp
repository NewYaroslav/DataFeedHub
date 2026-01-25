#pragma once
#ifndef _DFH_DATA_TRADE_TICK_HPP_INCLUDED
#define _DFH_DATA_TRADE_TICK_HPP_INCLUDED

/// \file TradeTick.hpp
/// \brief Defines the TradeTick DTO and helpers for packing trade metadata.
///
/// Packing layout (uint64_t id_and_side):
/// - lower 3 bits  : TradeSide (0..7)
/// - upper 61 bits : trade_id
///
/// Rationale:
/// - TradeSide is expected to be read more often than trade_id, so it is placed
///   into the lowest bits for faster extraction (mask only).

#include "enums.hpp"
#include <type_traits>
#include <cmath>
#include <cstdint>

namespace dfh {

    /// \struct TradeTick
    /// \brief DTO for a trade tick with packed trade identifier and aggressor side.
    ///
    /// \note trade_id must fit into 61 bits. Higher bits are discarded.
    /// \note TradeSide is stored in 3 bits. Values outside 0..7 are discarded.
    struct TradeTick {
        static constexpr std::uint64_t TRADE_SIDE_BITS  = 3ULL;                /// < Bits reserved for TradeSide (stored in the lowest bits).
        static constexpr std::uint64_t TRADE_SIDE_MASK  = (1ULL << TRADE_SIDE_BITS) - 1ULL; ///< Mask for TradeSide bits (lowest 3 bits).
        static constexpr std::uint64_t TRADE_ID_SHIFT   = TRADE_SIDE_BITS;     ///< Shift for trade_id (stored above side bits).
        static constexpr std::uint64_t TRADE_ID_MASK    = (1ULL << 61) - 1ULL; ///< Mask for trade_id bits (upper 61 bits after shifting).

        std::int64_t  time_ms{0};       ///< Trade timestamp in milliseconds since Unix epoch.
        std::uint64_t id_and_side{0};   ///< Packed trade_id and TradeSide (layout described above).
        double        price{0.0};       ///< Trade price.
        double        volume{0.0};      ///< Trade volume.

        /// \brief Default constructor.
        constexpr TradeTick() noexcept = default;

        /// \brief Constructs a trade tick.
        /// \param trade_time   Trade timestamp in milliseconds since Unix epoch.
        /// \param trade_id     Numeric trade identifier (<= 61 bits; higher bits are discarded).
        /// \param side         Aggressor side.
        /// \param trade_price  Trade price.
        /// \param trade_volume Trade volume.
        constexpr TradeTick(
                std::int64_t trade_time,
                std::uint64_t trade_id,
                TradeSide side,
                double trade_price,
                double trade_volume) noexcept :
            time_ms(trade_time),
            id_and_side(pack_id_and_side(trade_id, side)),
            price(trade_price),
            volume(trade_volume) {}

        /// \brief Extracts the trade identifier.
        [[nodiscard]] constexpr std::uint64_t trade_id() const noexcept {
            return extract_trade_id(id_and_side);
        }

        /// \brief Extracts the aggressor side.
        [[nodiscard]] constexpr TradeSide trade_side() const noexcept {
            return extract_trade_side(id_and_side);
        }

		/// \brief Returns the maximum possible trade_id (61 bits).
		/// \details This is the largest value that can be encoded in the trade_id field of the packed id_and_side.
		/// \return Maximum trade_id (61 bits).
		[[nodiscard]] static constexpr std::uint64_t max_trade_id() noexcept {
			return TRADE_ID_MASK;  // 61 bits mask
		}

        /// \brief Sets both the trade identifier and aggressor side.
        /// \param trade_id New trade identifier (<= 61 bits; higher bits are discarded).
        /// \param side     New aggressor side.
        constexpr void set_trade(std::uint64_t trade_id, TradeSide side) noexcept {
            id_and_side = pack_id_and_side(trade_id, side);
        }

        /// \brief Updates only the trade identifier (preserves side bits).
        /// \param trade_id New trade identifier (<= 61 bits; higher bits are discarded).
        constexpr void set_trade_id(std::uint64_t trade_id) noexcept {
            const std::uint64_t side_bits = (id_and_side & TRADE_SIDE_MASK);
            const std::uint64_t id_part = (trade_id & TRADE_ID_MASK) << TRADE_ID_SHIFT;
            id_and_side = id_part | side_bits;
        }

        /// \brief Updates only the aggressor side (preserves id bits).
        /// \param side New aggressor side.
        constexpr void set_trade_side(TradeSide side) noexcept {
            const std::uint64_t id_bits = (id_and_side & ~TRADE_SIDE_MASK);
            const std::uint64_t side_bits = (static_cast<std::uint64_t>(side) & TRADE_SIDE_MASK);
            id_and_side = id_bits | side_bits;
        }

        /// \brief Packs trade_id and side into a single 64-bit value.
        /// \param trade_id Trade identifier (<= 61 bits; higher bits are discarded).
        /// \param side     Aggressor side (stored in 3 bits; higher bits discarded).
        [[nodiscard]] static constexpr std::uint64_t pack_id_and_side(
                std::uint64_t trade_id,
                TradeSide side) noexcept {
            const std::uint64_t id_part = (trade_id & TRADE_ID_MASK) << TRADE_ID_SHIFT;
            const std::uint64_t side_part = (static_cast<std::uint64_t>(side) & TRADE_SIDE_MASK);
            return id_part | side_part;
        }

        /// \brief Extracts trade_id from packed field.
        [[nodiscard]] static constexpr std::uint64_t extract_trade_id(std::uint64_t encoded) noexcept {
            return (encoded >> TRADE_ID_SHIFT) & TRADE_ID_MASK;
        }

        /// \brief Extracts TradeSide from packed field.
        [[nodiscard]] static constexpr TradeSide extract_trade_side(std::uint64_t encoded) noexcept {
            return static_cast<TradeSide>(encoded & TRADE_SIDE_MASK);
        }

        /// \brief Checks whether price/volume are finite and within expected ranges.
        ///
        /// The tick is considered valid if:
        /// - price and volume are finite,
        /// - price > 0,
        /// - volume >= 0.
        [[nodiscard]] bool is_valid() const noexcept {
            return std::isfinite(price)
                && std::isfinite(volume)
                && price > 0.0
                && volume >= 0.0;
        }
    };

    static_assert(std::is_trivially_copyable_v<TradeTick>,
                  "TradeTick must remain trivially copyable for zero-copy I/O and binary serialization.");

    static_assert(sizeof(TradeTick) == 32,
                  "TradeTick size changed unexpectedly (ABI/layout impact).");

    static_assert(alignof(TradeTick) == alignof(double),
                  "TradeTick alignment changed unexpectedly (ABI/layout impact).");

} // namespace dfh

#endif // _DFH_DATA_TRADE_TICK_HPP_INCLUDED
