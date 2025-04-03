#pragma once
#ifndef _DFH_DATA_TICK_SPAN_HPP_INCLUDED
#define _DFH_DATA_TICK_SPAN_HPP_INCLUDED

/// \file TickSpan.hpp
/// \brief

namespace dfh {

    /// \struct TickSpan
    /// \brief
    template <typename TickType>
    struct TickSpan {
        const TickType* data;   ///< Указатель на начало диапазона
        size_t size;            ///< Количество элементов в диапазоне

        TickSpan() : data(nullptr), size(0) {}

        TickSpan(const TickType* data, size_t size) : data(data), size(size) {}

        /// \brief
        /// \return
        bool empty() const { return size == 0; }
    };

    using ValueTickSpan = TickSpan<ValueTick>;
    using SimpleTickSpan = TickSpan<SimpleTick>;
    using MarketTickSpan = TickSpan<MarketTick>;
};

#endif // _DFH_DATA_TICK_SPAN_HPP_INCLUDED
