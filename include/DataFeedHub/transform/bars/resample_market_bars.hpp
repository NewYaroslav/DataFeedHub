#pragma once
#ifndef _DFH_TRANSFORM_BARS_RESAMPLE_MARKET_BARS_HPP_INCLUDED
#define _DFH_TRANSFORM_BARS_RESAMPLE_MARKET_BARS_HPP_INCLUDED

/// \file resample_market_bars.hpp
/// \brief Provides functions for resampling MarketBars to a higher timeframe.

namespace dfh::transform {

    /// \brief Resamples MarketBars using the "last spread" strategy.
    /// \details Aggregates bars into a higher timeframe, setting the spread of each resampled bar
    ///          to the last known spread within its group.
    /// \param bars Input vector of MarketBars sorted by time (no gaps).
    /// \param target_interval_ms Target bar duration in milliseconds.
    /// \return Vector of resampled MarketBars with spread taken from the last sub-bar in each group.
    inline std::vector<MarketBar> resample_market_bars_last(
            const std::vector<MarketBar>& bars,
            uint64_t target_interval_ms) {
        std::vector<MarketBar> result;
        result.reserve(bars.size() / 2);

        const auto& first_bar = bars[0];
        uint64_t current_bucket = (first_bar.time_ms / target_interval_ms) * target_interval_ms;
        uint64_t next_bucket = current_bucket + target_interval_ms;

        MarketBar current;
        current.time_ms = current_bucket;
        current.open = first_bar.open;
        current.high = first_bar.high;
        current.low = first_bar.low;
        current.close = first_bar.close;
        current.volume = first_bar.volume;
        current.quote_volume = first_bar.quote_volume;
        current.buy_volume = first_bar.buy_volume;
        current.buy_quote_volume = first_bar.buy_quote_volume;
        current.tick_volume = first_bar.tick_volume;
        current.spread = first_bar.spread;

        for (size_t i = 1; i < bars.size(); ++i) {
            const auto& bar = bars[i];
            if (bar.time_ms == next_bucket) {
                result.push_back(current);
                current.time_ms = next_bucket;
                current.open = bar.open;
                current.high = bar.high;
                current.low = bar.low;
                current.close = bar.close;
                current.volume = bar.volume;
                current.quote_volume = bar.quote_volume;
                current.buy_volume = bar.buy_volume;
                current.buy_quote_volume = bar.buy_quote_volume;
                current.tick_volume = bar.tick_volume;
                current.spread = bar.spread;

                next_bucket += target_interval_ms;
            } else {
                current.high = std::max(current.high, bar.high);
                current.low = std::min(current.low, bar.low);
                current.close = bar.close;
                current.volume += bar.volume;
                current.quote_volume += bar.quote_volume;
                current.buy_volume += bar.buy_volume;
                current.buy_quote_volume += bar.buy_quote_volume;
                current.tick_volume += bar.tick_volume;
                current.spread = bar.spread;
            }
        }

        result.push_back(current);
        return result;
    }

    /// \brief Resamples MarketBars using the "max spread" strategy.
    /// \details Aggregates bars into a higher timeframe, setting the spread of each resampled bar
    ///          to the maximum spread among its constituent sub-bars.
    /// \param bars Input vector of MarketBars sorted by time (no gaps).
    /// \param target_interval_ms Target bar duration in milliseconds.
    /// \return Vector of resampled MarketBars with maximum spread per interval.
    inline std::vector<MarketBar> resample_market_bars_max(
            const std::vector<MarketBar>& bars,
            uint64_t target_interval_ms) {
        std::vector<MarketBar> result;
        result.reserve(bars.size() / 2);

        const auto& first_bar = bars[0];
        uint64_t current_bucket = (first_bar.time_ms / target_interval_ms) * target_interval_ms;
        uint64_t next_bucket = current_bucket + target_interval_ms;

        MarketBar current;
        current.time_ms = current_bucket;
        current.open = first_bar.open;
        current.high = first_bar.high;
        current.low = first_bar.low;
        current.close = first_bar.close;
        current.volume = first_bar.volume;
        current.quote_volume = first_bar.quote_volume;
        current.buy_volume = first_bar.buy_volume;
        current.buy_quote_volume = first_bar.buy_quote_volume;
        current.tick_volume = first_bar.tick_volume;
        current.spread = first_bar.spread;

        for (size_t i = 1; i < bars.size(); ++i) {
            const auto& bar = bars[i];
            if (bar.time_ms == next_bucket) {
                result.push_back(current);
                current.time_ms = next_bucket;
                current.open = bar.open;
                current.high = bar.high;
                current.low = bar.low;
                current.close = bar.close;
                current.volume = bar.volume;
                current.quote_volume = bar.quote_volume;
                current.buy_volume = bar.buy_volume;
                current.buy_quote_volume = bar.buy_quote_volume;
                current.tick_volume = bar.tick_volume;
                current.spread = bar.spread;

                next_bucket += target_interval_ms;
            } else {
                current.high = std::max(current.high, bar.high);
                current.low = std::min(current.low, bar.low);
                current.close = bar.close;
                current.volume += bar.volume;
                current.quote_volume += bar.quote_volume;
                current.buy_volume += bar.buy_volume;
                current.buy_quote_volume += bar.buy_quote_volume;
                current.tick_volume += bar.tick_volume;
                current.spread = std::max(current.spread, bar.spread);
            }
        }

        result.push_back(current);
        return result;
    }

