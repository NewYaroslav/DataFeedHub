#pragma once
#ifndef _DFH_TRANSFORM_BAR_CROP_HPP_INCLUDED
#define _DFH_TRANSFORM_BAR_CROP_HPP_INCLUDED

/// \file bar_crop.hpp
/// \brief Provides utilities to crop MarketBar sequences by time range.

namespace dfh::transform {

    /// \brief Removes all MarketBars with time_ms less than start_time_ms (inclusive crop start).
    /// \param bars Vector of MarketBars to modify in place.
    /// \param start_time_ms Inclusive lower time bound in milliseconds.
    inline void crop_bars_before(std::vector<dfh::MarketBar>& bars, uint64_t start_time_ms) {
        bars.erase(
            std::remove_if(bars.begin(), bars.end(), [start_time_ms](const dfh::MarketBar& bar) {
                return bar.time_ms < start_time_ms;
            }),
            bars.end()
        );
    }

    /// \brief Removes all MarketBars with time_ms greater than or equal to end_time_ms (exclusive crop end).
    /// \param bars Vector of MarketBars to modify in place.
    /// \param end_time_ms Exclusive upper time bound in milliseconds.
    inline void crop_bars_after(std::vector<dfh::MarketBar>& bars, uint64_t end_time_ms) {
        bars.erase(
            std::remove_if(bars.begin(), bars.end(), [end_time_ms](const dfh::MarketBar& bar) {
                return bar.time_ms >= end_time_ms;
            }),
            bars.end()
        );
    }

} // namespace dfh::transform

#endif // _DFH_TRANSFORM_BAR_CROP_HPP_INCLUDED
