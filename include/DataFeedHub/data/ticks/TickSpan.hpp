#pragma once
#ifndef _DFH_DATA_TICK_SPAN_HPP_INCLUDED
#define _DFH_DATA_TICK_SPAN_HPP_INCLUDED

/// \file TickSpan.hpp
/// \brief Non-owning span for contiguous tick ranges.

namespace dfh {

    /// \struct TickSpan
    /// \brief Lightweight read-only view over a contiguous range of ticks.
    template <typename TickType>
    struct TickSpan {
        const TickType* data{nullptr}; ///< Pointer to the first element.
        std::size_t size{0};           ///< Number of elements.

        /// \brief Default constructor creates an empty span.
        constexpr TickSpan() noexcept = default;

        /// \brief Constructs a span over a pointer and length.
        /// \param ptr Pointer to the first element.
        /// \param count Number of elements in the span.
        constexpr TickSpan(const TickType* ptr, std::size_t count) noexcept
            : data(ptr)
            , size(count) {}

        /// \brief Checks if span is empty.
        /// \return True when there are no elements.
        [[nodiscard]] constexpr bool empty() const noexcept { return size == 0; }

        /// \brief Begin iterator.
        [[nodiscard]] constexpr const TickType* begin() const noexcept { return data; }

        /// \brief End iterator (one-past-last element).
        [[nodiscard]] constexpr const TickType* end() const noexcept { return data + size; }

        /// \brief Access element by index (no bounds check).
        [[nodiscard]] constexpr const TickType& operator[](std::size_t i) const noexcept {
            return data[i];
        }
    };

    using ValueTickSpan = TickSpan<ValueTick>;    ///< Span over `ValueTick`.
    using QuoteTickSpan = TickSpan<QuoteTick>;    ///< Span over `QuoteTick`.
    using QuoteTickVolSpan = TickSpan<QuoteTickVol>; ///< Span over `QuoteTickVol`.
    using QuoteTickL1Span = TickSpan<QuoteTickL1>; ///< Span over `QuoteTickL1`.
    using MarketTickSpan = TickSpan<MarketTick>;  ///< Span over `MarketTick`.
    using TradeTickSpan = TickSpan<TradeTick>;    ///< Span over `TradeTick`.
}

#endif // _DFH_DATA_TICK_SPAN_HPP_INCLUDED
