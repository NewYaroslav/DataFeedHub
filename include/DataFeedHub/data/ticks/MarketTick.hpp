#pragma once
#ifndef _DFH_DATA_MARKET_TICK_HPP_INCLUDED
#define _DFH_DATA_MARKET_TICK_HPP_INCLUDED

/// \file MarketTick.hpp
/// \brief Описывает структуру MarketTick из домена data и её JSON-помощники.

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

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Сериализует MarketTick в JSON.
    /// \param j Объект JSON, который заполняется.
    /// \param tick Tick для сериализации.
    /// \details Поля с дефолтными значениями (received_ms=0, volume=0.0, flags=NONE) опускаются.
    inline void to_json(nlohmann::json& j, const MarketTick& tick) {
        j = nlohmann::json{
            {"time_ms", tick.time_ms},
            {"ask", tick.ask},
            {"bid", tick.bid},
            {"last", tick.last}
        };

        if (tick.received_ms != 0) {
            j["received_ms"] = tick.received_ms;
        }
        if (tick.volume != 0.0) {
            j["volume"] = tick.volume;
        }
        if (tick.flags != TickUpdateFlags::NONE) {
            j["flags"] = static_cast<std::uint64_t>(tick.flags);
        }
    }

    /// \brief Десериализует MarketTick из JSON.
    /// \param j JSON-объект с описанием тика.
    /// \param tick Структура, которую нужно заполнить.
    inline void from_json(const nlohmann::json& j, MarketTick& tick) {
        j.at("time_ms").get_to(tick.time_ms);
        j.at("ask").get_to(tick.ask);
        j.at("bid").get_to(tick.bid);
        j.at("last").get_to(tick.last);

        tick.received_ms = j.value("received_ms", std::uint64_t{0});
        tick.volume = j.value("volume", 0.0);

        if (j.contains("flags")) {
            std::uint64_t raw_flags = 0;
            j.at("flags").get_to(raw_flags);
            tick.flags = static_cast<TickUpdateFlags>(raw_flags);
        } else {
            tick.flags = TickUpdateFlags::NONE;
        }
    }

#endif // DFH_USE_JSON && DFH_USE_NLOHMANN_JSON

} // namespace dfh

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

/// \brief Специализации сериализации контейнеров MarketTick.
namespace nlohmann {

    template <>
    struct adl_serializer<std::vector<dfh::MarketTick>> {
        static void to_json(json& j, const std::vector<dfh::MarketTick>& vec) {
            j = json::array();
            for (const auto& tick : vec) {
                j.push_back(tick);
            }
        }

        static void from_json(const json& j, std::vector<dfh::MarketTick>& vec) {
            vec.clear();
            vec.reserve(j.size());
            for (const auto& item : j) {
                vec.push_back(item.get<dfh::MarketTick>());
            }
        }
    };

    template <>
    struct adl_serializer<std::deque<dfh::MarketTick>> {
        static void to_json(json& j, const std::deque<dfh::MarketTick>& deque_) {
            j = json::array();
            for (const auto& tick : deque_) {
                j.push_back(tick);
            }
        }

        static void from_json(const json& j, std::deque<dfh::MarketTick>& deque_) {
            deque_.clear();
            deque_.resize(j.size());
            std::size_t idx = 0;
            for (const auto& item : j) {
                deque_[idx++] = item.get<dfh::MarketTick>();
            }
        }
    };

    template <>
    struct adl_serializer<std::list<dfh::MarketTick>> {
        static void to_json(json& j, const std::list<dfh::MarketTick>& list_) {
            j = json::array();
            for (const auto& tick : list_) {
                j.push_back(tick);
            }
        }

        static void from_json(const json& j, std::list<dfh::MarketTick>& list_) {
            list_.clear();
            for (const auto& item : j) {
                list_.push_back(item.get<dfh::MarketTick>());
            }
        }
    };

    template <>
    struct adl_serializer<std::unordered_map<std::string, dfh::MarketTick>> {
        static void to_json(json& j, const std::unordered_map<std::string, dfh::MarketTick>& map_) {
            j = json::object();
            for (const auto& [key, tick] : map_) {
                j[key] = tick;
            }
        }

        static void from_json(const json& j, std::unordered_map<std::string, dfh::MarketTick>& map_) {
            map_.clear();
            for (auto it = j.begin(); it != j.end(); ++it) {
                map_.emplace(it.key(), it.value().get<dfh::MarketTick>());
            }
        }
    };

    template <>
    struct adl_serializer<std::map<std::string, dfh::MarketTick>> {
        static void to_json(json& j, const std::map<std::string, dfh::MarketTick>& map_) {
            j = json::object();
            for (const auto& [key, tick] : map_) {
                j[key] = tick;
            }
        }

        static void from_json(const json& j, std::map<std::string, dfh::MarketTick>& map_) {
            map_.clear();
            for (auto it = j.begin(); it != j.end(); ++it) {
                map_.emplace(it.key(), it.value().get<dfh::MarketTick>());
            }
        }
    };

} // namespace nlohmann

#endif // DFH_USE_JSON && DFH_USE_NLOHMANN_JSON

#endif // _DFH_DATA_MARKET_TICK_HPP_INCLUDED
