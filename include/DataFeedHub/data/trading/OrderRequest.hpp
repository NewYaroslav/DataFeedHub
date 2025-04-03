#pragma once
#ifndef _DFH_ORDER_REQUEST_HPP_INCLUDED
#define _DFH_ORDER_REQUEST_HPP_INCLUDED

/// \file OrderRequest.hpp
/// \brief Defines the OrderRequest class, representing a low-level order instruction for exchanges.

namespace dfh {

    /// \class OrderRequest
    /// \brief Represents a low-level order request sent directly to the exchange.
    class OrderRequest {
    public:
        // --- Core identifiers ---
        std::string symbol;              ///< Trading symbol (e.g., "BTC/USDT").
        std::string order_link_id;      ///< Optional client-assigned order link ID.

        int64_t unique_id = 0;          ///< Internal unique ID for the order.
        int64_t account_id = 0;         ///< Associated account ID.

        // --- Trading configuration ---
        OrderSide side = OrderSide::UNKNOWN;           ///< Buy or Sell.
        OrderType order_type = OrderType::UNKNOWN;     ///< Order type (e.g., MARKET, LIMIT).
        TradeMode trade_mode = TradeMode::UNKNOWN;     ///< Spot, Futures, etc.
        AccountType account_type = AccountType::UNKNOWN; ///< Real or Demo.
        std::string currency;                          ///< Margin/settlement currency.

        // --- Order values ---
        double quantity = 0.0;         ///< Quantity in base currency.
        double price = 0.0;            ///< Limit price.
        double trigger_price = 0.0;    ///< Trigger price for stop/conditional orders.
        double sl_limit_price = 0.0;   ///< Limit price for stop-loss.
        double tp_limit_price = 0.0;   ///< Limit price for take-profit.

        // --- Execution modifiers ---
        TimeInForce time_in_force = TimeInForce::UNKNOWN; ///< Time in force.
        bool reduce_only = false;      ///< Whether order can only reduce position.
        bool close_on_trigger = false; ///< Automatically closes on trigger.

        // --- Stop/TP/SL related ---
        OrderType sl_order_type = OrderType::UNKNOWN;  ///< SL order type (e.g., LIMIT or MARKET).
        OrderType tp_order_type = OrderType::UNKNOWN;  ///< TP order type (e.g., LIMIT or MARKET).
        PriceTriggerType trigger_price_type = PriceTriggerType::LAST_PRICE; ///< Trigger price reference.
        int trigger_direction = 0;      ///< 1 = rise above, 2 = fall below.
        TpSlMode tpsl_mode;         	///< Mode to submit TP/SL. Exchange-specific.

        // --- Advanced features ---
        QuantityUnit quantity_unit = QuantityUnit::UNKNOWN; ///< Quantity unit: base or quote (e.g., BTC or USDT).
        SmpType smp_type;          	   ///< Self-match prevention type (exchange-specific).
        bool mmp = false;              ///< Market maker protection (exchange-specific).

        // --- Optional expiration ---
        int64_t expiry_time = 0;       ///< Expiration timestamp (for GTT orders).

        /// \brief JSON serialization
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(
            OrderRequest,
            symbol,
            order_link_id,
            unique_id,
            account_id,
            side,
            order_type,
            trade_mode,
            account_type,
            currency,
            quantity,
            price,
            trigger_price,
            sl_limit_price,
            tp_limit_price,
            time_in_force,
            reduce_only,
            close_on_trigger,
            sl_order_type,
            tp_order_type,
            trigger_price_type,
            trigger_direction,
            tpsl_mode,
            quantity_unit,
            smp_type,
            mmp,
            expiry_time
        )
    };
	
    /// \brief Type alias for a unique pointer to `OrderRequest`.
    using order_request_t = std::unique_ptr<OrderRequest>;

    /// \brief Type alias for a order result callback function.
    using order_result_callback_t = std::function<void(std::unique_ptr<OrderRequest>, std::unique_ptr<OrderResult>)>;

} // namespace dfh

#endif // _DFH_ORDER_REQUEST_HPP_INCLUDED