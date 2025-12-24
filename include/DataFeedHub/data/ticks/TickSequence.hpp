#pragma once
#ifndef _DFH_DATA_TICK_SEQUENCE_HPP_INCLUDED
#define _DFH_DATA_TICK_SEQUENCE_HPP_INCLUDED

/// \file TickSequence.hpp
/// \brief Объединяет шаблонную структуру последовательности тиков с набором сопутствующих метаданных.

#include <cstdint>
#include <vector>

#include "DataFeedHub/data/ticks/flags.hpp"

namespace dfh {

    struct MarketTick;
    struct QuoteTick;
    struct ValueTick;

    /// \brief Шаблонная структура последовательности тиков и связанных данных.
    template <typename TickType>
    struct TickSequence {
        std::vector<TickType> ticks{}; ///< Последовательность тиков указанного типа.
        TickUpdateFlags flags{TickUpdateFlags::NONE}; ///< Маска флагов для данных тиков.
        std::uint16_t symbol_index{0};       ///< Индекс символа.
        std::uint16_t provider_index{0};     ///< Индекс источника данных.
        std::uint16_t price_digits{0};       ///< Цифр после запятой для цены.
        std::uint16_t volume_digits{0};      ///< Цифр после запятой для объёма.

        /// \brief Конструктор по умолчанию.
        TickSequence() = default;

        /// \brief Конструктор с непосредственной инициализацией всех полей.
        /// \param ts Последовательность тиков.
        /// \param f Маска флагов.
        /// \param si Индекс символа.
        /// \param pi Индекс источника.
        /// \param d Число знаков после запятой для цены.
        /// \param vd Число знаков после запятой для объёма.
        TickSequence(std::vector<TickType> ts,
                     TickUpdateFlags f,
                     std::uint16_t si,
                     std::uint16_t pi,
                     std::uint16_t d,
                     std::uint16_t vd)
            : ticks(std::move(ts))
            , flags(f)
            , symbol_index(si)
            , provider_index(pi)
            , price_digits(d)
            , volume_digits(vd) {}
    };

    // Специализации последовательности для конкретных типов тиков.
    using ValueTickSequence = TickSequence<ValueTick>;
    using QuoteTickSequence = TickSequence<QuoteTick>;
    using MarketTickSequence = TickSequence<MarketTick>;

} // namespace dfh

#endif // _DFH_DATA_TICK_SEQUENCE_HPP_INCLUDED
