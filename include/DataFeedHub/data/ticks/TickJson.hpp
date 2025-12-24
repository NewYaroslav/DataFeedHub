#pragma once
#ifndef _DFH_DATA_TICK_JSON_HPP_INCLUDED
#define _DFH_DATA_TICK_JSON_HPP_INCLUDED

/// \file TickJson.hpp
/// \brief JSON (де)сериализация для DTO тиков.

#include <cstddef>
#include <cstdint>
#include <deque>
#include <list>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "DataFeedHub/data/ticks/BidAskRestoreConfig.hpp"
#include "DataFeedHub/data/ticks/MarketTick.hpp"
#include "DataFeedHub/data/ticks/QuoteTick.hpp"
#include "DataFeedHub/data/ticks/QuoteTickVol.hpp"
#include "DataFeedHub/data/ticks/QuoteTickL1.hpp"
#include "DataFeedHub/data/ticks/TradeTick.hpp"
#include "DataFeedHub/data/ticks/TickCodecConfig.hpp"
#include "DataFeedHub/data/ticks/TickMetadata.hpp"
#include "DataFeedHub/data/ticks/TickSequence.hpp"
#include "DataFeedHub/data/ticks/SingleTick.hpp"
#include "DataFeedHub/data/ticks/ValueTick.hpp"

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)
#include <nlohmann/json.hpp>
#endif

namespace dfh {

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Сериализует MarketTick в JSON.
    /// \param j Объект JSON, который заполняется.
    /// \param tick Тик для сериализации.
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

namespace dfh {

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Сериализует QuoteTick в JSON.
    /// \param j Объект JSON, который будет заполнен.
    /// \param tick Тик для сериализации.
    inline void to_json(nlohmann::json& j, const QuoteTick& tick) {
        j = nlohmann::json{{"ask", tick.ask}, {"bid", tick.bid}, {"time_ms", tick.time_ms}};
        if (tick.received_ms != 0) {
            j["received_ms"] = tick.received_ms;
        }
    }

    /// \brief Десериализует QuoteTick из JSON.
    /// \param j JSON-источник с описанием тика.
    /// \param tick Структура для заполнения.
    inline void from_json(const nlohmann::json& j, QuoteTick& tick) {
        j.at("ask").get_to(tick.ask);
        j.at("bid").get_to(tick.bid);
        j.at("time_ms").get_to(tick.time_ms);
        tick.received_ms = j.value("received_ms", std::uint64_t{0});
    }

    /// \brief Сериализует QuoteTickVol в JSON.
    /// \param j Объект JSON, который будет заполнен.
    /// \param tick Тик для сериализации.
    inline void to_json(nlohmann::json& j, const QuoteTickVol& tick) {
        j = nlohmann::json{{"ask", tick.ask},
                           {"bid", tick.bid},
                           {"volume", tick.volume},
                           {"time_ms", tick.time_ms}};
        if (tick.received_ms != 0) {
            j["received_ms"] = tick.received_ms;
        }
    }

    /// \brief Десериализует QuoteTickVol из JSON.
    /// \param j JSON-источник с описанием тика.
    /// \param tick Структура для заполнения.
    inline void from_json(const nlohmann::json& j, QuoteTickVol& tick) {
        j.at("ask").get_to(tick.ask);
        j.at("bid").get_to(tick.bid);
        j.at("volume").get_to(tick.volume);
        j.at("time_ms").get_to(tick.time_ms);
        tick.received_ms = j.value("received_ms", std::uint64_t{0});
    }

    /// \brief Сериализует QuoteTickL1 в JSON.
    inline void to_json(nlohmann::json& j, const QuoteTickL1& tick) {
        j = nlohmann::json{
            {"ask", tick.ask},
            {"bid", tick.bid},
            {"ask_volume", tick.ask_volume},
            {"bid_volume", tick.bid_volume},
            {"time_ms", tick.time_ms}
        };
        if (tick.received_ms != 0) {
            j["received_ms"] = tick.received_ms;
        }
    }

