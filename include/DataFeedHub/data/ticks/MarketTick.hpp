#pragma once
#ifndef _DFH_DATA_MARKET_TICK_HPP_INCLUDED
#define _DFH_DATA_MARKET_TICK_HPP_INCLUDED

/// \file MarketTick.hpp
/// \brief Defines the MarketTick structure and helpers for JSON serialization.

namespace dfh {

    /// \struct MarketTick
    /// \brief Represents a single market tick with time, price, volume, and update flags.
    struct MarketTick {
        std::uint64_t time_ms{0};      ///< Timestamp of the tick (ms since Unix epoch).
        std::uint64_t received_ms{0};  ///< Timestamp when the tick was received (ms since Unix epoch).
        double ask{0.0};               ///< Best ask price.
        double bid{0.0};               ///< Best bid price.
        double last{0.0};              ///< Last trade price.
        double volume{0.0};            ///< Trade volume in base asset (optional).
        TickUpdateFlags flags{TickUpdateFlags::NONE}; ///< Flags indicating which fields were updated.

        /// \brief Default constructor.
        constexpr MarketTick() noexcept = default;

        /// \brief Full constructor for MarketTick.
        /// \param time Timestamp of the tick (in ms since Unix epoch).
        /// \param received Timestamp when the tick was received (ms since Unix epoch).
        /// \param ask_price Best ask price at the time of the tick.
        /// \param bid_price Best bid price at the time of the tick.
        /// \param last_price Last trade price at the time of the tick.
        /// \param volume_value Trade volume in base asset units.
        /// \param flag_mask Update flags indicating which fields were changed.
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

        /// \brief Sets a specific flag in the tick's update flags.
        /// \param flag The flag to set.
        constexpr void set_flag(TickUpdateFlags flag) noexcept { flags |= flag; }

        /// \brief Conditionally sets or clears a flag in the tick's update flags.
        /// \param flag The flag to modify.
        /// \param value True to set the flag, false to clear it.
        constexpr void set_flag(TickUpdateFlags flag, bool value) noexcept {
            if (value) flags |= flag;
            else flags &= ~flag;
        }

        /// \brief Checks if a specific update flag is set.
        /// \param flag The flag to check.
        /// \return True if the flag is set, false otherwise.
        [[nodiscard]] constexpr bool has_flag(TickUpdateFlags flag) const noexcept {
            return (flags & flag) != TickUpdateFlags::NONE;
        }
    };

    static_assert(std::is_trivially_copyable_v<MarketTick>,
                  "MarketTick must remain trivially copyable for ring buffers.");

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Serializes a MarketTick to JSON.
    /// \param j JSON object to populate.
    /// \param tick MarketTick instance to serialize.
    /// \details Fields with default values (received_ms=0, volume=0.0, flags=NONE) are omitted.
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

    /// \brief Deserializes a MarketTick from JSON.
    /// \param j JSON object containing the serialized tick.
    /// \param tick MarketTick instance to populate.
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

/// \brief Specializations for serializing containers of MarketTick.
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
