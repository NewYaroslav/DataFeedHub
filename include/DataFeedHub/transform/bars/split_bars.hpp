#pragma once
#ifndef _DFH_TRANSFORM_SPLIT_BARS_HPP_INCLUDED
#define _DFH_TRANSFORM_SPLIT_BARS_HPP_INCLUDED

/// \file split_bars.hpp
/// \brief Provides a utility function for splitting bars into time-based segments.

namespace dfh::transform {

    /// \brief Splits a sequence of MarketBars into segments by their segment duration.
    /// \param time_frame TimeFrame enum value used to determine segment duration.
    /// \param bars Input vector of MarketBars sorted by time.
    /// \param out_segments Output vector where each segment is a vector of MarketBars.
    /// \return True if the bars were sorted and segmentation succeeded, false if input is unsorted.
    inline bool split_bars(
            dfh::TimeFrame time_frame,
            const std::vector<dfh::MarketBar>& bars,
            std::vector<std::vector<dfh::MarketBar>>& out_segments) {
        if (bars.empty()) return true;

        const uint64_t duration_ms = dfh::get_segment_duration_ms(time_frame);
        uint64_t next_time_ms = time_shield::start_of_period(duration_ms, bars[0].time_ms) + duration_ms;

        std::vector<dfh::MarketBar> current_segment;
        current_segment.reserve(bars.size());
        current_segment.push_back(bars[0]);

        for (size_t i = 1; i < bars.size(); ++i) {
            const MarketBar& bar = bars[i];
            if (bar.time_ms < bars[i - 1].time_ms) {
                return false;
            }

            if (bar.time_ms >= next_time_ms) {
                out_segments.push_back(std::move(current_segment));
                current_segment.clear();
                next_time_ms = time_shield::start_of_period(duration_ms, bar.time_ms) + duration_ms;
            }
            current_segment.push_back(bar);
        }

        if (!current_segment.empty()) {
            out_segments.push_back(std::move(current_segment));
        }

        return true;
    }

} // namespace dfh::transform

#endif // _DFH_TRANSFORM_SPLIT_BARS_HPP_INCLUDED
