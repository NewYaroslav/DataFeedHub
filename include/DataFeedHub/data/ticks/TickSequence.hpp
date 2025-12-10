#pragma once
#ifndef _DFH_DATA_TICK_SEQUENCE_HPP_INCLUDED
#define _DFH_DATA_TICK_SEQUENCE_HPP_INCLUDED

/// \file TickSequence.hpp
/// \brief Объединяет шаблонную структуру последовательности тиков с набором сопутствующих метаданных.

namespace dfh {

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

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Сериализует TickSequence в JSON.
    template <typename TickType>
    inline void to_json(nlohmann::json& j, const TickSequence<TickType>& value) {
        j = nlohmann::json{
            {"ticks", value.ticks},
            {"flags", static_cast<std::uint64_t>(value.flags)},
            {"symbol_index", value.symbol_index},
            {"provider_index", value.provider_index},
            {"price_digits", value.price_digits},
            {"volume_digits", value.volume_digits}
        };
    }

    /// \brief Десериализует TickSequence из JSON.
    template <typename TickType>
    inline void from_json(const nlohmann::json& j, TickSequence<TickType>& value) {
        j.at("ticks").get_to(value.ticks);
        std::uint64_t raw_flags = 0;
        j.at("flags").get_to(raw_flags);
        value.flags = static_cast<TickUpdateFlags>(raw_flags);
        j.at("symbol_index").get_to(value.symbol_index);
        j.at("provider_index").get_to(value.provider_index);
        j.at("price_digits").get_to(value.price_digits);
        j.at("volume_digits").get_to(value.volume_digits);
    }

#endif // DFH_USE_JSON && DFH_USE_NLOHMANN_JSON

} // namespace dfh

#endif // _DFH_DATA_TICK_SEQUENCE_HPP_INCLUDED
