#pragma once
#ifndef _DTH_FIXED_SPREAD_PROCESSOR_HPP_INCLUDED
#define _DTH_FIXED_SPREAD_PROCESSOR_HPP_INCLUDED

/// \file FixedSpreadProcessor.hpp
/// \brief Implementation of the fixed spread restoration processor.

namespace dfh::core {

    /// \class FixedSpreadProcessor
    /// \brief Processor for restoring bid/ask spreads with a fixed spread value.
    ///
    /// This processor assumes a constant spread value provided in the configuration
    /// and restores bid/ask prices based on the last trade price and tick flags.
    class FixedSpreadProcessor final : public ISpreadProcessor {
    public:

        /// \brief Processes ticks and restores bid/ask spreads with a fixed spread.
        ///
        /// This method processes the provided tick data, calculates bid/ask prices
        /// using a fixed spread value, segments the data into one-second intervals,
        /// and updates the buffer's state with the latest tick.
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
            const double spread = utils::pow10<double>(price_digits) * static_cast<double>(bidask_config.fixed_spread);

            uint64_t fragment_time_ms = start_time_ms + time_shield::MS_PER_SEC;
            MarketTick& tick = ticks[0];

            if (has_prev_data) {
                if (!utils::compare_with_precision(tick.last, prev_tick.last, price_digits)) {
                    tick.set_flag(TickUpdateFlags::LAST_UPDATED);
                }
            }

            if (tick.has_flag(TickUpdateFlags::TICK_FROM_BUY)) {
                tick.ask = tick.last;
                tick.bid = tick.ask - spread;
            } else
            if (tick.has_flag(TickUpdateFlags::TICK_FROM_SELL)) {
                tick.bid = tick.last;
                tick.ask = tick.bid + spread;
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

                if (tick.has_flag(TickUpdateFlags::LAST_UPDATED)) {
                    if (tick.has_flag(TickUpdateFlags::TICK_FROM_BUY)) {
                        tick.ask = tick.last;
                        tick.bid = tick.ask - spread;
                    } else
                    if (tick.has_flag(TickUpdateFlags::TICK_FROM_SELL)) {
                        tick.bid = tick.last;
                        tick.ask = tick.bid + spread;
                    } else {
                        throw std::runtime_error("Invalid tick type flags combination");
                    }
                    tick.set_flag(TickUpdateFlags::ASK_UPDATED);
                    tick.set_flag(TickUpdateFlags::BID_UPDATED);
                } else {
                    MarketTick& prev_tick = ticks[i - 1];
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
    };

};

#endif // _DTH_FIXED_SPREAD_PROCESSOR_HPP_INCLUDED
