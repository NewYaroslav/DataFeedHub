#pragma once
#ifndef _DFH_DATA_TRADE_TICK_HPP_INCLUDED
#define _DFH_DATA_TRADE_TICK_HPP_INCLUDED

/// \file TradeTick.hpp
/// \brief Defines TradeTick structure and helpers for trade metadata packing.

namespace dfh {

    /// \struct TradeTick
    /// \brief Trade DTO with tightly packed trade identifier and aggressor side.
    /// \warning Trade identifier must fit into 61 bits; higher bits are truncated by the mask.
    /// \note Upper three bits store TradeSide; values outside 0..7 are truncated by the mask as well.
    struct TradeTick {
        static constexpr std::uint64_t TRADE_SIDE_BITS = 3;             ///< Bits reserved for TradeSide.
        static constexpr std::uint64_t TRADE_SIDE_SHIFT = 61;           ///< Bit shift for TradeSide (upper bits).
        static constexpr std::uint64_t TRADE_SIDE_MASK = (std::uint64_t{1} << TRADE_SIDE_BITS) - 1; ///< Mask of TradeSide bits.
        static constexpr std::uint64_t TRADE_ID_MASK = (std::uint64_t{1} << TRADE_SIDE_SHIFT) - 1;  ///< Mask of trade id bits.

        double price{0.0};        ///< Trade price.
        double volume{0.0};       ///< Trade volume.
        std::uint64_t time_ms{0}; ///< Trade timestamp in milliseconds.
        std::uint64_t id_and_side{0}; ///< Packed trade id and side information.

        /// \brief Default constructor.
        constexpr TradeTick() noexcept = default;

        /// \brief Full constructor.
        /// \param trade_price Trade price.
        /// \param trade_volume Trade volume.
        /// \param trade_time Trade timestamp (milliseconds since Unix epoch).
        /// \param trade_id Numeric trade identifier (<= 61 bits; higher bits truncated).
        /// \param side Aggressor direction of the trade (expects values 0..7).
        constexpr TradeTick(double trade_price,
                            double trade_volume,
                            std::uint64_t trade_time,
                            std::uint64_t trade_id,
                            TradeSide side) noexcept
            : price(trade_price)
            , volume(trade_volume)
            , time_ms(trade_time)
            , id_and_side(pack_id_and_side(trade_id, side)) {}

        /// \brief Extracts the trade identifier.
        /// \return Trade identifier stripped from side bits.
        [[nodiscard]] constexpr std::uint64_t trade_id() const noexcept {
            return extract_trade_id(id_and_side);
        }

        /// \brief Extracts the aggressor side.
        /// \return TradeSide stored in the packed field.
        [[nodiscard]] constexpr TradeSide trade_side() const noexcept {
            return extract_trade_side(id_and_side);
        }

        /// \brief Updates the packed field with both trade id and side.
        /// \param trade_id Trade identifier.
        /// \param side Aggressor side.
        constexpr void set_trade(std::uint64_t trade_id, TradeSide side) noexcept {
            id_and_side = pack_id_and_side(trade_id, side);
        }

        /// \brief Updates only the trade identifier portion.
        /// \param trade_id Trade identifier to store.
        constexpr void set_trade_id(std::uint64_t trade_id) noexcept {
            id_and_side = (id_and_side & ~TRADE_ID_MASK) | (trade_id & TRADE_ID_MASK);
        }

        /// \brief Updates only the trade side portion.
        /// \param side Aggressor side to store.
        constexpr void set_trade_side(TradeSide side) noexcept {
            const auto side_bits = (static_cast<std::uint64_t>(side) & TRADE_SIDE_MASK) << TRADE_SIDE_SHIFT;
            id_and_side = (id_and_side & TRADE_ID_MASK) | side_bits;
        }

        /// \brief Helper that packs id and side into the storage layout.
        /// \param trade_id Trade identifier to pack.
        /// \param side Trade side to pack.
        /// \return Packed representation.
        [[nodiscard]] static constexpr std::uint64_t pack_id_and_side(std::uint64_t trade_id,
                                                                     TradeSide side) noexcept {
            return (trade_id & TRADE_ID_MASK)
                 | ((static_cast<std::uint64_t>(side) & TRADE_SIDE_MASK) << TRADE_SIDE_SHIFT);
        }

        /// \brief Extracts trade identifier from packed storage.
        /// \param encoded Packed id+side value.
        /// \return Trade identifier.
        [[nodiscard]] static constexpr std::uint64_t extract_trade_id(std::uint64_t encoded) noexcept {
            return encoded & TRADE_ID_MASK;
        }

        /// \brief Extracts trade side from packed storage.
        /// \param encoded Packed id+side value.
        /// \return Trade side.
        [[nodiscard]] static constexpr TradeSide extract_trade_side(std::uint64_t encoded) noexcept {
            return static_cast<TradeSide>((encoded >> TRADE_SIDE_SHIFT) & TRADE_SIDE_MASK);
        }
    };

    static_assert(std::is_trivially_copyable_v<TradeTick>,
                  "TradeTick must remain trivially copyable for zero-copy transports.");
    static_assert(sizeof(TradeTick) == 32, "TradeTick size must be 32 bytes (layout changed?)");

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Serializes TradeTick to JSON.
    /// \param j JSON destination object.
    /// \param tick TradeTick to serialize.
    inline void to_json(nlohmann::json& j, const TradeTick& tick) {
        j = nlohmann::json{
            {"price", tick.price},
            {"volume", tick.volume},
            {"time_ms", tick.time_ms},
            {"trade_id", tick.trade_id()},
            {"side", static_cast<std::uint8_t>(tick.trade_side())}
        };
    }

    /// \brief Deserializes TradeTick from JSON.
    /// \param j JSON source object.
    /// \param tick TradeTick to populate.
    inline void from_json(const nlohmann::json& j, TradeTick& tick) {
        j.at("price").get_to(tick.price);
        j.at("volume").get_to(tick.volume);
        j.at("time_ms").get_to(tick.time_ms);

        if (j.contains("id_and_side")) {
            j.at("id_and_side").get_to(tick.id_and_side);
        } else {
            const auto trade_id = j.value("trade_id", std::uint64_t{0});
            const auto side_raw = j.value("side", std::uint32_t{0});
            tick.id_and_side = TradeTick::pack_id_and_side(trade_id, static_cast<TradeSide>(side_raw));
        }
    }

#endif // DFH_USE_JSON && DFH_USE_NLOHMANN_JSON

} // namespace dfh

#endif // _DFH_DATA_TRADE_TICK_HPP_INCLUDED
