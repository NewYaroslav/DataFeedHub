#pragma once
#ifndef _DTH_NONE_SPREAD_PROCESSOR_HPP_INCLUDED
#define _DTH_NONE_SPREAD_PROCESSOR_HPP_INCLUDED

/// \file NoneSpreadProcessor.hpp
/// \brief Implementation of the "none" spread processor.

namespace dfh::core {

    /// \class NoneSpreadProcessor
    /// \brief Processor for handling tick data without restoring bid/ask spreads.
    ///
    /// This processor assumes that bid/ask prices are already present in the tick data
    /// and does not apply any restoration algorithms. It segments the tick data into
    /// time-based chunks and updates the state of the buffer with the latest tick.
    class NoneSpreadProcessor final : public ISpreadProcessor {
    public:

        /// \brief Processes ticks and segments them into time-based chunks.
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

            if (has_prev_data) {
                if (!utils::compare_with_precision(ticks[0].last, prev_tick.last, price_digits)) {
                    ticks[0].set_flag(TickUpdateFlags::LAST_UPDATED);
                }
            }

            size_t fragment = 1;
            for (size_t i = 0; i < ticks.size(); ++i) {
                while (ticks[i].time_ms >= fragment_time_ms) {
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

#endif // _DTH_NONE_SPREAD_PROCESSOR_HPP_INCLUDED
