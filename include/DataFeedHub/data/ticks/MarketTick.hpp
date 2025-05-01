#pragma once
#ifndef _DFH_DATA_MARKET_TICK_HPP_INCLUDED
#define _DFH_DATA_MARKET_TICK_HPP_INCLUDED

/// \file MarketTick.hpp
/// \brief Defines the MarketTick structure and TickUpdateFlags for detailed market ticks.

namespace dfh {

    /// \struct MarketTick
    /// \brief Represents a single market tick with time, price, volume, and update flags.
    struct MarketTick {
        uint64_t time_ms;        ///< Timestamp of the tick (ms since Unix epoch)
        uint64_t received_ms;    ///< Timestamp when the tick was received (optional)
        double ask;              ///< Best ask price
        double bid;              ///< Best bid price
        double last;             ///< Last trade price
        double volume;           ///< Trade volume in base asset (optional)
        TickUpdateFlags flags;   ///< Flags indicating which fields were updated (optional)

        /// \brief Default constructor. Initializes all fields to zero or NONE.
        MarketTick()
            : time_ms(0), received_ms(0),
              ask(0.0), bid(0.0), last(0.0), volume(0.0),
              flags(TickUpdateFlags::NONE) {}

		/// \brief Full constructor for MarketTick.
		/// \param time_ms Timestamp of the tick (in ms since Unix epoch).
		/// \param received_ms Timestamp when the tick was received (optional, ms since Unix epoch).
		/// \param ask Best ask price at the time of the tick.
		/// \param bid Best bid price at the time of the tick.
		/// \param last Last trade price at the time of the tick.
		/// \param volume Trade volume in base asset units (optional).
		/// \param flags Update flags indicating which fields were changed.
        MarketTick(
            uint64_t time_ms,
            uint64_t received_ms,
            double ask,
            double bid,
            double last,
            double volume,
            TickUpdateFlags flags)
            : time_ms(time_ms), received_ms(received_ms),
              ask(ask), bid(bid), last(last), volume(volume), flags(flags) {}

        /// \brief Sets a specific flag in the tick's update flags.
		/// \param flag The flag to set.
        void set_flag(TickUpdateFlags flag) {
            flags |= flag;
        }

		/// \brief Conditionally sets or clears a flag in the tick's update flags.
		/// \param flag The flag to modify.
		/// \param value True to set the flag, false to clear it.
        void set_flag(TickUpdateFlags flag, bool value) {
            if (value) flags |= flag;
            else flags &= ~flag;
        }

        /// \brief Checks if a specific update flag is set.
        /// \param flag The flag to check.
        /// \return True if the flag is set, false otherwise.
        bool has_flag(TickUpdateFlags flag) const {
            return (flags & flag) != 0;
        }
    };

    /// \brief Serializes a MarketTick to JSON.
    /// \param j JSON object to populate.
    /// \param tick MarketTick instance to serialize.
	/// \details Fields with default values (received_ms=0, volume=0.0, flags=NONE) are omitted to minimize JSON size.
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
            j["flags"] = static_cast<uint64_t>(tick.flags);
        }
    }

    /// \brief Deserializes a MarketTick from JSON.
    /// \param j JSON object containing the serialized tick.
    /// \param tick MarketTick instance to populate.
    inline void from_json(const nlohmann::json& j, MarketTick& tick) {
        j.at("time_ms").get_to(tick.time_ms);
        j.at("ask").get_to(tick.ask);
        j.at("bid").get_to(tick.bid);
        j.at("last").get_to(tick.last);

        if (j.contains("received_ms")) {
            j.at("received_ms").get_to(tick.received_ms);
        } else {
            tick.received_ms = 0;
        }

        if (j.contains("volume")) {
            j.at("volume").get_to(tick.volume);
        } else {
            tick.volume = 0.0;
        }

        if (j.contains("flags")) {
            uint64_t raw_flags = 0;
            j.at("flags").get_to(raw_flags);
            tick.flags = static_cast<TickUpdateFlags>(raw_flags);
        } else {
            tick.flags = TickUpdateFlags::NONE;
        }
    }

} // namespace dfh

/// \brief Specializations for serializing containers of MarketTick
namespace nlohmann {

    template<>
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

    template<>
    struct adl_serializer<std::deque<dfh::MarketTick>> {
        static void to_json(json& j, const std::deque<dfh::MarketTick>& deque_) {
            j = json::array();
            for (const auto& tick : deque_) {
                j.push_back(tick);
            }
        }

        static void from_json(const json& j, std::deque<dfh::MarketTick>& deque_) {
            deque_.clear();
            for (const auto& item : j) {
                deque_.push_back(item.get<dfh::MarketTick>());
            }
        }
    };

    template<>
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

    template<>
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

    template<>
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

#endif // _DFH_DATA_MARKET_TICK_HPP_INCLUDED