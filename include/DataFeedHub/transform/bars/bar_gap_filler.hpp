#ifndef _DFH_TRANSFORM_BAR_GAP_FILLER_HPP_INCLUDED
#define _DFH_TRANSFORM_BAR_GAP_FILLER_HPP_INCLUDED

/// \file bar_gap_filler.hpp
/// \brief Provides functions for filling gaps in bar sequences (both in-place and copy-based).

namespace dfh::transform {

    /// \brief Fills missing bars in a sequence by creating flat bars with zero volume.
    /// \details Returns a new vector with gaps filled between the start and end times.
    /// \param bars A sorted vector of MarketBar objects (sorted by the time_ms field).
    /// \param bar_interval_ms Duration of a single bar in milliseconds.
    /// \param start_time_ms Start time of the range (inclusive) in milliseconds.
    /// \param end_time_ms End time of the range (exclusive) in milliseconds.
    /// \return A new vector of MarketBar objects with gaps filled.
    std::vector<MarketBar> fill_missing_bars(
            const std::vector<MarketBar>& bars,
            uint64_t bar_interval_ms,
            uint64_t start_time_ms,
            uint64_t end_time_ms) {
        std::vector<MarketBar> result;
        if (bars.empty()) return result;

        const size_t estimated_size = (end_time_ms - start_time_ms) / bar_interval_ms + 1;
        result.reserve(estimated_size);

        size_t index = 0;
        uint64_t expected_time = start_time_ms;
        uint32_t last_spread = bars.front().spread;

        while (expected_time < end_time_ms) {
            if (index < bars.size() && bars[index].time_ms == expected_time) {
                result.push_back(bars[index]);
                last_spread = bars[index].spread;
                ++index;
            } else {
                double price = (index > 0) ? bars[index - 1].close : bars.front().close;

                MarketBar filler;
                filler.time_ms = expected_time;
                filler.open = filler.high = filler.low = filler.close = price;
                filler.spread = last_spread;

                result.push_back(filler);
            }
            expected_time += bar_interval_ms;
        }

        return result;
    }

    /// \brief Fills missing bars directly into the provided vector (in-place).
    /// \details Modifies the input vector by inserting placeholders for all missing bars.
    /// \param bars A vector of MarketBar objects (must be sorted by the time_ms field).
    /// \param bar_interval_ms Duration of a single bar in milliseconds.
    /// \param start_time_ms Start time of the range (inclusive) in milliseconds.
    /// \param end_time_ms End time of the range (exclusive) in milliseconds.
    void fill_missing_bars_inplace(
            std::vector<MarketBar>& bars,
            uint64_t bar_interval_ms,
            uint64_t start_time_ms,
            uint64_t end_time_ms) {
        if (bars.empty()) return;
        std::vector<MarketBar> missing;

        // Fill missing bars before the first bar
        uint64_t expected_time = start_time_ms;
        const MarketBar& first = bars.front();
        while (expected_time < first.time_ms) {
            MarketBar filler;
            filler.time_ms = expected_time;
            filler.open = filler.high = filler.low = filler.close = first.open;
            filler.spread = first.spread;
            missing.push_back(filler);
            expected_time += bar_interval_ms;
        }

        if (!missing.empty()) {
            bars.insert(bars.begin(), missing.begin(), missing.end());
            missing.clear();
        }

        // Fill internal gaps
        for (size_t i = 1; i < bars.size(); ++i) {
            uint64_t expected = bars[i - 1].time_ms + bar_interval_ms;
            while (expected < bars[i].time_ms) {
                MarketBar filler;
                filler.time_ms = expected;
                filler.open = filler.high = filler.low = filler.close = bars[i - 1].close;
                filler.spread = bars[i - 1].spread;
                bars.insert(bars.begin() + static_cast<long>(i), filler);
                ++i;
                expected += bar_interval_ms;
            }
        }

        // Fill missing bars after the last bar
        const MarketBar& last = bars.back();
        expected_time = last.time_ms + bar_interval_ms;
        while (expected_time < end_time_ms) {
            MarketBar filler;
            filler.time_ms = expected_time;
            filler.open = filler.high = filler.low = filler.close = last.close;
            filler.spread = last.spread;
            bars.emplace_back(filler);
            expected_time += bar_interval_ms;
        }
    }

    /// \brief Fills missing bars (copy-based version) using TimeFrame enum.
    /// \details Returns a new vector with gaps filled between the start and end times.
    /// \param bars Input vector.
    /// \param time_frame TimeFrame enum value.
    /// \param start_time_ms Start of time range (inclusive).
    /// \param end_time_ms End of time range (exclusive).
    /// \return Vector of MarketBars with gaps filled.
    inline std::vector<MarketBar> fill_missing_bars(
            const std::vector<MarketBar>& bars,
            TimeFrame time_frame,
            uint64_t start_time_ms,
            uint64_t end_time_ms) {
        return fill_missing_bars(bars, time_shield::sec_to_ms(static_cast<uint64_t>(time_frame)), start_time_ms, end_time_ms);
    }

    /// \brief Fills missing bars in-place using TimeFrame enum.
    /// \details Modifies the input vector by inserting placeholders for all missing bars.
    /// \param bars Input/output vector.
    /// \param time_frame TimeFrame enum value.
    /// \param start_time_ms Start of time range (inclusive).
    /// \param end_time_ms End of time range (exclusive).
    inline void fill_missing_bars_inplace(
            std::vector<MarketBar>& bars,
            TimeFrame time_frame,
            uint64_t start_time_ms,
            uint64_t end_time_ms) {
        fill_missing_bars_inplace(bars, time_shield::sec_to_ms(static_cast<uint64_t>(time_frame)), start_time_ms, end_time_ms);
    }

} // namespace dfh::transform


#endif // _DFH_TRANSFORM_BAR_GAP_FILLER_HPP_INCLUDED
