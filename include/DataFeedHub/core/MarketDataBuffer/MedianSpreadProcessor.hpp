#pragma once
#ifndef _DTH_MEDIAN_SPREAD_PROCESSOR_HPP_INCLUDED
#define _DTH_MEDIAN_SPREAD_PROCESSOR_HPP_INCLUDED

/// \file MedianSpreadProcessor.hpp
/// \brief Implementation of the median spread restoration processor.

namespace dfh::core {

    /// \class MedianSpreadProcessor
    /// \brief Processor for restoring bid/ask spreads using a median filter.
    ///
    /// This processor restores bid/ask prices based on the last trade price and tick flags,
    /// applying a median filter to smooth the calculated spread values.
    class MedianSpreadProcessor final : public ISpreadProcessor {
    public:

        virtual ~MedianSpreadProcessor() = default;

        /// \brief Processes ticks and restores bid/ask spreads using a median filter.
        ///
        /// This method processes the provided tick data, calculates bid/ask prices
        /// using a median filter for smoothing spreads, segments the data into
        /// one-second intervals, and updates the buffer's state with the latest tick.
        ///
        /// \param ticks Reference to the vector of market ticks to be processed.
        /// \param chunks Reference to the vector of chunk indices for tick segmentation.
        /// \param prev_tick Reference to the previous tick, used for continuity in processing.
        /// \param has_prev_data Reference to the flag indicating whether previous data exists.
        /// \param codec_config Reference to the tick codec configuration.
        /// \param bidask_config Reference to the bid/ask restoration configuration.
        /// \param start_time_ms Start time of the processing range in milliseconds.
        /// \param end_time_ms End time of the processing range in milliseconds.
        void process(
                std::vector<MarketTick>& ticks,
                std::vector<uint32_t>& chunks,
                MarketTick& prev_tick,
                bool& has_prev_data,
                const TickCodecConfig& codec_config,
                const BidAskRestoreConfig& bidask_config,
                uint64_t start_time_ms,
                uint64_t end_time_ms) override final {
            const size_t price_digits = bidask_config.price_digits
                ? bidask_config.price_digits
                : codec_config.price_digits;

            uint64_t fragment_time_ms = start_time_ms + time_shield::MS_PER_SEC;
            MarketTick& tick = ticks[0];

            if (has_prev_data) {
                if (!utils::compare_with_precision(tick.last, prev_tick.last, price_digits)) {
                    tick.set_flag(TickUpdateFlags::LAST_UPDATED);
                }
            }

            double spread = 0.0, filter_spread = 0.0;
            if (!has_prev_data) {
                filter_spread = utils::pow10<double>(price_digits) * static_cast<double>(bidask_config.fixed_spread);
                m_prev2_spread = m_prev_spread = filter_spread;
            } else {
                filter_spread = m_prev_spread;
            }

            if (tick.has_flag(TickUpdateFlags::TICK_FROM_BUY)) {
                if (has_prev_data && prev_tick.has_flag(TickUpdateFlags::TICK_FROM_SELL) && tick.last > prev_tick.last) {
                    spread = utils::normalize_double(tick.last - prev_tick.last, price_digits);
                    filter_spread = utils::median_filter(m_prev2_spread, m_prev_spread, spread);
                    m_prev2_spread = m_prev_spread;
                    m_prev_spread = spread;
                }
                tick.ask = tick.last;
                tick.bid = tick.ask - filter_spread;
            } else
            if (tick.has_flag(TickUpdateFlags::TICK_FROM_SELL)) {
                if (has_prev_data &&  prev_tick.has_flag(TickUpdateFlags::TICK_FROM_BUY) && tick.last < prev_tick.last) {
                    spread = utils::normalize_double(prev_tick.last - tick.last, price_digits);
                    filter_spread = utils::median_filter(m_prev2_spread, m_prev_spread, spread);
                    m_prev2_spread = m_prev_spread;
                    m_prev_spread = spread;
                }

                tick.bid = tick.last;
                tick.ask = tick.bid + filter_spread;
            } else {
                throw std::runtime_error("Invalid tick type flags combination");
            }
            if (tick.has_flag(TickUpdateFlags::LAST_UPDATED)) {
                tick.set_flag(TickUpdateFlags::ASK_UPDATED);
                tick.set_flag(TickUpdateFlags::BID_UPDATED);
            }

            size_t fragment = 1;
            while (tick.time_ms >= fragment_time_ms) {
                chunks[fragment++] = 0;
                fragment_time_ms += time_shield::MS_PER_SEC;
            }

            for (size_t i = 1; i < ticks.size(); ++i) {
                MarketTick& tick = ticks[i];
                MarketTick& prev_tick = ticks[i - 1];

                if (tick.has_flag(TickUpdateFlags::LAST_UPDATED)) {
                    if (tick.has_flag(TickUpdateFlags::TICK_FROM_BUY)) {
                        if (prev_tick.has_flag(TickUpdateFlags::TICK_FROM_SELL) && tick.last > prev_tick.last) {
                            spread = utils::normalize_double(tick.last - prev_tick.last, price_digits);
                            filter_spread = utils::median_filter(m_prev2_spread, m_prev_spread, spread);
                            m_prev2_spread = m_prev_spread;
                            m_prev_spread = spread;
                        }

                        tick.ask = tick.last;
                        tick.bid = tick.ask - filter_spread;
                    } else
                    if (tick.has_flag(TickUpdateFlags::TICK_FROM_SELL)) {
                        if (prev_tick.has_flag(TickUpdateFlags::TICK_FROM_BUY) && tick.last < prev_tick.last) {
                            spread = utils::normalize_double(prev_tick.last - tick.last, price_digits);
                            filter_spread = utils::median_filter(m_prev2_spread, m_prev_spread, spread);
                            m_prev2_spread = m_prev_spread;
                            m_prev_spread = spread;
                        }

                        tick.bid = tick.last;
                        tick.ask = tick.bid + filter_spread;
                    } else {
                        throw std::runtime_error("Invalid tick type flags combination");
                    }
                    tick.set_flag(TickUpdateFlags::ASK_UPDATED);
                    tick.set_flag(TickUpdateFlags::BID_UPDATED);
                } else {
                    tick.bid = prev_tick.bid;
                    tick.ask = prev_tick.ask;
                }

                while (tick.time_ms >= fragment_time_ms) {
                    chunks[fragment++] = i;
                    fragment_time_ms += time_shield::MS_PER_SEC;
                }
            }

            for (size_t i = fragment; i < chunks.size(); ++i) {
                chunks[i] = ticks.size() - 1;
            }

            prev_tick = ticks.back();
            has_prev_data = true;
        }

    private:
        double m_prev_spread  = 0.0;    ///< Value of the previous spread.
        double m_prev2_spread = 0.0;    ///< Value of the second-to-last spread.
    };

}

#endif // _DTH_MEDIAN_SPREAD_PROCESSOR_HPP_INCLUDED