    /// \brief Resamples MarketBars using the "average spread" strategy.
    /// \details Aggregates bars into a higher timeframe, calculating the average spread
    ///          from all sub-bars within each group.
    /// \param bars Input vector of MarketBars sorted by time (no gaps).
    /// \param target_interval_ms Target bar duration in milliseconds.
    /// \return Vector of resampled MarketBars with averaged spread values.
    static std::vector<MarketBar> resample_market_bars_avg(
            const std::vector<MarketBar>& bars,
            uint64_t target_interval_ms) {
        std::vector<MarketBar> result;
        result.reserve(bars.size() / 2);

        const auto& first_bar = bars[0];
        uint64_t current_bucket = (first_bar.time_ms / target_interval_ms) * target_interval_ms;
        uint64_t next_bucket = current_bucket + target_interval_ms;

        MarketBar current;
        current.time_ms = current_bucket;
        current.open = first_bar.open;
        current.high = first_bar.high;
        current.low = first_bar.low;
        current.close = first_bar.close;
        current.volume = first_bar.volume;
        current.quote_volume = first_bar.quote_volume;
        current.buy_volume = first_bar.buy_volume;
        current.buy_quote_volume = first_bar.buy_quote_volume;
        current.tick_volume = first_bar.tick_volume;

        uint32_t spread_sum = first_bar.spread;
        uint32_t spread_count = 1;

        for (size_t i = 1; i < bars.size(); ++i) {
            const auto& bar = bars[i];
            if (bar.time_ms == next_bucket) {
                current.spread = spread_count ? (spread_sum / spread_count) : 0;
                result.push_back(current);
                current.time_ms = next_bucket;
                current.open = bar.open;
                current.high = bar.high;
                current.low = bar.low;
                current.close = bar.close;
                current.volume = bar.volume;
                current.quote_volume = bar.quote_volume;
                current.buy_volume = bar.buy_volume;
                current.buy_quote_volume = bar.buy_quote_volume;
                current.tick_volume = bar.tick_volume;

                spread_sum = bar.spread;
                spread_count = 1;

                next_bucket += target_interval_ms;
            } else {
                current.high = std::max(current.high, bar.high);
                current.low = std::min(current.low, bar.low);
                current.close = bar.close;
                current.volume += bar.volume;
                current.quote_volume += bar.quote_volume;
                current.buy_volume += bar.buy_volume;
                current.buy_quote_volume += bar.buy_quote_volume;
                current.tick_volume += bar.tick_volume;

                spread_sum += bar.spread;
                ++spread_count;
            }
        }

        current.spread = spread_count ? (spread_sum / spread_count) : 0;
        result.push_back(current);
        return result;
    }

