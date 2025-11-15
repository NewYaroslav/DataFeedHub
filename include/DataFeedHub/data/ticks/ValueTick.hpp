#pragma once
#ifndef _DFH_DATA_VALUE_TICK_HPP_INCLUDED
#define _DFH_DATA_VALUE_TICK_HPP_INCLUDED

/// \file ValueTick.hpp
/// \brief Defines the ValueTick structure for simplified tick data.

namespace dfh {

    /// \struct ValueTick
    /// \brief Tick that stores a single numeric value and the originating timestamp.
    struct ValueTick {
        double value{0.0};        ///< Single value (e.g., price or indicator).
        std::uint64_t time_ms{0}; ///< Tick timestamp in milliseconds.

        /// \brief Constructor to initialize the tick.
        /// \param v Single value (e.g., price or indicator).
        /// \param ts Tick timestamp in milliseconds.
        constexpr ValueTick(double v, std::uint64_t ts) noexcept : value(v), time_ms(ts) {}
    };

    static_assert(std::is_trivially_copyable_v<ValueTick>,
                  "ValueTick must remain trivially copyable.");

#if defined(DFH_USE_JSON) && defined(DFH_USE_NLOHMANN_JSON)

    /// \brief Serializes ValueTick to JSON.
    /// \param j JSON destination.
    /// \param tick Tick to serialize.
    inline void to_json(nlohmann::json& j, const ValueTick& tick) {
        j = nlohmann::json{{"value", tick.value}, {"time_ms", tick.time_ms}};
    }

    /// \brief Deserializes ValueTick from JSON.
    /// \param j JSON source.
    /// \param tick Tick to populate.
    inline void from_json(const nlohmann::json& j, ValueTick& tick) {
        j.at("value").get_to(tick.value);
        j.at("time_ms").get_to(tick.time_ms);
    }

#endif // DFH_USE_JSON && DFH_USE_NLOHMANN_JSON

} // namespace dfh

#endif // _DFH_DATA_VALUE_TICK_HPP_INCLUDED
