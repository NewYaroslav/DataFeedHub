#pragma once
#ifndef _DFH_DATA_СOMMON_ENUMS_HPP_INCLUDED
#define _DFH_DATA_СOMMON_ENUMS_HPP_INCLUDED

/// \file enums.hpp
/// \brief Enumerations for trading parameters.

#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <nlohmann/json.hpp>

namespace dfh {

    /// \enum MarketType
    /// \brief Represents the type of trading market (spot, futures, options, etc.)
    enum class MarketType : uint8_t {
        UNKNOWN = 0,                ///< Unspecified or unrecognized market type
        SPOT,                       ///< Spot market (immediate delivery)
        FUTURES_PERPETUAL_LINEAR,   ///< Perpetual futures with linear settlement
        FUTURES_PERPETUAL_INVERSE,  ///< Perpetual futures with inverse settlement
        FUTURES_DATED_LINEAR,       ///< Dated futures with linear settlement (fixed expiration)
        FUTURES_DATED_INVERSE,      ///< Dated futures with inverse settlement (fixed expiration)
        OPTIONS_LINEAR,             ///< Options settled in quote currency
        OPTIONS_INVERSE             ///< Options settled in base currency
    };

    /// \brief Converts MarketType to its string representation.
    inline const std::string& to_str(MarketType type) noexcept {
        static const std::vector<std::string> str_data = {
            "UNKNOWN",
            "SPOT",
            "FUTURES_PERPETUAL_LINEAR",
            "FUTURES_PERPETUAL_INVERSE",
            "FUTURES_DATED_LINEAR",
            "FUTURES_DATED_INVERSE",
            "OPTIONS_LINEAR",
            "OPTIONS_INVERSE"
        };
        return str_data[static_cast<size_t>(type)];
    }

    /// \brief Parses a string into a MarketType value.
    /// \param str Input string.
    /// \param type Output MarketType.
    /// \return True if parsing was successful, false otherwise.
    inline bool to_enum(const std::string& str, MarketType& type) noexcept {
        static const std::unordered_map<std::string, MarketType> str_map = {
            {"UNKNOWN", MarketType::UNKNOWN},
            {"SPOT", MarketType::SPOT},
            {"FUTURES_PERPETUAL_LINEAR", MarketType::FUTURES_PERPETUAL_LINEAR},
            {"FUTURES_PERPETUAL_INVERSE", MarketType::FUTURES_PERPETUAL_INVERSE},
            {"FUTURES_DATED_LINEAR", MarketType::FUTURES_DATED_LINEAR},
            {"FUTURES_DATED_INVERSE", MarketType::FUTURES_DATED_INVERSE},
            {"OPTIONS_LINEAR", MarketType::OPTIONS_LINEAR},
            {"OPTIONS_INVERSE", MarketType::OPTIONS_INVERSE}
        };
        auto it = str_map.find(str);
        if (it != str_map.end()) {
            type = it->second;
            return true;
        }
        return false;
    }

    /// \brief Serializes MarketType to JSON.
    inline void to_json(nlohmann::json& j, const MarketType& type) {
        j = to_str(type);
    }

    /// \brief Deserializes MarketType from JSON.
    inline void from_json(const nlohmann::json& j, MarketType& type) {
        if (!to_enum(j.get<std::string>(), type)) {
            type = MarketType::UNKNOWN;
        }
    }

    /// \brief Output stream operator for MarketType.
    inline std::ostream& operator<<(std::ostream& os, MarketType type) {
        os << to_str(type);
        return os;
    }

//------------------------------------------------------------------------------

    /// \enum ExchangeType
    /// \brief Enumeration of trading exchanges and venues (crypto, forex, traditional).
    enum class ExchangeType {
        UNKNOWN = 0,         ///< Unknown or unsupported exchange.
        METATRADER,          ///< MetaTrader platform connector (MT4/MT5)

        // Rating 2 exchanges
        BINANCE,
        BITSTAMP,
        BYBIT,
        COINBASE,
        ECB,
        GATE_IO,
        GEMINI,
        KRAKEN,
        LMAX_DIGITAL,

        // Rating 1 exchanges
        BIBOX,
        BINANCE_US,
        BITBAY,
        BITFINEX,
        BITHUMB,
        BITMEX,
        BITTREX,
        BTC_CHINA,
        BTC_TRADE,
        COINONE,
        DERIBIT,
        HITBTC,
        HUOBI,
        HUOBI_PRO,
        ITBIT,
        KUCOIN,
        KUCOIN_FTS,
        OKCOIN_CNY,
        OKCOIN_USD,
        OKEX,
        POLONIEX,
        UPBIT
    };

    /// \brief Converts ExchangeType to its string representation.
    inline const std::string& to_str(ExchangeType value) noexcept {
        static const std::vector<std::string> str_data = {
            "UNKNOWN",
            "METATRADER",
            "BINANCE",
            "BITSTAMP",
            "BYBIT",
            "COINBASE",
            "ECB",
            "GATE_IO",
            "GEMINI",
            "KRAKEN",
            "LMAX_DIGITAL",
            "BIBOX",
            "BINANCE_US",
            "BITBAY",
            "BITFINEX",
            "BITHUMB",
            "BITMEX",
            "BITTREX",
            "BTC_CHINA",
            "BTC_TRADE",
            "COINONE",
            "DERIBIT",
            "HITBTC",
            "HUOBI",
            "HUOBI_PRO",
            "ITBIT",
            "KUCOIN",
            "KUCOIN_FTS",
            "OKCOIN_CNY",
            "OKCOIN_USD",
            "OKEX",
            "POLONIEX",
            "UPBIT"
        };
        return str_data[static_cast<size_t>(value)];
    }

