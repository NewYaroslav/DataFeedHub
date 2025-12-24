#pragma once
#ifndef _DFH_DATA_MARKET_TICK_HPP_INCLUDED
#define _DFH_DATA_MARKET_TICK_HPP_INCLUDED

/// \file MarketTick.hpp
/// \brief Описывает структуру MarketTick из домена data и её JSON-помощники.

#include <cstddef>
#include <cstdint>
#include <deque>
#include <list>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

#include "DataFeedHub/data/ticks/flags.hpp"

namespace dfh {

    /// \struct MarketTick
    /// \brief Содержит временные метки, цены, объёмы и флаги обновлений для одного тика.
    struct MarketTick {
        std::uint64_t time_ms{0};      ///< Временная метка тика (мс от эпохи Unix).
        std::uint64_t received_ms{0};  ///< Время получения тика (мс от эпохи Unix).
        double ask{0.0};               ///< Цена лучшего запроса.
        double bid{0.0};               ///< Цена лучшего предложения.
        double last{0.0};              ///< Цена последней сделки.
        double volume{0.0};            ///< Объём сделки в базовых единицах (по умолчанию 0).
        TickUpdateFlags flags{TickUpdateFlags::NONE}; ///< Маска флагов обновлений полей.

        /// \brief Конструктор по умолчанию.
        constexpr MarketTick() noexcept = default;

        /// \brief Полный конструктор с указанием времени, цен и флагов.
        /// \param time Временная метка тика (мс с начала эпохи Unix).
        /// \param received Время получения тика (мс с начала эпохи Unix).
        /// \param ask_price Цена лучшего запроса при поступлении тика.
        /// \param bid_price Цена лучшего предложения при поступлении тика.
        /// \param last_price Цена последней сделки на момент тика.
        /// \param volume_value Объём сделки в единицах базового актива.
        /// \param flag_mask Маска флагов, описывающая, какие поля обновились.
        constexpr MarketTick(std::uint64_t time,
                             std::uint64_t received,
                             double ask_price,
                             double bid_price,
                             double last_price,
                             double volume_value,
                             TickUpdateFlags flag_mask) noexcept
            : time_ms(time)
            , received_ms(received)
            , ask(ask_price)
            , bid(bid_price)
            , last(last_price)
            , volume(volume_value)
            , flags(flag_mask) {}

        /// \brief Устанавливает конкретный флаг обновления.
        /// \param flag Флаг, который необходимо включить.
        constexpr void set_flag(TickUpdateFlags flag) noexcept { flags |= flag; }

        /// \brief Устанавливает или сбрасывает флаг по булеву значению.
        /// \param flag Флаг, который нужно обновить.
        /// \param value true – устанавливает флаг, false – сбрасывает.
        constexpr void set_flag(TickUpdateFlags flag, bool value) noexcept {
            if (value) flags |= flag;
            else flags &= ~flag;
        }

        /// \brief Проверяет, установлен ли флаг обновления.
        /// \param flag Флаг для проверки.
        /// \return true, если флаг установлен, иначе false.
        [[nodiscard]] constexpr bool has_flag(TickUpdateFlags flag) const noexcept {
            return (flags & flag) != TickUpdateFlags::NONE;
        }
    };

    static_assert(std::is_trivially_copyable_v<MarketTick>,
                  "MarketTick must remain trivially copyable for ring buffers.");

} // namespace dfh
#endif // _DFH_DATA_MARKET_TICK_HPP_INCLUDED
