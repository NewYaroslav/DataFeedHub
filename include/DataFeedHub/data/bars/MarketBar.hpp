#pragma once
#ifndef _DTH_DATA_MARKET_BAR_HPP_INCLUDED
#define _DTH_DATA_MARKET_BAR_HPP_INCLUDED

/// \file MarketBar.hpp
/// \brief Defines a compact structure representing a market bar with OHLCV, tick volume, and spread in points.

namespace dfh {

    /// \struct MarketBar
    /// \brief Represents a single market bar with price, volume, spread, and tick volume.
    struct MarketBar {
        uint64_t time_ms;          ///< Start time of the bar (ms since epoch)
        double   open;             ///< Open price
        double   high;             ///< Highest price during the bar
        double   low;              ///< Lowest price during the bar
        double   close;            ///< Close price
        double   volume;           ///< Traded volume during the bar
        double   quote_volume;     ///< Quote volume during the bar
        double   buy_volume;       ///< Taker buy volume (in base units)
        double   buy_quote_volume; ///< Taker buy volume (in quote units)
        uint32_t spread;           ///< Spread in points (at bar close)
        uint32_t tick_volume;      ///< Number of price updates during the bar

        /// \brief Default constructor. Initializes all fields to zero.
        MarketBar()
            : time_ms(0), open(0.0), high(0.0), low(0.0), close(0.0),
              volume(0.0), quote_volume(0.0), buy_volume(0.0), buy_quote_volume(0.0),
              spread(0), tick_volume(0) {}

        /// \brief Constructs a bar with specified field values.
        /// \param time_ms Start time of the bar (in ms since Unix epoch).
        /// \param open Open price.
        /// \param high High price.
        /// \param low Low price.
        /// \param close Close price.
        /// \param volume Traded volume.
        /// \param quote_volume Quote volume.
        /// \param buy_volume Taker buy volume.
        /// \param buy_quote_volume Taker buy quote volume.
        /// \param spread Spread in points.
        /// \param tick_volume Number of price updates.
        MarketBar(
            uint64_t time_ms,
            double open,
            double high,
            double low,
            double close,
            double volume,
            double quote_volume,
            double buy_volume,
            double buy_quote_volume,
            uint32_t spread,
            uint32_t tick_volume)
            : time_ms(time_ms),
              open(open), high(high), low(low), close(close),
              volume(volume), quote_volume(quote_volume),
              buy_volume(buy_volume), buy_quote_volume(buy_quote_volume),
              spread(spread), tick_volume(tick_volume) {}
    };
	
    /// \brief Serializes a MarketBar to JSON.
    /// \param j Output JSON object.
    /// \param bar MarketBar instance to serialize.
    /// \details Fields with zero values for volume-related data are omitted to reduce JSON size.
    inline void to_json(nlohmann::json& j, const dfh::MarketBar& bar) {
        j = nlohmann::json{
            {"time_ms", bar.time_ms},
            {"open", bar.open},
            {"high", bar.high},
            {"low", bar.low},
            {"close", bar.close},
            {"spread", bar.spread},
            {"tick_volume", bar.tick_volume}
        };

        if (bar.volume != 0.0) {
            j["volume"] = bar.volume;
        }
        if (bar.quote_volume != 0.0) {
            j["quote_volume"] = bar.quote_volume;
        }
        if (bar.buy_volume != 0.0) {
            j["buy_volume"] = bar.buy_volume;
        }
        if (bar.buy_quote_volume != 0.0) {
            j["buy_quote_volume"] = bar.buy_quote_volume;
        }
    }

    /// \brief Deserializes a MarketBar from JSON.
    /// \param j Input JSON object.
    /// \param bar MarketBar instance to populate.
    /// \details Missing volume-related fields are treated as zero.
    inline void from_json(const nlohmann::json& j, dfh::MarketBar& bar) {
        j.at("time_ms").get_to(bar.time_ms);
        j.at("open").get_to(bar.open);
        j.at("high").get_to(bar.high);
        j.at("low").get_to(bar.low);
        j.at("close").get_to(bar.close);
        j.at("spread").get_to(bar.spread);
        j.at("tick_volume").get_to(bar.tick_volume);

        if (j.contains("volume")) {
            j.at("volume").get_to(bar.volume);
        } else {
            bar.volume = 0.0;
        }

        if (j.contains("quote_volume")) {
            j.at("quote_volume").get_to(bar.quote_volume);
        } else {
            bar.quote_volume = 0.0;
        }

        if (j.contains("buy_volume")) {
            j.at("buy_volume").get_to(bar.buy_volume);
        } else {
            bar.buy_volume = 0.0;
        }

        if (j.contains("buy_quote_volume")) {
            j.at("buy_quote_volume").get_to(bar.buy_quote_volume);
        } else {
            bar.buy_quote_volume = 0.0;
        }
    }

} // namespace dfh

/// \brief Specializations for serializing containers of MarketBar
namespace nlohmann {

    template<>
    struct adl_serializer<std::vector<dfh::MarketBar>> {
        static void to_json(json& j, const std::vector<dfh::MarketBar>& vec) {
            j = json::array();
            for (const auto& tick : vec) {
                j.push_back(tick);
            }
        }

        static void from_json(const json& j, std::vector<dfh::MarketBar>& vec) {
            vec.clear();
            vec.reserve(j.size());
            for (const auto& item : j) {
                vec.push_back(item.get<dfh::MarketBar>());
            }
        }
    };

    template<>
    struct adl_serializer<std::deque<dfh::MarketBar>> {
        static void to_json(json& j, const std::deque<dfh::MarketBar>& deque_) {
            j = json::array();
            for (const auto& tick : deque_) {
                j.push_back(tick);
            }
        }

        static void from_json(const json& j, std::deque<dfh::MarketBar>& deque_) {
            deque_.clear();
            for (const auto& item : j) {
                deque_.push_back(item.get<dfh::MarketBar>());
            }
        }
    };

    template<>
    struct adl_serializer<std::list<dfh::MarketBar>> {
        static void to_json(json& j, const std::list<dfh::MarketBar>& list_) {
            j = json::array();
            for (const auto& tick : list_) {
                j.push_back(tick);
            }
        }

        static void from_json(const json& j, std::list<dfh::MarketBar>& list_) {
            list_.clear();
            for (const auto& item : j) {
                list_.push_back(item.get<dfh::MarketBar>());
            }
        }
    };

    template<>
    struct adl_serializer<std::unordered_map<std::string, dfh::MarketBar>> {
        static void to_json(json& j, const std::unordered_map<std::string, dfh::MarketBar>& map_) {
            j = json::object();
            for (const auto& [key, tick] : map_) {
                j[key] = tick;
            }
        }

        static void from_json(const json& j, std::unordered_map<std::string, dfh::MarketBar>& map_) {
            map_.clear();
            for (auto it = j.begin(); it != j.end(); ++it) {
                map_.emplace(it.key(), it.value().get<dfh::MarketBar>());
            }
        }
    };

    template<>
    struct adl_serializer<std::map<std::string, dfh::MarketBar>> {
        static void to_json(json& j, const std::map<std::string, dfh::MarketBar>& map_) {
            j = json::object();
            for (const auto& [key, tick] : map_) {
                j[key] = tick;
            }
        }

        static void from_json(const json& j, std::map<std::string, dfh::MarketBar>& map_) {
            map_.clear();
            for (auto it = j.begin(); it != j.end(); ++it) {
                map_.emplace(it.key(), it.value().get<dfh::MarketBar>());
            }
        }
    };

} // namespace nlohmann

#endif // _DTH_DATA_MARKET_BAR_HPP_INCLUDED