    /// \brief Десериализует QuoteTickL1 из JSON.
    inline void from_json(const nlohmann::json& j, QuoteTickL1& tick) {
        j.at("ask").get_to(tick.ask);
        j.at("bid").get_to(tick.bid);
        j.at("ask_volume").get_to(tick.ask_volume);
        j.at("bid_volume").get_to(tick.bid_volume);
        j.at("time_ms").get_to(tick.time_ms);
        tick.received_ms = j.value("received_ms", std::uint64_t{0});
    }

    /// \brief Сериализует TradeTick в JSON.
    /// \param j Объект JSON, который будет заполнен.
    /// \param tick Сделка для сериализации.
    inline void to_json(nlohmann::json& j, const TradeTick& tick) {
        j = nlohmann::json{
            {"price", tick.price},
            {"volume", tick.volume},
            {"time_ms", tick.time_ms},
            {"trade_id", tick.trade_id()},
            {"side", static_cast<std::uint8_t>(tick.trade_side())}
        };
    }

    /// \brief Десериализует TradeTick из JSON.
    /// \param j JSON-источник с описанием сделки.
    /// \param tick Структура, которая заполняется.
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

    /// \brief Сериализует BidAskRestoreConfig в JSON.
    inline void to_json(nlohmann::json& j, const BidAskRestoreConfig& cfg) {
        j = nlohmann::json{
            {"mode", static_cast<std::uint32_t>(cfg.mode)},
            {"fixed_spread", cfg.fixed_spread},
            {"price_digits", cfg.price_digits}
        };
    }

    /// \brief Десериализует BidAskRestoreConfig из JSON.
    inline void from_json(const nlohmann::json& j, BidAskRestoreConfig& cfg) {
        std::uint32_t raw_mode = 0;
        j.at("mode").get_to(raw_mode);
        cfg.mode = static_cast<BidAskModel>(raw_mode);
        j.at("fixed_spread").get_to(cfg.fixed_spread);
        j.at("price_digits").get_to(cfg.price_digits);
    }

    /// \brief Сериализует TickCodecConfig в JSON.
    inline void to_json(nlohmann::json& j, const TickCodecConfig& cfg) {
        j = nlohmann::json{
            {"tick_size", cfg.tick_size},
            {"expiration_time_ms", cfg.expiration_time_ms},
            {"next_expiration_time_ms", cfg.next_expiration_time_ms},
            {"flags", static_cast<std::uint64_t>(cfg.flags)},
            {"price_digits", cfg.price_digits},
            {"volume_digits", cfg.volume_digits}
        };

        nlohmann::json reserved = nlohmann::json::array();
        for (auto byte : cfg.reserved) {
            reserved.push_back(byte);
        }
        j["reserved"] = std::move(reserved);
    }

    /// \brief Десериализует TickCodecConfig из JSON.
    inline void from_json(const nlohmann::json& j, TickCodecConfig& cfg) {
        j.at("tick_size").get_to(cfg.tick_size);
        j.at("expiration_time_ms").get_to(cfg.expiration_time_ms);
        j.at("next_expiration_time_ms").get_to(cfg.next_expiration_time_ms);
        std::uint64_t raw_flags = 0;
        j.at("flags").get_to(raw_flags);
        cfg.flags = static_cast<TickStorageFlags>(raw_flags);
        j.at("price_digits").get_to(cfg.price_digits);
        j.at("volume_digits").get_to(cfg.volume_digits);

        auto reserved = j.value("reserved", std::vector<std::uint8_t>(cfg.reserved.size(), 0));
        for (std::size_t i = 0; i < cfg.reserved.size(); ++i) {
            cfg.reserved[i] = i < reserved.size() ? reserved[i] : 0;
        }
    }

    /// \brief Сериализует SingleTick в JSON.
    /// \tparam TickType Тип тика.
    /// \param j Объект JSON, который будет заполнен.
    /// \param value Значение SingleTick.
    template<typename TickType>
    inline void to_json(nlohmann::json& j, const SingleTick<TickType>& value) {
        j = nlohmann::json{
            {"tick", value.tick},
            {"flags", static_cast<std::uint64_t>(value.flags)},
            {"symbol_index", value.symbol_index},
            {"provider_index", value.provider_index},
            {"price_digits", value.price_digits},
            {"volume_digits", value.volume_digits}
        };
    }

