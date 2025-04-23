#pragma once
#ifndef _DFH_DATA_TRADING_ENUMS_HPP_INCLUDED
#define _DFH_DATA_TRADING_ENUMS_HPP_INCLUDED

/// \file enums.hpp
/// \brief Enumerations for trading parameters.

namespace dfh {

    /// \enum OrderSide
    /// \brief Defines the possible sides of an order (buy/sell).
    enum class OrderSide {
        UNKNOWN = 0, ///< Unknown order side
        BUY,         ///< Buy order
        SELL         ///< Sell order
    };

    /// \brief Converts OrderSide to its string representation.
    inline const std::string& to_str(OrderSide value) noexcept {
        static const std::vector<std::string> str_data = {"UNKNOWN", "BUY", "SELL"};
        return str_data[static_cast<size_t>(value)];
    }

    /// \brief Converts string to OrderSide enumeration.
    inline bool to_enum(const std::string& str, OrderSide& value) noexcept {
        static const std::unordered_map<std::string, OrderSide> str_data = {
            {"UNKNOWN", OrderSide::UNKNOWN},
            {"BUY", OrderSide::BUY},
            {"SELL", OrderSide::SELL}
        };
        auto it = str_data.find(str);
        if (it != str_data.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    /// \brief Template specialization for OrderSide enum conversion.
    template <>
    inline OrderSide to_enum<OrderSide>(const std::string& str) {
        OrderSide value;
        if (!to_enum(str, value)) {
            throw std::invalid_argument("Invalid OrderSide string: " + str);
        }
        return value;
    }

	/// \brief Converts OrderSide to JSON.
    inline void to_json(nlohmann::json& j, const OrderSide& value) {
        j = to_str(value);
    }

    /// \brief Converts JSON to OrderSide.
    inline void from_json(const nlohmann::json& j, OrderSide& value) {
        value = to_enum<OrderSide>(j.get<std::string>());
    }

	/// \brief Stream output operator for OrderSide.
    inline std::ostream& operator<<(std::ostream& os, OrderSide value) {
        os << to_str(value);
        return os;
    }

//------------------------------------------------------------------------------

    /// \enum OrderType
    /// \brief Defines the possible types of orders.
    enum class OrderType {
        UNKNOWN = 0, ///< Unknown order type
        MARKET,      ///< Market order
        LIMIT,       ///< Limit order
        STOP,        ///< Stop order
        STOP_LIMIT   ///< Stop-limit order
    };

    /// \brief Converts OrderType to its string representation.
    inline const std::string& to_str(OrderType value) noexcept {
        static const std::vector<std::string> str_data = {"UNKNOWN", "MARKET", "LIMIT", "STOP", "STOP_LIMIT"};
        return str_data[static_cast<size_t>(value)];
    }

    /// \brief Converts string to OrderType enumeration.
    inline bool to_enum(const std::string& str, OrderType& value) noexcept {
        static const std::unordered_map<std::string, OrderType> str_data = {
            {"UNKNOWN", OrderType::UNKNOWN},
            {"MARKET", OrderType::MARKET},
            {"LIMIT", OrderType::LIMIT},
            {"STOP", OrderType::STOP},
            {"STOP_LIMIT", OrderType::STOP_LIMIT}
        };
        auto it = str_data.find(str);
        if (it != str_data.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    /// \brief Template specialization for OrderType enum conversion.
    template <>
    inline OrderType to_enum<OrderType>(const std::string& str) {
        OrderType value;
        if (!to_enum(str, value)) {
            throw std::invalid_argument("Invalid OrderType string: " + str);
        }
        return value;
    }

    /// \brief Converts OrderType to JSON.
    inline void to_json(nlohmann::json& j, const OrderType& value) {
        j = to_str(value);
    }

    /// \brief Converts JSON to OrderType.
    inline void from_json(const nlohmann::json& j, OrderType& value) {
        value = to_enum<OrderType>(j.get<std::string>());
    }

    /// \brief Stream output operator for OrderType.
    inline std::ostream& operator<<(std::ostream& os, OrderType value) {
        os << to_str(value);
        return os;
    }

//------------------------------------------------------------------------------

	/// \enum TimeInForce
    /// \brief Order duration policies.
    enum class TimeInForce {
        UNKNOWN = 0,
        GTC,    ///< Good till cancelled
        IOC,    ///< Immediate or cancel
        FOK,    ///< Fill or kill
        GTX     ///< Good till crossing (post only)
    };

    inline const std::string& to_str(TimeInForce value) noexcept {
        static const std::vector<std::string> str_data = {"UNKNOWN", "GTC", "IOC", "FOK", "GTX"};
        return str_data[static_cast<size_t>(value)];
    }

    inline bool to_enum(const std::string& str, TimeInForce& value) noexcept {
        static const std::unordered_map<std::string, TimeInForce> str_data = {
            {"UNKNOWN", TimeInForce::UNKNOWN}, {"GTC", TimeInForce::GTC},
            {"IOC", TimeInForce::IOC}, {"FOK", TimeInForce::FOK}, {"GTX", TimeInForce::GTX}
        };
        auto it = str_data.find(str);
        if (it != str_data.end()) { value = it->second; return true; }
        return false;
    }

    /// \brief Template specialization for TimeInForce enum conversion.
    template <>
    inline TimeInForce to_enum<TimeInForce>(const std::string& str) {
        TimeInForce value;
        if (!to_enum(str, value)) {
            throw std::invalid_argument("Invalid TimeInForce string: " + str);
        }
        return value;
    }

    inline void to_json(nlohmann::json& j, const TimeInForce& value) {
		j = to_str(value);
	}

    inline void from_json(const nlohmann::json& j, TimeInForce& value) {
		value = to_enum<TimeInForce>(j.get<std::string>());
	}

    inline std::ostream& operator<<(std::ostream& os, TimeInForce value) {
		os << to_str(value);
		return os;
	}

//------------------------------------------------------------------------------

	/// \enum SlippageType
    /// \brief Slippage tolerance modes.
    enum class SlippageType {
        UNKNOWN = 0,
        ABSOLUTE_VALUE, ///< Slippage defined in ticks or currency units
        PERCENT_VALUE   ///< Slippage defined as a percentage of price
    };

    inline const std::string& to_str(SlippageType value) noexcept {
        static const std::vector<std::string> str_data = {"UNKNOWN", "ABSOLUTE_VALUE", "PERCENT_VALUE"};
        return str_data[static_cast<size_t>(value)];
    }

    inline bool to_enum(const std::string& str, SlippageType& value) noexcept {
        static const std::unordered_map<std::string, SlippageType> str_data = {
            {"UNKNOWN",        SlippageType::UNKNOWN},
			{"ABSOLUTE_VALUE", SlippageType::ABSOLUTE_VALUE},
            {"PERCENT_VALUE",  SlippageType::PERCENT_VALUE}
        };
        auto it = str_data.find(str);
        if (it != str_data.end()) { value = it->second; return true; }
        return false;
    }

    /// \brief Template specialization for SlippageType enum conversion.
    template <>
    inline SlippageType to_enum<SlippageType>(const std::string& str) {
        SlippageType value;
        if (!to_enum(str, value)) {
            throw std::invalid_argument("Invalid SlippageType string: " + str);
        }
        return value;
    }

    inline void to_json(nlohmann::json& j, const SlippageType& value) {
		j = to_str(value);
	}

    inline void from_json(const nlohmann::json& j, SlippageType& value) {
		value = to_enum<SlippageType>(j.get<std::string>());
	}

	inline std::ostream& operator<<(std::ostream& os, SlippageType value) {
		os << to_str(value);
		return os;
	}

//------------------------------------------------------------------------------

	/// \enum TradeMode
    /// \brief Trading mode (spot, futures, etc.)
    enum class TradeMode {
        UNKNOWN = 0,
        SPOT,
        LINEAR,    ///< USDT-margined futures
        INVERSE,   ///< Coin-margined futures
        OPTION
    };

    inline const std::string& to_str(TradeMode value) noexcept {
        static const std::vector<std::string> str_data = {
			"UNKNOWN", "SPOT", "LINEAR", "INVERSE", "OPTION"};
        return str_data[static_cast<size_t>(value)];
    }

    inline bool to_enum(const std::string& str, TradeMode& value) noexcept {
        static const std::unordered_map<std::string, TradeMode> str_data = {
            {"UNKNOWN", TradeMode::UNKNOWN},
			{"SPOT", TradeMode::SPOT},
            {"LINEAR", TradeMode::LINEAR},
			{"INVERSE", TradeMode::INVERSE},
            {"OPTION", TradeMode::OPTION}
        };
        auto it = str_data.find(str);
        if (it != str_data.end()) { value = it->second; return true; }
        return false;
    }

    /// \brief Template specialization for TradeMode enum conversion.
    template <>
    inline TradeMode to_enum<TradeMode>(const std::string& str) {
        TradeMode value;
        if (!to_enum(str, value)) {
            throw std::invalid_argument("Invalid TradeMode string: " + str);
        }
        return value;
    }

    inline void to_json(nlohmann::json& j, const TradeMode& value) {
		j = to_str(value);
	}

    inline void from_json(const nlohmann::json& j, TradeMode& value) {
		value = to_enum<TradeMode>(j.get<std::string>());
	}

    inline std::ostream& operator<<(std::ostream& os, TradeMode value) {
		os << to_str(value); return os;
	}

//------------------------------------------------------------------------------

    /// \enum PriceTriggerType
    /// \brief Trigger price type for SL/TP.
    enum class PriceTriggerType {
        UNKNOWN = 0,
        MARK_PRICE,
        INDEX_PRICE,
        LAST_PRICE
    };

    inline const std::string& to_str(PriceTriggerType value) noexcept {
        static const std::vector<std::string> str_data = {
			"UNKNOWN", "MARK_PRICE", "INDEX_PRICE", "LAST_PRICE"};
        return str_data[static_cast<size_t>(value)];
    }

    inline bool to_enum(const std::string& str, PriceTriggerType& value) noexcept {
        static const std::unordered_map<std::string, PriceTriggerType> str_data = {
            {"UNKNOWN", PriceTriggerType::UNKNOWN},
			{"MARK_PRICE", PriceTriggerType::MARK_PRICE},
            {"INDEX_PRICE", PriceTriggerType::INDEX_PRICE},
			{"LAST_PRICE", PriceTriggerType::LAST_PRICE}
        };
        auto it = str_data.find(str);
        if (it != str_data.end()) { value = it->second; return true; }
        return false;
    }

    /// \brief Template specialization for PriceTriggerType enum conversion.
    template <>
    inline PriceTriggerType to_enum<PriceTriggerType>(const std::string& str) {
        PriceTriggerType value;
        if (!to_enum(str, value)) {
            throw std::invalid_argument("Invalid PriceTriggerType string: " + str);
        }
        return value;
    }

    inline void to_json(nlohmann::json& j, const PriceTriggerType& value) {
		j = to_str(value);
	}

    inline void from_json(const nlohmann::json& j, PriceTriggerType& value) {
		value = to_enum<PriceTriggerType>(j.get<std::string>());
	}

	inline std::ostream& operator<<(std::ostream& os, PriceTriggerType value) {
		os << to_str(value); return os;
	}

//------------------------------------------------------------------------------

	/// \enum MarginMode
    /// brief Defines margin modes for futures trading.
    enum class MarginMode {
        UNKNOWN = 0,
		CROSS,     ///< Cross margin: all positions share collateral.
        ISOLATED   ///< Isolated margin: each position has independent collateral.
    };

    /// brief Converts MarginMode to its string representation.
    inline const std::string& to_str(MarginMode value) noexcept {
        static const std::vector<std::string> str_data = {
			"UNKNOWN", "CROSS", "ISOLATED"};
        return str_data[static_cast<size_t>(value)];
    }

    /// brief Converts a string to its corresponding MarginMode enumeration value.
    inline bool to_enum(const std::string& str, MarginMode& value) noexcept {
        static const std::unordered_map<std::string, MarginMode> str_data = {
            {"UNKNOWN", MarginMode::UNKNOWN},
			{"CROSS", MarginMode::CROSS},
            {"ISOLATED", MarginMode::ISOLATED}
        };
        auto it = str_data.find(str);
        if (it != str_data.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    /// \brief Template specialization for MarginMode enum conversion.
    template <>
    inline MarginMode to_enum<MarginMode>(const std::string& str) {
        MarginMode value;
        if (!to_enum(str, value)) {
            throw std::invalid_argument("Invalid MarginMode string: " + str);
        }
        return value;
    }

    /// brief Converts MarginMode to JSON.
    inline void to_json(nlohmann::json& j, const MarginMode& value) {
        j = to_str(value);
    }

    /// brief Converts JSON to MarginMode.
    inline void from_json(const nlohmann::json& j, MarginMode& value) {
        value = to_enum<MarginMode>(j.get<std::string>());
    }

    /// brief Stream output operator for MarginMode.
    inline std::ostream& operator<<(std::ostream& os, MarginMode value) {
        os << to_str(value);
        return os;
    }

//------------------------------------------------------------------------------

    /// \enum TradeState
    /// \brief Represents the lifecycle state of a trade.
    enum class TradeState {
        UNKNOWN = 0,             ///< Undefined state.

        // Open lifecycle
        OPEN_PLACE,             ///< Trade request created and passed to interface.
        OPEN_SEND,              ///< Trade sent to broker/exchange.
        OPEN_PENDING,           ///< Awaiting response or fill from broker.
        OPEN_PARTIAL,           ///< Trade partially filled.
        OPENED,                 ///< Trade fully opened.

        // Active
        ACTIVE,                 ///< Trade is actively open in the market.

        // Close lifecycle
        CLOSE_PLACE,            ///< Close request created.
        CLOSE_SEND,             ///< Close request sent to broker.
        CLOSE_PENDING,          ///< Awaiting close execution.
        CLOSE_PARTIAL,          ///< Close partially filled.
        CLOSED,                 ///< Trade fully closed.

        // Finalized/error states
        CANCELED,               ///< Trade was canceled before execution.
        REJECTED,               ///< Trade was rejected by the exchange/broker.
        EXPIRED,                ///< Trade expired (TTL or time-in-force expired).
        CLOSE_ERROR,            ///< Error occurred while closing trade.
        FAILED_OPEN,            ///< Failed to open the trade.
        FAILED_CLOSE            ///< Failed to close the trade.
    };

    /// \brief Converts TradeState to string.
    inline const std::string& to_str(TradeState value) noexcept {
        static const std::vector<std::string> str_data = {
            "UNKNOWN",
            "OPEN_PLACE",
            "OPEN_SEND",
            "OPEN_PENDING",
            "OPEN_PARTIAL",
            "OPENED",
            "ACTIVE",
            "CLOSE_PLACE",
            "CLOSE_SEND",
            "CLOSE_PENDING",
            "CLOSE_PARTIAL",
            "CLOSED",
            "CANCELED",
            "REJECTED",
            "EXPIRED",
            "CLOSE_ERROR",
            "FAILED_OPEN",
            "FAILED_CLOSE"
        };
        return str_data[static_cast<size_t>(value)];
    }

    /// \brief Converts string to TradeState.
    inline bool to_enum(const std::string& str, TradeState& value) noexcept {
        static const std::unordered_map<std::string, TradeState> str_map = {
            {"UNKNOWN",         TradeState::UNKNOWN},
            {"OPEN_PLACE",      TradeState::OPEN_PLACE},
            {"OPEN_SEND",       TradeState::OPEN_SEND},
            {"OPEN_PENDING",    TradeState::OPEN_PENDING},
            {"OPEN_PARTIAL",    TradeState::OPEN_PARTIAL},
            {"OPENED",          TradeState::OPENED},
            {"ACTIVE",          TradeState::ACTIVE},
            {"CLOSE_PLACE",     TradeState::CLOSE_PLACE},
            {"CLOSE_SEND",      TradeState::CLOSE_SEND},
            {"CLOSE_PENDING",   TradeState::CLOSE_PENDING},
            {"CLOSE_PARTIAL",   TradeState::CLOSE_PARTIAL},
            {"CLOSED",          TradeState::CLOSED},
            {"CANCELED",        TradeState::CANCELED},
            {"REJECTED",        TradeState::REJECTED},
            {"EXPIRED",         TradeState::EXPIRED},
            {"CLOSE_ERROR",     TradeState::CLOSE_ERROR},
            {"FAILED_OPEN",     TradeState::FAILED_OPEN},
            {"FAILED_CLOSE",    TradeState::FAILED_CLOSE}
        };
        auto it = str_map.find(str);
        if (it != str_map.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    /// \brief Template specialization for TradeState.
    template <>
    inline TradeState to_enum<TradeState>(const std::string& str) {
        TradeState value;
        if (!to_enum(str, value)) {
            throw std::invalid_argument("Invalid TradeState string: " + str);
        }
        return value;
    }

    /// \brief Converts TradeState to JSON.
    inline void to_json(nlohmann::json& j, const TradeState& state) {
        j = dfh::to_str(state);
    }

    /// \brief Converts JSON to TradeState.
    inline void from_json(const nlohmann::json& j, TradeState& state) {
        state = dfh::to_enum<TradeState>(j.get<std::string>());
    }

    /// \brief Stream output operator for TradeState.
    inline std::ostream& operator<<(std::ostream& os, TradeState value) {
        os << dfh::to_str(value);
        return os;
    }

//------------------------------------------------------------------------------

	/// \enum TpSlMode
	/// \brief Defines behavior for Take Profit / Stop Loss triggering.
	enum class TpSlMode {
		UNKNOWN = 0, ///< Unknown mode
		FULL    = 1, ///< Close entire position
		PARTIAL = 2  ///< Close partial position
	};

	/// \brief Converts TpSlMode to its string representation.
	inline const std::string& to_str(TpSlMode mode) noexcept {
		static const std::vector<std::string> str_data = {
			"UNKNOWN", "FULL", "PARTIAL"
		};
		return str_data[static_cast<size_t>(mode)];
	}

	/// \brief Converts string to TpSlMode.
	inline bool to_enum(const std::string& str, TpSlMode& mode) noexcept {
		static const std::unordered_map<std::string, TpSlMode> str_data = {
			{"UNKNOWN", TpSlMode::UNKNOWN},
			{"FULL", TpSlMode::FULL},
			{"PARTIAL", TpSlMode::PARTIAL}
		};
		auto it = str_data.find(dfh::utils::to_upper_case(str));
		if (it != str_data.end()) {
			mode = it->second;
			return true;
		}
		return false;
	}

	/// \brief Template specialization for TpSlMode enum conversion.
    template <>
    inline TpSlMode to_enum<TpSlMode>(const std::string& str) {
        TpSlMode value;
        if (!to_enum(str, value)) {
            throw std::invalid_argument("Invalid TpSlMode string: " + str);
        }
        return value;
    }

	inline std::ostream& operator<<(std::ostream& os, TpSlMode value) {
        os << dfh::to_str(value);
        return os;
    }

//------------------------------------------------------------------------------

	/// \enum SmpType
	/// \brief Defines behavior for Self Match Prevention.
	enum class SmpType {
		UNKNOWN      = 0, ///< Not specified
		CANCEL_MAKER = 1, ///< Cancel maker order when matched
		CANCEL_TAKER = 2, ///< Cancel taker order when matched
		CANCEL_BOTH  = 3  ///< Cancel both maker and taker
	};

	/// \brief Converts SmpType to string.
	inline const std::string& to_str(SmpType type) noexcept {
		static const std::vector<std::string> str_data = {
			"UNKNOWN", "CANCEL_MAKER", "CANCEL_TAKER", "CANCEL_BOTH"
		};
		return str_data[static_cast<size_t>(type)];
	}

	/// \brief Converts string to SmpType.
	inline bool to_enum(const std::string& str, SmpType& type) noexcept {
		static const std::unordered_map<std::string, SmpType> str_data = {
			{"UNKNOWN", SmpType::UNKNOWN},
			{"CANCEL_MAKER", SmpType::CANCEL_MAKER},
			{"CANCEL_TAKER", SmpType::CANCEL_TAKER},
			{"CANCEL_BOTH",  SmpType::CANCEL_BOTH}
		};
		auto it = str_data.find(dfh::utils::to_upper_case(str));
		if (it != str_data.end()) {
			type = it->second;
			return true;
		}
		return false;
	}

	/// \brief Template specialization for SmpType enum conversion.
    template <>
    inline SmpType to_enum<SmpType>(const std::string& str) {
        SmpType value;
        if (!to_enum(str, value)) {
            throw std::invalid_argument("Invalid SmpType string: " + str);
        }
        return value;
    }

	inline std::ostream& operator<<(std::ostream& os, SmpType value) {
        os << dfh::to_str(value);
        return os;
    }

//------------------------------------------------------------------------------

	/// \enum PositionIdx
	/// \brief Used to identify positions in hedge vs one-way mode.
	enum class PositionIdx {
		UNKNOWN = 0,     ///< Not specified or irrelevant.
		ONE_WAY = 1,     ///< One-way position mode.
		HEDGE_BUY = 2,   ///< Hedge mode - long side.
		HEDGE_SELL = 3   ///< Hedge mode - short side.
	};

	/// \brief Converts PositionIdx to string.
	inline const std::string& to_str(PositionIdx value) noexcept {
		static const std::vector<std::string> str_data = {
			"UNKNOWN", "ONE_WAY", "HEDGE_BUY", "HEDGE_SELL"
		};
		return str_data[static_cast<size_t>(value)];
	}

	/// \brief Converts string to PositionIdx.
	inline bool to_enum(const std::string& str, PositionIdx& value) noexcept {
		static const std::unordered_map<std::string, PositionIdx> str_data = {
			{"UNKNOWN", PositionIdx::UNKNOWN},
			{"ONE_WAY", PositionIdx::ONE_WAY},
			{"HEDGE_BUY", PositionIdx::HEDGE_BUY},
			{"HEDGE_SELL", PositionIdx::HEDGE_SELL}
		};
		auto it = str_data.find(dfh::utils::to_upper_case(str));
		if (it != str_data.end()) {
			value = it->second;
			return true;
		}
		return false;
	}

	/// \brief Template specialization for PositionIdx enum conversion.
    template <>
    inline PositionIdx to_enum<PositionIdx>(const std::string& str) {
        PositionIdx value;
        if (!to_enum(str, value)) {
            throw std::invalid_argument("Invalid PositionIdx string: " + str);
        }
        return value;
    }

	inline void to_json(nlohmann::json& j, const PositionIdx& value) {
		j = to_str(value);
	}

	inline void from_json(const nlohmann::json& j, PositionIdx& value) {
		value = to_enum<PositionIdx>(j.get<std::string>());
	}

	inline std::ostream& operator<<(std::ostream& os, PositionIdx value) {
		os << to_str(value);
		return os;
	}

//------------------------------------------------------------------------------

	/// \enum QuantityUnit
	/// \brief Defines quantity unit type for orders: base or quote currency.
	enum class QuantityUnit {
		UNKNOWN = 0, ///< Unknown or not specified
		BASE    = 1, ///< Amount is specified in base currency (e.g., BTC in BTC/USDT)
		QUOTE   = 2  ///< Amount is specified in quote currency (e.g., USDT in BTC/USDT)
	};

	/// \brief Converts QuantityUnit to string.
	inline const std::string& to_str(QuantityUnit value) noexcept {
		static const std::vector<std::string> str_data = {
			"UNKNOWN", "BASE", "QUOTE"
		};
		return str_data[static_cast<size_t>(value)];
	}

	/// \brief Converts string to QuantityUnit.
	inline bool to_enum(const std::string& str, QuantityUnit& value) noexcept {
		static const std::unordered_map<std::string, QuantityUnit> str_map = {
			{"UNKNOWN", QuantityUnit::UNKNOWN},
			{"BASE",    QuantityUnit::BASE},
			{"QUOTE",   QuantityUnit::QUOTE}
		};
		auto it = str_map.find(dfh::utils::to_upper_case(str));
		if (it != str_map.end()) {
			value = it->second;
			return true;
		}
		return false;
	}

	/// \brief Template specialization for QuantityUnit conversion.
	template <>
	inline QuantityUnit to_enum<QuantityUnit>(const std::string& str) {
		QuantityUnit value;
		if (!to_enum(str, value)) {
			throw std::invalid_argument("Invalid QuantityUnit string: " + str);
		}
		return value;
	}

	/// \brief Converts QuantityUnit to JSON.
	inline void to_json(nlohmann::json& j, const QuantityUnit& value) {
		j = to_str(value);
	}

	/// \brief Converts JSON to QuantityUnit.
	inline void from_json(const nlohmann::json& j, QuantityUnit& value) {
		value = to_enum<QuantityUnit>(j.get<std::string>());
	}

	/// \brief Stream output operator for QuantityUnit.
	inline std::ostream& operator<<(std::ostream& os, QuantityUnit value) {
		os << to_str(value);
		return os;
	}

//------------------------------------------------------------------------------

	/// \enum AccountType
    /// \brief Represents account types (Demo or Real).
    enum class AccountType {
        UNKNOWN = 0, ///< Unknown account type
        DEMO,        ///< Demo trading account
        REAL         ///< Real money account
    };

    inline const std::string& to_str(AccountType value, int mode = 0) noexcept {
        static const std::vector<std::string> str_data_0 = {"UNKNOWN", "DEMO", "REAL"};
        static const std::vector<std::string> str_data_1 = {"Unknown", "Demo", "Real"};

        switch (mode) {
            case 1: return str_data_1[static_cast<size_t>(value)];
            default: return str_data_0[static_cast<size_t>(value)];
        }
    }

    inline bool to_enum(const std::string& str, AccountType& value) noexcept {
        static const std::unordered_map<std::string, AccountType> str_data = {
            {"UNKNOWN", AccountType::UNKNOWN},
            {"DEMO", AccountType::DEMO},
            {"REAL", AccountType::REAL}
        };
        auto it = str_data.find(str);
        if (it != str_data.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    /// \brief Template specialization for AccountType enum conversion.
    template <>
    inline AccountType to_enum<AccountType>(const std::string& str) {
        AccountType value;
        if (!to_enum(str, value)) {
            throw std::invalid_argument("Invalid AccountType string: " + str);
        }
        return value;
    }

    inline void to_json(nlohmann::json& j, const AccountType& value) {
        j = dfh::to_str(value);
    }

    inline void from_json(const nlohmann::json& j, AccountType& value) {
        value = dfh::to_enum<AccountType>(j.get<std::string>());
    }

    inline std::ostream& operator<<(std::ostream& os, AccountType value) {
        os << dfh::to_str(value);
        return os;
    }

//------------------------------------------------------------------------------

    /// \enum TradeErrorCode
    /// \brief Represents error codes for order validation and processing.
    enum class TradeErrorCode {
        SUCCESS,                ///< Operation successful.
        INVALID_SYMBOL,         ///< Invalid trading symbol.
        INVALID_ORDER,          ///< Invalid order type.
        INVALID_ACCOUNT,        ///< Invalid account type.
        INVALID_CURRENCY,       ///< Invalid currency.
        AMOUNT_TOO_LOW,         ///< Order amount is below the minimum allowed.
        AMOUNT_TOO_HIGH,        ///< Order amount exceeds the maximum allowed.
        PRICE_OUT_OF_RANGE,     ///< Price is outside the acceptable range.
        SLIPPAGE_EXCEEDED,      ///< Order execution failed due to excessive slippage.
        ORDER_NOT_FILLED,       ///< Order was placed but not filled.
        ORDER_PARTIALLY_FILLED, ///< Order was partially filled.
        ORDER_ALREADY_CLOSED,   ///< Order was already closed before modification/cancelation.
        ORDER_CANCELLED,        ///< Order was canceled.
        ORDER_REJECTED,         ///< Order was rejected by the exchange.
        INSUFFICIENT_BALANCE,   ///< Not enough funds to execute the order.
        INSUFFICIENT_MARGIN,    ///< Not enough margin for leveraged trading.
        POSITION_NOT_FOUND,     ///< Position not found (for closing/modifying).
        POSITION_MODE_MISMATCH, ///< Hedge/One-Way mode conflict.
        DUPLICATE_ORDER,        ///< Order with the same ID already exists.
        INVALID_TIME_IN_FORCE,  ///< Invalid or unsupported Time in Force (TIF) setting.
        INVALID_STOP_PRICE,     ///< Invalid stop price for stop/stop-limit order.
        RATE_LIMIT_EXCEEDED,    ///< API rate limit exceeded.
        CONNECTION_ERROR,       ///< Network connection issue.
        SERVER_ERROR,           ///< Exchange/internal server error.
        INVALID_REQUEST,        ///< Malformed request.
        TIMEOUT,                ///< Request timed out.
        PARSING_ERROR,          ///< Response parsing error.
        UNKNOWN_ERROR           ///< Unknown error occurred.
    };

    /// \brief Converts TradeErrorCode to its string representation.
    inline const std::string& to_str(TradeErrorCode value) noexcept {
        static const std::vector<std::string> str_data = {
            "SUCCESS",
            "INVALID_SYMBOL",
            "INVALID_ORDER",
            "INVALID_ACCOUNT",
            "INVALID_CURRENCY",
            "AMOUNT_TOO_LOW",
            "AMOUNT_TOO_HIGH",
            "PRICE_OUT_OF_RANGE",
            "SLIPPAGE_EXCEEDED",
            "ORDER_NOT_FILLED",
            "ORDER_PARTIALLY_FILLED",
            "ORDER_ALREADY_CLOSED",
            "ORDER_CANCELLED",
            "ORDER_REJECTED",
            "INSUFFICIENT_BALANCE",
            "INSUFFICIENT_MARGIN",
            "POSITION_NOT_FOUND",
            "POSITION_MODE_MISMATCH",
            "DUPLICATE_ORDER",
            "INVALID_TIME_IN_FORCE",
            "INVALID_STOP_PRICE",
            "RATE_LIMIT_EXCEEDED",
            "CONNECTION_ERROR",
            "SERVER_ERROR",
            "INVALID_REQUEST",
            "TIMEOUT",
            "PARSING_ERROR",
            "UNKNOWN_ERROR"
        };
        return str_data[static_cast<size_t>(value)];
    }

    /// \brief Converts string to TradeErrorCode enumeration.
    inline bool to_enum(const std::string& str, TradeErrorCode& value) noexcept {
        static const std::unordered_map<std::string, TradeErrorCode> str_data = {
            {"SUCCESS", TradeErrorCode::SUCCESS},
            {"INVALID_SYMBOL", TradeErrorCode::INVALID_SYMBOL},
            {"INVALID_ORDER", TradeErrorCode::INVALID_ORDER},
            {"INVALID_ACCOUNT", TradeErrorCode::INVALID_ACCOUNT},
            {"INVALID_CURRENCY", TradeErrorCode::INVALID_CURRENCY},
            {"AMOUNT_TOO_LOW", TradeErrorCode::AMOUNT_TOO_LOW},
            {"AMOUNT_TOO_HIGH", TradeErrorCode::AMOUNT_TOO_HIGH},
            {"PRICE_OUT_OF_RANGE", TradeErrorCode::PRICE_OUT_OF_RANGE},
            {"SLIPPAGE_EXCEEDED", TradeErrorCode::SLIPPAGE_EXCEEDED},
            {"ORDER_NOT_FILLED", TradeErrorCode::ORDER_NOT_FILLED},
            {"ORDER_PARTIALLY_FILLED", TradeErrorCode::ORDER_PARTIALLY_FILLED},
            {"ORDER_ALREADY_CLOSED", TradeErrorCode::ORDER_ALREADY_CLOSED},
            {"ORDER_CANCELLED", TradeErrorCode::ORDER_CANCELLED},
            {"ORDER_REJECTED", TradeErrorCode::ORDER_REJECTED},
            {"INSUFFICIENT_BALANCE", TradeErrorCode::INSUFFICIENT_BALANCE},
            {"INSUFFICIENT_MARGIN", TradeErrorCode::INSUFFICIENT_MARGIN},
            {"POSITION_NOT_FOUND", TradeErrorCode::POSITION_NOT_FOUND},
            {"POSITION_MODE_MISMATCH", TradeErrorCode::POSITION_MODE_MISMATCH},
            {"DUPLICATE_ORDER", TradeErrorCode::DUPLICATE_ORDER},
            {"INVALID_TIME_IN_FORCE", TradeErrorCode::INVALID_TIME_IN_FORCE},
            {"INVALID_STOP_PRICE", TradeErrorCode::INVALID_STOP_PRICE},
            {"RATE_LIMIT_EXCEEDED", TradeErrorCode::RATE_LIMIT_EXCEEDED},
            {"CONNECTION_ERROR", TradeErrorCode::CONNECTION_ERROR},
            {"SERVER_ERROR", TradeErrorCode::SERVER_ERROR},
            {"INVALID_REQUEST", TradeErrorCode::INVALID_REQUEST},
            {"TIMEOUT", TradeErrorCode::TIMEOUT},
            {"PARSING_ERROR", TradeErrorCode::PARSING_ERROR},
            {"UNKNOWN_ERROR", TradeErrorCode::UNKNOWN_ERROR}
        };

        auto it = str_data.find(str);
        if (it != str_data.end()) {
            value = it->second;
            return true;
        }
        return false;
    }

    /// \brief Template specialization for TradeErrorCode enum conversion.
    template <>
    inline TradeErrorCode to_enum<TradeErrorCode>(const std::string& str) {
        TradeErrorCode value;
        if (!to_enum(str, value)) {
            throw std::invalid_argument("Invalid TradeErrorCode string: " + str);
        }
        return value;
    }

    /// \brief Converts TradeErrorCode to JSON.
    inline void to_json(nlohmann::json& j, const TradeErrorCode& state) {
        j = to_str(state);
    }

    /// \brief Converts JSON to TradeErrorCode.
    inline void from_json(const nlohmann::json& j, TradeErrorCode& state) {
        state = to_enum<TradeErrorCode>(j.get<std::string>());
    }

    /// \brief Stream output operator for TradeErrorCode.
    inline std::ostream& operator<<(std::ostream& os, TradeErrorCode value) {
        os << to_str(value);
        return os;
    }

}; // namespace dfh

#endif // _DFH_DATA_TRADING_ENUMS_HPP_INCLUDED
