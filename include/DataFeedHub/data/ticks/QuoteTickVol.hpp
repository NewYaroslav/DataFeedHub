#pragma once
#ifndef _DFH_DATA_QUOTE_TICK_VOL_HPP_INCLUDED
#define _DFH_DATA_QUOTE_TICK_VOL_HPP_INCLUDED

/// \file QuoteTickVol.hpp
/// \brief Defines QuoteTickVol structure for quote ticks with single volume value.

namespace dfh {

    /// \struct QuoteTickVol
    /// \brief Quote tick with bid/ask, a single provider-specific volume, and timestamps.
    struct QuoteTickVol {
        double ask{0.0};                 ///< Цена спроса.
        double bid{0.0};                 ///< Цена предложения.
        double volume{0.0};              ///< Провайдер-специфичный объём.
        std::uint64_t time_ms{0};        ///< Биржевой timestamp в миллисекундах.

        /// \brief Конструктор задаёт цены, объём и метку времени.
        /// \param a Цена ask.
        /// \param b Цена bid.
        /// \param v Поставщик-специфичный объём.
        /// \param ts Биржевой timestamp в миллисекундах.
        constexpr QuoteTickVol(double a,
                               double b,
                               double v,
                               std::uint64_t ts) noexcept
            : ask(a)
            , bid(b)
            , volume(v)
            , time_ms(ts) {}
    };

    static_assert(std::is_trivially_copyable_v<QuoteTickVol>,
                  "QuoteTickVol must remain trivially copyable for zero-copy transports.");
    static_assert(sizeof(QuoteTickVol) == 3 * sizeof(double) + sizeof(std::uint64_t),
                  "QuoteTickVol layout changed unexpectedly.");

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Сериализует QuoteTickVol в JSON.
    /// \param j JSON destination.
    /// \param tick Tick to serialize.
    inline void to_json(nlohmann::json& j, const QuoteTickVol& tick) {
        j = nlohmann::json{{"ask", tick.ask},
                           {"bid", tick.bid},
                           {"volume", tick.volume},
                           {"time_ms", tick.time_ms}};
    }

    /// \brief Десериализует QuoteTickVol из JSON.
    /// \param j JSON source.
    /// \param tick Tick to populate.
    inline void from_json(const nlohmann::json& j, QuoteTickVol& tick) {
        j.at("ask").get_to(tick.ask);
        j.at("bid").get_to(tick.bid);
        j.at("volume").get_to(tick.volume);
        j.at("time_ms").get_to(tick.time_ms);
    }

#endif // DFH_USE_JSON && DFH_USE_NLOHMANN_JSON

} // namespace dfh

#endif // _DFH_DATA_QUOTE_TICK_VOL_HPP_INCLUDED
