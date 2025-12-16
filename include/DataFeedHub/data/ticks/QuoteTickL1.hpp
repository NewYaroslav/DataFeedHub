#pragma once
#ifndef _DFH_DATA_QUOTE_TICK_L1_HPP_INCLUDED
#define _DFH_DATA_QUOTE_TICK_L1_HPP_INCLUDED

/// \file QuoteTickL1.hpp
/// \brief Defines the QuoteTickL1 structure for L1 quote ticks with bid/ask volumes.

namespace dfh {

    /// \brief L1 quote tick with bid/ask prices and volumes
    struct QuoteTickL1 {
        double ask{0.0};             ///< Цена лучшего ask.
        double bid{0.0};             ///< Цена лучшего bid.
        double ask_volume{0.0};      ///< Объём по ask.
        double bid_volume{0.0};      ///< Объём по bid.
        std::uint64_t time_ms{0};    ///< Отметка времени тика в миллисекундах.

        /// \brief Конструктор по умолчанию обнуляет все поля.
        constexpr QuoteTickL1() noexcept = default;

        /// \brief Конструктор задаёт цены, объёмы и отметку времени.
        /// \param a Лучший ask.
        /// \param b Лучший bid.
        /// \param av Объём по ask.
        /// \param bv Объём по bid.
        /// \param ts Отметка времени тика в миллисекундах.
        constexpr QuoteTickL1(double a,
                              double b,
                              double av,
                              double bv,
                              std::uint64_t ts) noexcept
            : ask(a)
            , bid(b)
            , ask_volume(av)
            , bid_volume(bv)
            , time_ms(ts) {}
    };

    static_assert(std::is_trivially_copyable_v<QuoteTickL1>,
                  "QuoteTickL1 must remain trivially copyable.");
    static_assert(sizeof(QuoteTickL1) == 4 * sizeof(double) + sizeof(std::uint64_t),
                  "QuoteTickL1 layout changed unexpectedly.");

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Сериализует QuoteTickL1 в JSON.
    inline void to_json(nlohmann::json& j, const QuoteTickL1& tick) {
        j = nlohmann::json{
            {"ask", tick.ask},
            {"bid", tick.bid},
            {"ask_volume", tick.ask_volume},
            {"bid_volume", tick.bid_volume},
            {"time_ms", tick.time_ms}
        };
    }

    /// \brief Десериализует QuoteTickL1 из JSON.
    inline void from_json(const nlohmann::json& j, QuoteTickL1& tick) {
        j.at("ask").get_to(tick.ask);
        j.at("bid").get_to(tick.bid);
        j.at("ask_volume").get_to(tick.ask_volume);
        j.at("bid_volume").get_to(tick.bid_volume);
        j.at("time_ms").get_to(tick.time_ms);
    }

#endif // DFH_USE_JSON && DFH_USE_NLOHMANN_JSON

} // namespace dfh

#endif // _DFH_DATA_QUOTE_TICK_L1_HPP_INCLUDED
