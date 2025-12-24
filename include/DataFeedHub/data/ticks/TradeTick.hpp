#pragma once
#ifndef _DFH_DATA_TRADE_TICK_HPP_INCLUDED
#define _DFH_DATA_TRADE_TICK_HPP_INCLUDED

/// \file TradeTick.hpp
/// \brief Описывает TradeTick и вспомогательные утилиты для упаковки метаданных сделок.

#include <cstdint>
#include <type_traits>

#include "DataFeedHub/data/ticks/enums.hpp"

namespace dfh {

    /// \struct TradeTick
    /// \brief DTO сделки с упакованными идентификатором и направлением агрессора.
    /// \warning Идентификатор сделки должен укладываться в 61 бит; старшие биты обрезаются маской.
    /// \note Старшие три бита хранят TradeSide; значения вне диапазона 0..7 также обрезаются.
    struct TradeTick {
        static constexpr std::uint64_t TRADE_SIDE_BITS = 3;             ///< Биты, отведённые под TradeSide.
        static constexpr std::uint64_t TRADE_SIDE_SHIFT = 61;           ///< Смещение битов TradeSide (старшие разряды).
        static constexpr std::uint64_t TRADE_SIDE_MASK = (std::uint64_t{1} << TRADE_SIDE_BITS) - 1; ///< Маска битов TradeSide.
        static constexpr std::uint64_t TRADE_ID_MASK = (std::uint64_t{1} << TRADE_SIDE_SHIFT) - 1;  ///< Маска идентификатора сделки.

        double price{0.0};        ///< Цена сделки.
        double volume{0.0};       ///< Объём сделки.
        std::uint64_t time_ms{0}; ///< Временная метка сделки в миллисекундах.
        std::uint64_t id_and_side{0}; ///< Упакированный идентификатор и направление.

        /// \brief Конструктор по умолчанию.
        constexpr TradeTick() noexcept = default;

        /// \brief Полный конструктор с упакованным идентификатором и направлением.
        /// \param trade_price Цена сделки.
        /// \param trade_volume Объём сделки.
        /// \param trade_time Временная метка сделки в миллисекундах от эпохи Unix.
        /// \param trade_id Числовой идентификатор сделки (<= 61 бит, старшие биты отбрасываются).
        /// \param side Направление агрессора (ожидается 0..7).
        constexpr TradeTick(double trade_price,
                            double trade_volume,
                            std::uint64_t trade_time,
                            std::uint64_t trade_id,
                            TradeSide side) noexcept
            : price(trade_price)
            , volume(trade_volume)
            , time_ms(trade_time)
            , id_and_side(pack_id_and_side(trade_id, side)) {}

        /// \brief Извлекает идентификатор сделки.
        /// \return Идентификатор сделки без битов направления.
        [[nodiscard]] constexpr std::uint64_t trade_id() const noexcept {
            return extract_trade_id(id_and_side);
        }

        /// \brief Извлекает направление агрессора.
        /// \return Значение TradeSide из упакованной области.
        [[nodiscard]] constexpr TradeSide trade_side() const noexcept {
            return extract_trade_side(id_and_side);
        }

        /// \brief Записывает одновременно идентификатор и направление в упаковку.
        /// \param trade_id Идентификатор сделки.
        /// \param side Направление агрессора.
        constexpr void set_trade(std::uint64_t trade_id, TradeSide side) noexcept {
            id_and_side = pack_id_and_side(trade_id, side);
        }

        /// \brief Обновляет только часть с идентификатором.
        /// \param trade_id Идентификатор сделки для записи.
        constexpr void set_trade_id(std::uint64_t trade_id) noexcept {
            id_and_side = (id_and_side & ~TRADE_ID_MASK) | (trade_id & TRADE_ID_MASK);
        }

        /// \brief Обновляет только часть с направлением.
        /// \param side Направление агрессора для записи.
        constexpr void set_trade_side(TradeSide side) noexcept {
            const auto side_bits = (static_cast<std::uint64_t>(side) & TRADE_SIDE_MASK) << TRADE_SIDE_SHIFT;
            id_and_side = (id_and_side & TRADE_ID_MASK) | side_bits;
        }

        /// \brief Упаковывает идентификатор и направление в компактное представление.
        /// \param trade_id Идентификатор сделки для упаковки.
        /// \param side Направление агрессора.
        /// \return Упакованное значение.
        [[nodiscard]] static constexpr std::uint64_t pack_id_and_side(std::uint64_t trade_id,
                                                                     TradeSide side) noexcept {
            return (trade_id & TRADE_ID_MASK)
                 | ((static_cast<std::uint64_t>(side) & TRADE_SIDE_MASK) << TRADE_SIDE_SHIFT);
        }

        /// \brief Достаёт идентификатор сделки из упакованного значения.
        /// \param encoded Упакованный идентификатор + направление.
        /// \return Идентификатор сделки.
        [[nodiscard]] static constexpr std::uint64_t extract_trade_id(std::uint64_t encoded) noexcept {
            return encoded & TRADE_ID_MASK;
        }

        /// \brief Достаёт направление агрессора из упакованного значения.
        /// \param encoded Упакованное значение id+side.
        /// \return Направление сделки.
        [[nodiscard]] static constexpr TradeSide extract_trade_side(std::uint64_t encoded) noexcept {
            return static_cast<TradeSide>((encoded >> TRADE_SIDE_SHIFT) & TRADE_SIDE_MASK);
        }
    };

    static_assert(std::is_trivially_copyable_v<TradeTick>,
                  "TradeTick must remain trivially copyable for zero-copy transports.");
    static_assert(sizeof(TradeTick) == 32, "TradeTick size must be 32 bytes (layout changed?)");

} // namespace dfh

#endif // _DFH_DATA_TRADE_TICK_HPP_INCLUDED