    /// \brief Resamples MarketBars without including spread data.
    /// \details Aggregates bars into a higher timeframe, omitting spread information.
    ///          Spread values in output bars will be uninitialized (zero).
    /// \param bars Input vector of MarketBars sorted by time (no gaps).
    /// \param target_interval_ms Target bar duration in milliseconds.
    /// \return Vector of resampled MarketBars without spread.
    static std::vector<MarketBar> resample_market_bars_no_spread(
            const std::vector<MarketBar>& bars,
            uint64_t target_interval_ms) {
        std::vector<MarketBar> result;
        result.reserve(bars.size() / 2);

        const auto& first_bar = bars[0];
        uint64_t current_bucket = (first_bar.time_ms / target_interval_ms) * target_interval_ms;
        uint64_t next_bucket = current_bucket + target_interval_ms;

        MarketBar current;
        current.time_ms = current_bucket;
        current.open = first_bar.open;
        current.high = first_bar.high;
        current.low = first_bar.low;
        current.close = first_bar.close;
        current.volume = first_bar.volume;
        current.quote_volume = first_bar.quote_volume;
        current.buy_volume = first_bar.buy_volume;
        current.buy_quote_volume = first_bar.buy_quote_volume;
        current.tick_volume = first_bar.tick_volume;

        for (size_t i = 1; i < bars.size(); ++i) {
            const auto& bar = bars[i];
            if (bar.time_ms == next_bucket) {
                result.push_back(current);
                current.time_ms = next_bucket;
                current.open = bar.open;
                current.high = bar.high;
                current.low = bar.low;
                current.close = bar.close;
                current.volume = bar.volume;
                current.quote_volume = bar.quote_volume;
                current.buy_volume = bar.buy_volume;
                current.buy_quote_volume = bar.buy_quote_volume;
                current.tick_volume = bar.tick_volume;

                next_bucket += target_interval_ms;
            } else {
                current.high = std::max(current.high, bar.high);
                current.low = std::min(current.low, bar.low);
                current.close = bar.close;
                current.volume += bar.volume;
                current.quote_volume += bar.quote_volume;
                current.buy_volume += bar.buy_volume;
                current.buy_quote_volume += bar.buy_quote_volume;
                current.tick_volume += bar.tick_volume;
            }
        }

        result.push_back(current);
        return result;
    }

    /// \brief Resamples a sequence of MarketBars to a higher timeframe.
    /// \details Selects the appropriate spread aggregation strategy based on BarCodecConfig flags.
    ///          Volume fields are aggregated based on what is enabled in the config.
    /// \param bars Input vector of MarketBars sorted by time (without gaps).
    /// \param target_interval_ms Duration of the target bar in milliseconds (must be greater than source bar interval).
    /// \param config Configuration object defining spread aggregation strategy and enabled volume fields.
    /// \return Vector of resampled MarketBars at the target interval.
    std::vector<MarketBar> resample_market_bars(
            const std::vector<MarketBar>& bars,
            uint64_t target_interval_ms,
            const BarCodecConfig& config) {
        if (bars.empty()) return {};

        if (!config.has_flag(BarStorageFlags::ENABLE_SPREAD)) {
            return resample_market_bars_no_spread(bars, target_interval_ms);
        }
        if (config.has_flag(BarStorageFlags::SPREAD_LAST)) {
            return resample_market_bars_last(bars, target_interval_ms);
        }
        if (config.has_flag(BarStorageFlags::SPREAD_MAX)) {
            return resample_market_bars_max(bars, target_interval_ms);
        }
        if (config.has_flag(BarStorageFlags::SPREAD_AVG)) {
            return resample_market_bars_avg(bars, target_interval_ms);
        }

        throw std::invalid_argument("Unsupported or unspecified spread aggregation mode.");
    }

    /// \brief Resamples a sequence of MarketBars to a higher timeframe.
    /// \details Uses the given TimeFrame to determine the target bar interval in milliseconds.
    ///          Selects the spread aggregation strategy and volume behavior from BarCodecConfig.
    /// \param bars Input vector of MarketBars sorted by time (without gaps).
    /// \param target_tf Target timeframe enum (e.g., H1, M15).
    /// \param config Configuration object defining spread aggregation strategy and enabled volume fields.
    /// \return Vector of resampled MarketBars at the given timeframe.
    inline std::vector<MarketBar> resample_market_bars(
            const std::vector<MarketBar>& bars,
            TimeFrame target_tf,
            const BarCodecConfig& config) {
        return resample_market_bars(
            bars,
            time_shield::sec_to_ms(static_cast<uint64_t>(target_tf)),
            config);
    }

} // namespace dfh::transform

#endif // _DFH_TRANSFORM_BARS_RESAMPLE_MARKET_BARS_HPP_INCLUDED