    /// \brief Returns the string view representation of ExchangeType.
    constexpr std::string_view to_str_view(ExchangeType value) noexcept {
        switch (value) {
            case ExchangeType::UNKNOWN:        return "UNKNOWN";
            case ExchangeType::METATRADER:     return "METATRADER";
            case ExchangeType::BINANCE:        return "BINANCE";
            case ExchangeType::BITSTAMP:       return "BITSTAMP";
            case ExchangeType::BYBIT:          return "BYBIT";
            case ExchangeType::COINBASE:       return "COINBASE";
            case ExchangeType::ECB:            return "ECB";
            case ExchangeType::GATE_IO:        return "GATE_IO";
            case ExchangeType::GEMINI:         return "GEMINI";
            case ExchangeType::KRAKEN:         return "KRAKEN";
            case ExchangeType::LMAX_DIGITAL:   return "LMAX_DIGITAL";
            case ExchangeType::BIBOX:          return "BIBOX";
            case ExchangeType::BINANCE_US:     return "BINANCE_US";
            case ExchangeType::BITBAY:         return "BITBAY";
            case ExchangeType::BITFINEX:       return "BITFINEX";
            case ExchangeType::BITHUMB:        return "BITHUMB";
            case ExchangeType::BITMEX:         return "BITMEX";
            case ExchangeType::BITTREX:        return "BITTREX";
            case ExchangeType::BTC_CHINA:      return "BTC_CHINA";
            case ExchangeType::BTC_TRADE:      return "BTC_TRADE";
            case ExchangeType::COINONE:        return "COINONE";
            case ExchangeType::DERIBIT:        return "DERIBIT";
            case ExchangeType::HITBTC:         return "HITBTC";
            case ExchangeType::HUOBI:          return "HUOBI";
            case ExchangeType::HUOBI_PRO:      return "HUOBI_PRO";
            case ExchangeType::ITBIT:          return "ITBIT";
            case ExchangeType::KUCOIN:         return "KUCOIN";
            case ExchangeType::KUCOIN_FTS:     return "KUCOIN_FTS";
            case ExchangeType::OKCOIN_CNY:     return "OKCOIN_CNY";
            case ExchangeType::OKCOIN_USD:     return "OKCOIN_USD";
            case ExchangeType::OKEX:           return "OKEX";
            case ExchangeType::POLONIEX:       return "POLONIEX";
            case ExchangeType::UPBIT:          return "UPBIT";
        }
        return "UNKNOWN";
    }

    /// \brief Parses a string into an ExchangeType enum.
    inline bool to_enum(const std::string& str, ExchangeType& value) noexcept {
        static const std::unordered_map<std::string, ExchangeType> str_map = {
            {"UNKNOWN", ExchangeType::UNKNOWN},
            {"METATRADER", ExchangeType::METATRADER},
            {"BINANCE", ExchangeType::BINANCE},
            {"BITSTAMP", ExchangeType::BITSTAMP},
            {"BYBIT", ExchangeType::BYBIT},
            {"COINBASE", ExchangeType::COINBASE},
            {"ECB", ExchangeType::ECB},
            {"GATE_IO", ExchangeType::GATE_IO},
            {"GEMINI", ExchangeType::GEMINI},
            {"KRAKEN", ExchangeType::KRAKEN},
            {"LMAX_DIGITAL", ExchangeType::LMAX_DIGITAL},
            {"BIBOX", ExchangeType::BIBOX},
            {"BINANCE_US", ExchangeType::BINANCE_US},
            {"BITBAY", ExchangeType::BITBAY},
            {"BITFINEX", ExchangeType::BITFINEX},
            {"BITHUMB", ExchangeType::BITHUMB},
            {"BITMEX", ExchangeType::BITMEX},
            {"BITTREX", ExchangeType::BITTREX},
            {"BTC_CHINA", ExchangeType::BTC_CHINA},
            {"BTC_TRADE", ExchangeType::BTC_TRADE},
            {"COINONE", ExchangeType::COINONE},
            {"DERIBIT", ExchangeType::DERIBIT},
            {"HITBTC", ExchangeType::HITBTC},
            {"HUOBI", ExchangeType::HUOBI},
            {"HUOBI_PRO", ExchangeType::HUOBI_PRO},
            {"ITBIT", ExchangeType::ITBIT},
            {"KUCOIN", ExchangeType::KUCOIN},
            {"KUCOIN_FTS", ExchangeType::KUCOIN_FTS},
            {"OKCOIN_CNY", ExchangeType::OKCOIN_CNY},
            {"OKCOIN_USD", ExchangeType::OKCOIN_USD},
            {"OKEX", ExchangeType::OKEX},
            {"POLONIEX", ExchangeType::POLONIEX},
            {"UPBIT", ExchangeType::UPBIT}
        };
        auto it = str_map.find(str);
        if (it != str_map.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    /// \brief Serializes an ExchangeType to JSON.
    inline void to_json(nlohmann::json& j, const ExchangeType& value) {
        j = to_str(value);
    }

    /// \brief Deserializes an ExchangeType from JSON.
    inline void from_json(const nlohmann::json& j, ExchangeType& value) {
        if (!to_enum(j.get<std::string>(), value)) {
            value = ExchangeType::UNKNOWN;
        }
    }

    /// \brief Outputs string representation to stream.
    inline std::ostream& operator<<(std::ostream& os, ExchangeType value) {
        os << to_str(value);
        return os;
    }

}; // namespace dfh

#endif // _DFH_DATA_СOMMON_ENUMS_HPP_INCLUDED
