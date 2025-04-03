#pragma once
#ifndef _DFH_TRADE_RESULT_HPP_INCLUDED
#define _DFH_TRADE_RESULT_HPP_INCLUDED

/// \file TradeResult.hpp
/// \brief Contains the TradeResult class for forex and crypto trading.

namespace dfh {

    /// \class TradeResult
    /// \brief Represents the result of a trade request.
    class TradeResult {
    public:
        // Unique trade identifier
        uint64_t trade_id = 0;          ///< Unique internal trade ID.
		std::string exchange_trade_id;  ///< Trade ID assigned by the exchange.

        // Trade execution metadata
        TradeErrorCode error_code = TradeErrorCode::SUCCESS; ///< Error code.
        std::string error_desc;         ///< Human-readable error description.

        // Price information
        double requested_price = 0.0;   ///< Price at trade request submission.
        double entry_price = 0.0;       ///< Actual execution price.
        double close_request_price = 0.0; ///< Price at close request submission.
        double exit_price = 0.0;        ///< Price at trade closure.

        // Financials
        double amount = 0.0;            ///< Final executed amount.
        double commission_open = 0.0;   ///< Fee for opening.
        double commission_close = 0.0;  ///< Fee for closing.
        double gross_pnl = 0.0; 		///< Profit before fees and commissions.
		double net_pnl   = 0.0; 		///< Profit after all fees and commissions.

        // Balance tracking
        double balance_before = 0.0;      ///< Account balance before trade.
        double balance_after_open = 0.0;  ///< Balance after opening.
        double balance_after_close = 0.0; ///< Balance after closing.

        // Timing parameters (milliseconds)
        int64_t place_time = 0;        ///< When trade request was received.
        int64_t send_time = 0;         ///< When trade request was sent to the exchange.
        int64_t open_time = 0;         ///< When trade was executed.
        int64_t close_place_time = 0;  ///< When close request was received.
        int64_t close_send_time = 0;   ///< When close request was sent to the exchange.
        int64_t close_time = 0;        ///< When trade was closed.

        int64_t open_ping = 0;         ///< Latency measurement on open request.
        int64_t close_ping = 0;        ///< Latency measurement on close request.
        int64_t open_delay = 0;        ///< Delay in trade opening.
        int64_t close_delay = 0;       ///< Delay in trade closing.

        // Related orders
        std::vector<OrderRequest> order_requests; ///< All related order requests.
        std::vector<OrderResult> order_results;   ///< Results of executed orders.

        // State information
        TradeState trade_state = TradeState::UNKNOWN;  ///< Lifecycle state.

        /// \brief Creates a unique pointer to a copy of this TradeResult.
        std::unique_ptr<TradeResult> clone_unique() const {
            return std::make_unique<TradeResult>(*this);
        }

        /// \brief Creates a shared pointer to a copy of this TradeResult.
        std::shared_ptr<TradeResult> clone_shared() const {
            return std::make_shared<TradeResult>(*this);
        }

        /// \brief Destructor for `TradeResult`.
        virtual ~TradeResult() = default;

        /// \brief Defines the JSON serialization format.
        NLOHMANN_DEFINE_TYPE_INTRUSIVE(
            TradeResult,
            trade_id,
            exchange_trade_id,
            requested_price,
            entry_price,
            close_request_price,
            exit_price,
            amount,
            commission_open,
            commission_close,
            gross_pnl,
            net_pnl,
            balance_before,
            balance_after_open,
            balance_after_close,
            place_time,
            send_time,
            open_time,
            close_place_time,
            close_send_time,
            close_time,
            open_ping,
            close_ping,
            open_delay,
            close_delay,
            order_requests,
            order_results,
            trade_state
        )
    };

    /// \brief Alias for a unique pointer to TradeResult.
    using trade_result_t = std::unique_ptr<TradeResult>;

} // namespace dfh

#endif // _DFH_TRADE_RESULT_HPP_INCLUDED