#pragma once
#ifndef _DFH_DATA_QUOTE_TICK_HPP_INCLUDED
#define _DFH_DATA_QUOTE_TICK_HPP_INCLUDED

/// \file QuoteTick.hpp
/// \brief Defines the QuoteTick structure for tick data without volume.

namespace dfh {

    /// \struct QuoteTick
    /// \brief Упрощённый тик с bid/ask и отметкой времени.
    struct QuoteTick {
        double ask{0.0};           ///< Цена спроса.
        double bid{0.0};           ///< Цена предложения.
        std::uint64_t time_ms{0};  ///< Отметка времени тика в миллисекундах.
        /// \brief Конструктор задаёт цены и отметку времени.
        /// \param a Цена спроса.
        /// \param b Цена предложения.
        /// \param ts Биржевой timestamp в миллисекундах.
        constexpr QuoteTick(double a, double b, std::uint64_t ts) noexcept
            : ask(a), bid(b), time_ms(ts) {}
    };

    static_assert(std::is_trivially_copyable_v<QuoteTick>,
                  "QuoteTick must remain trivially copyable for zero-copy transports.");
    static_assert(sizeof(QuoteTick) == 2 * sizeof(double) + sizeof(std::uint64_t),
                  "QuoteTick layout changed unexpectedly.");

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Сериализует QuoteTick в JSON.
    /// \param j JSON destination.
    /// \param tick Tick to serialize.
    inline void to_json(nlohmann::json& j, const QuoteTick& tick) {
        j = nlohmann::json{{"ask", tick.ask}, {"bid", tick.bid}, {"time_ms", tick.time_ms}};
    }

    /// \brief Десериализует QuoteTick из JSON.
    /// \param j JSON source.
    /// \param tick Tick to populate.
    inline void from_json(const nlohmann::json& j, QuoteTick& tick) {
        j.at("ask").get_to(tick.ask);
        j.at("bid").get_to(tick.bid);
        j.at("time_ms").get_to(tick.time_ms);
    }

#endif // DFH_USE_JSON && DFH_USE_NLOHMANN_JSON

} // namespace dfh

#endif // _DFH_DATA_QUOTE_TICK_HPP_INCLUDED