    /// \brief Десериализует SingleTick из JSON.
    /// \tparam TickType Тип тика.
    /// \param j JSON-источник.
    /// \param value Структура для заполнения.
    template<typename TickType>
    inline void from_json(const nlohmann::json& j, SingleTick<TickType>& value) {
        j.at("tick").get_to(value.tick);
        std::uint64_t raw_flags = 0;
        j.at("flags").get_to(raw_flags);
        value.flags = static_cast<TickUpdateFlags>(raw_flags);
        j.at("symbol_index").get_to(value.symbol_index);
        j.at("provider_index").get_to(value.provider_index);
        j.at("price_digits").get_to(value.price_digits);
        j.at("volume_digits").get_to(value.volume_digits);
    }

    /// \brief Сериализует TickMetadata в JSON.
    inline void to_json(nlohmann::json& j, const TickMetadata& metadata) {
        j = nlohmann::json{
            {"start_time_ms", metadata.start_time_ms},
            {"end_time_ms", metadata.end_time_ms},
            {"expiration_time_ms", metadata.expiration_time_ms},
            {"next_expiration_time_ms", metadata.next_expiration_time_ms},
            {"count", metadata.count},
            {"tick_size", metadata.tick_size},
            {"symbol_id", metadata.symbol_id},
            {"exchange_id", metadata.exchange_id},
            {"market_type", static_cast<std::uint32_t>(metadata.market_type)},
            {"price_digits", metadata.price_digits},
            {"volume_digits", metadata.volume_digits},
            {"flags", static_cast<std::uint64_t>(metadata.flags)}
        };
    }

    /// \brief Десериализует TickMetadata из JSON.
    inline void from_json(const nlohmann::json& j, TickMetadata& metadata) {
        j.at("start_time_ms").get_to(metadata.start_time_ms);
        j.at("end_time_ms").get_to(metadata.end_time_ms);
        j.at("expiration_time_ms").get_to(metadata.expiration_time_ms);
        j.at("next_expiration_time_ms").get_to(metadata.next_expiration_time_ms);
        j.at("count").get_to(metadata.count);
        j.at("tick_size").get_to(metadata.tick_size);
        j.at("symbol_id").get_to(metadata.symbol_id);
        j.at("exchange_id").get_to(metadata.exchange_id);
        std::uint32_t raw_market_type = 0;
        j.at("market_type").get_to(raw_market_type);
        metadata.market_type = static_cast<MarketType>(raw_market_type);
        j.at("price_digits").get_to(metadata.price_digits);
        j.at("volume_digits").get_to(metadata.volume_digits);
        std::uint64_t raw_flags = 0;
        j.at("flags").get_to(raw_flags);
        metadata.flags = static_cast<TickStorageFlags>(raw_flags);
    }

    /// \brief Сериализует TickSequence в JSON.
    /// \tparam TickType Тип тика.
    template<typename TickType>
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
    /// \tparam TickType Тип тика.
    template<typename TickType>
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

    /// \brief Сериализует ValueTick в JSON.
    /// \param j Объект JSON, который будет заполнен.
    /// \param tick Тик для сериализации.
    inline void to_json(nlohmann::json& j, const ValueTick& tick) {
        j = nlohmann::json{{"value", tick.value}, {"time_ms", tick.time_ms}};
    }

    /// \brief Десериализует ValueTick из JSON.
    /// \param j JSON-источник.
    /// \param tick Структура для заполнения.
    inline void from_json(const nlohmann::json& j, ValueTick& tick) {
        j.at("value").get_to(tick.value);
        j.at("time_ms").get_to(tick.time_ms);
    }

#endif // DFH_USE_JSON && DFH_USE_NLOHMANN_JSON

} // namespace dfh

#endif // _DFH_DATA_TICK_JSON_HPP_INCLUDED
