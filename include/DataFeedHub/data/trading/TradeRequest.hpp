#pragma once
#ifndef _DFH_TRADE_REQUEST_HPP_INCLUDED
#define _DFH_TRADE_REQUEST_HPP_INCLUDED

/// \file TradeRequest.hpp
/// \brief Defines the TradeRequest class, which represents a trade request for execution.

namespace dfh {
	
    /// \class TradeRequest
    /// \brief Represents a trade request with all necessary parameters for execution.
    class TradeRequest {
    public:
        // --- Trade meta parameters ---
        std::string symbol;              ///< Trading symbol (e.g., "BTC/USDT").
        std::string signal_name;         ///< Name of the strategy or signal.
        std::string user_data;           ///< Optional user data.
        std::string comment;             ///< Optional trade comment.
        std::string unique_hash;         ///< Unique hash to avoid duplicate trades.

        // --- Identifiers ---
        int64_t unique_id = 0;           ///< Unique request ID.
        int64_t account_id = 0;          ///< Associated trading account ID.

        // --- Trading configuration ---
        OrderSide side = OrderSide::UNKNOWN;       ///< Buy or Sell.
        OrderType order_type = OrderType::UNKNOWN; ///< Market, Limit, etc.
        TradeMode trade_mode = TradeMode::UNKNOWN; ///< Spot, Futures (linear/inverse), Options.
        AccountType account_type = AccountType::UNKNOWN; ///< Real or Demo.
        std::string currency;   				   ///< Trading currency.

        // --- Trade values ---
        double amount = 0.0;         ///< Trade amount in base currency.
        double price = 0.0;          ///< Limit price if applicable.
        double stop_loss = 0.0;      ///< Stop loss price level.
        double take_profit = 0.0;    ///< Take profit price level.

        // --- Slippage control ---
        double slippage_value = 0.0; ///< Slippage tolerance value.
        SlippageType slippage_type = SlippageType::UNKNOWN; ///< Slippage type (percent, tick size).

        // --- Execution settings ---
        TimeInForce time_in_force = TimeInForce::UNKNOWN; ///< Time in force (GTC, IOC, FOK).
        bool reduce_only = false;          ///< Reduce only flag (futures).
        bool close_on_trigger = false;     ///< Close on trigger (used in risk control).

        // --- Trigger price types ---
        PriceTriggerType tp_trigger = PriceTriggerType::LAST_PRICE; ///< Price type for take profit trigger.
        PriceTriggerType sl_trigger = PriceTriggerType::LAST_PRICE; ///< Price type for stop loss trigger.

        // --- Position mode ---
        bool hedge_mode = false;  ///< If true, supports both long and short positions independently.

        // --- Expiration / lifetime ---
        int64_t expiry_time = 0;   ///< Expiration timestamp (if needed).

        // --- Callback mechanism ---
        using callback_t = std::function<void(std::unique_ptr<TradeRequest>, std::unique_ptr<class TradeResult>)>;

        /// \brief Adds a callback to be invoked when a result is available.
        void add_callback(callback_t callback) {
            m_callbacks.push_back(std::move(callback));
        }

        /// \brief Dispatches callbacks with the trade result.
        template<class RequestType, class ResultType>
        void dispatch_callbacks(RequestType& request, ResultType& result) {
            for (auto& cb : m_callbacks) {
                cb(request->clone_unique(), result->clone_unique());
            }
        }

        /// \brief Clones the request into a unique pointer.
        virtual std::unique_ptr<TradeRequest> clone_unique() const {
            return std::make_unique<TradeRequest>(*this);
        }

        /// \brief Clones the request into a shared pointer.
        virtual std::shared_ptr<TradeRequest> clone_shared() const {
            return std::make_shared<TradeRequest>(*this);
        }

        virtual ~TradeRequest() = default;

        /// \brief Defines the JSON serialization format for `TradeRequest`.
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(
            TradeRequest,
            symbol,
            signal_name,
            user_data,
            comment,
            unique_hash,
            unique_id,
            account_id,
            side,
            order_type,
            trade_mode,
            account_type,
            currency,
            amount,
            price,
            stop_loss,
            take_profit,
            slippage_value,
            slippage_type,
            time_in_force,
            reduce_only,
            close_on_trigger,
            tp_trigger,
            sl_trigger,
            hedge_mode,
            expiry_time
        )

    private:
        std::list<callback_t> m_callbacks; ///< List of callbacks.
    };

    /// \brief Type alias for a unique pointer to `TradeRequest`.
    using trade_request_t = std::unique_ptr<TradeRequest>;

    /// \brief Type alias for a trade result callback function.
    using trade_result_callback_t = std::function<void(std::unique_ptr<TradeRequest>, std::unique_ptr<TradeResult>)>;

} // namespace dfh

#endif // _DFH_TRADE_REQUEST_HPP_INCLUDED