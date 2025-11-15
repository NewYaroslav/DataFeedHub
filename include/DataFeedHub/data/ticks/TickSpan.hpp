#pragma once
#ifndef _DFH_DATA_TICK_SPAN_HPP_INCLUDED
#define _DFH_DATA_TICK_SPAN_HPP_INCLUDED

/// \file TickSpan.hpp
/// \brief Non-owning span for contiguous tick ranges

namespace dfh {

    /// \struct TickSpan
    /// \brief Non-owning view over a contiguous range of ticks
	/// Важно! span только для чтени
    template <typename TickType>
    struct TickSpan {
        const TickType* data;	///< Pointer to the first element
        size_t size;            ///< Number of elements

		/// \brief
		constexpr TickSpan() noexcept : data(nullptr), size(0) {}

		/// \brief
		/// \param data
		/// \param size
		constexpr TickSpan(const TickType* data, std::size_t size) noexcept
			: data(data), size(size) {}

        /// \brief Checks if span is empty
        /// \return
		constexpr bool empty() const noexcept { return size == 0; }
		
		/// \brief Begin iterator
		constexpr const TickType* begin() const noexcept { return data; }
		
		/// \brief End iterator (one-past-last
		constexpr const TickType* end()   const noexcept { return data + size; }

		/// \brief Access element by index (no bounds check)
		constexpr const TickType& operator[](std::size_t i) const noexcept {
			return data[i];
		}
    };

    using ValueTickSpan = TickSpan<ValueTick>;   ///< 
    using QuoteTickSpan = TickSpan<QuoteTick>;  ///< 
    using MarketTickSpan = TickSpan<MarketTick>; ///< 
}

#endif // _DFH_DATA_TICK_SPAN_HPP_INCLUDED
