#pragma once
#ifndef _DFH_ORDER_RESULT_HPP_INCLUDED
#define _DFH_ORDER_RESULT_HPP_INCLUDED

/// \file OrderResult.hpp
/// \brief Contains the OrderResult class representing individual order execution results.

namespace dfh {
	
	/// \class OrderResult
    /// \brief Represents the execution result of a single order.
    class OrderResult {
    public:
        // Order identifiers
        uint64_t order_id = 0;               ///< Internal order ID.
        std::string exchange_order_id;       ///< Exchange-assigned order ID.
        std::string client_order_id;         ///< Client-assigned order ID (if any).

        // Status and errors
        TradeErrorCode error_code = TradeErrorCode::SUCCESS; ///< Result status.
        std::string error_desc;               ///< Human-readable error message.

        // Execution state
        OrderType order_type = OrderType::UNKNOWN;   ///< Order type (market, limit, etc.).
        TradeState trade_state = TradeState::UNKNOWN;///< Order lifecycle state.

        // Price information
        double requested_price = 0.0;   ///< Requested price (for limit/stop orders).
        double executed_price  = 0.0;   ///< Actual execution price.

        // Financials
        double amount = 0.0;            ///< Executed amount (in base or quote asset).
        double commission = 0.0;        ///< Commission paid for this order.

        // Timing (milliseconds)
        int64_t place_time = 0;         ///< When order was created locally.
        int64_t send_time = 0;          ///< When order was sent to exchange.
        int64_t exec_time = 0;          ///< When order was executed.

        int64_t ping_time = 0;          ///< Round-trip latency measurement.
        int64_t delay = 0;              ///< Delay from placing to execution.

        // Order linkage (optional)
        std::string order_link_id;      ///< Optional client link ID for group/correlation.

        /// \brief Creates a unique pointer to a copy of this OrderResult.
        std::unique_ptr<OrderResult> clone_unique() const {
            return std::make_unique<OrderResult>(*this);
        }

        /// \brief Creates a shared pointer to a copy of this OrderResult.
        std::shared_ptr<OrderResult> clone_shared() const {
            return std::make_shared<OrderResult>(*this);
        }

        /// \brief Destructor.
        virtual ~OrderResult() = default;

        /// \brief JSON serialization format.
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(
            OrderResult,
            order_id,
            exchange_order_id,
            client_order_id,
            error_code,
            error_desc,
            order_type,
            trade_state,
            requested_price,
            executed_price,
            amount,
            commission,
            place_time,
            send_time,
            exec_time,
            ping_time,
            delay,
            order_link_id
        )
    };

    /// \brief Type alias for unique pointer to OrderResult.
    using order_result_t = std::unique_ptr<OrderResult>;

} // namespace dfh

#endif // _DFH_ORDER_RESULT_HPP_INCLUDED