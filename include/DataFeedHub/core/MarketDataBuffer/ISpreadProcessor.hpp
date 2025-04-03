#pragma once
#ifndef _DTH_ISPREAD_PROCESSOR_HPP_INCLUDED
#define _DTH_ISPREAD_PROCESSOR_HPP_INCLUDED

/// \file ISpreadProcessor.hpp
/// \brief Defines the interface for spread restoration processors.

namespace dfh::core {

    /// \class ISpreadProcessor
    /// \brief Interface for spread restoration processors.
    ///
    /// This interface provides the contract for implementing various spread
    /// restoration algorithms (e.g., fixed, dynamic, or median spread). Implementations
    /// of this interface will define the behavior for restoring bid/ask spreads from tick data.
    class ISpreadProcessor {
    public:

        /// \brief Virtual destructor for the interface.
        virtual ~ISpreadProcessor() = default;

        /// \brief Processes ticks and restores bid/ask spreads for the given data.
        /// \param ticks Reference to the vector of market ticks to be processed.
        /// \param chunks Reference to the vector of chunk indices for tick segmentation.
        /// \param prev_tick Reference to the previous tick, used for continuity in processing.
        /// \param has_prev_data Reference to the flag indicating whether previous data exists.
        /// \param codec_config Reference to the tick codec configuration.
        /// \param bidask_config Reference to the bid/ask restoration configuration.
        /// \param start_time_ms Start time of the processing range in milliseconds.
        /// \param end_time_ms End time of the processing range in milliseconds.
        virtual void process(
            std::vector<MarketTick>& ticks,
            std::vector<uint32_t>& chunks,
            MarketTick& prev_tick,
            bool& has_prev_data,
            const TickCodecConfig& codec_config,
            const BidAskRestoreConfig& bidask_config,
            uint64_t start_time_ms,
            uint64_t end_time_ms) = 0;
    };

};

#endif // _DTH_ISPREAD_PROCESSOR_HPP_INCLUDED
