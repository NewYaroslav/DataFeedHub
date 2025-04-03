#pragma once
#ifndef _DTH_DATA_BAR_SECQUENCE_HPP_INCLUDED
#define _DTH_DATA_BAR_SECQUENCE_HPP_INCLUDED

/// \file BarSequence.hpp
/// \brief Defines a generic template for holding a sequence of bar data with metadata.

namespace dfh {

    /// \struct BarSequence
    /// \brief Represents a sequence of market bars with additional metadata.
    ///
    /// This template structure holds a list of bars along with contextual
    /// metadata such as exchange/symbol identifiers, market type, precision,
    /// expiration time (for futures), and aggregation timeframe.
    ///
    /// \tparam BarType Type of the bars (e.g. OHLCBar, MarketBar).
    template <typename BarType>
    struct BarSequence {
        std::vector<BarType> bars;          ///< Sequence of bar data of the specified type.
        uint64_t   expiration_time_ms;      ///< Expiration time for futures (0 for perpetual or spot).
        uint64_t   next_expiration_time_ms; ///< Expiration time of the next contract (0 if not defined).
        double     tick_size;               ///< Minimum price increment (tick size).
        TimeFrame  time_frame;              ///< Timeframe used for aggregation.
        BarStatusFlags flags;               ///< Flags indicating the status of the bar sequence.
        uint16_t   symbol_id;               ///< Symbol identifier.
        uint16_t   exchange_id;             ///< Exchange identifier.
        MarketType market_type;             ///< Market type (spot, futures, etc.)
        uint8_t    price_digits;            ///< Number of decimal places for price.
        uint8_t    volume_digits;           ///< Number of decimal places for volume.
        uint8_t    quote_volume_digits;     ///< Number of decimal places for quote volume.

        /// \brief Default constructor initializes all fields to zero/default.
        BarSequence()
            : expiration_time_ms(0), next_expiration_time_ms(0), tick_size(0.0),
              time_frame(TimeFrame::UNKNOWN), flags(BarStatusFlags::NONE),
              symbol_id(0), exchange_id(0), market_type(MarketType::UNKNOWN),
              price_digits(0), volume_digits(0), quote_volume_digits(0) {}

        /// \brief Constructs a bar sequence with full metadata.
        /// \param bs Bar data.
        /// \param exp Expiration timestamp (in ms).
        /// \param next_exp Next contract's expiration timestamp (in ms).
        /// \param tick Tick size (minimum price step).
        /// \param tf Timeframe used for bars.
        /// \param f Bar status flags.
        /// \param sym_id Symbol identifier.
        /// \param exch_id Exchange identifier.
        /// \param mt Market type.
        /// \param pd Number of price decimals.
        /// \param vd Number of volume decimals.
        /// \param qvd Number of quote volume decimals.
        BarSequence(
            std::vector<BarType> bs,
            uint64_t exp,
            uint64_t next_exp,
            double tick,
            TimeFrame tf,
            BarStatusFlags f,
            uint16_t sym_id,
            uint16_t exch_id,
            MarketType mt,
            uint8_t pd,
            uint8_t vd,
            uint8_t qvd)
            : bars(std::move(bs)), expiration_time_ms(exp), next_expiration_time_ms(next_exp),
              tick_size(tick), time_frame(tf), flags(f),
              symbol_id(sym_id), exchange_id(exch_id), market_type(mt),
              price_digits(pd), volume_digits(vd), quote_volume_digits(qvd) {}
    };

    using OHLCBarSequence   = BarSequence<OHLCBar>;   ///< Bar sequence for OHLC-only bars.
    using OHLCVBarSequence  = BarSequence<OHLCVBar>;  ///< Bar sequence for OHLCV bars.
    using MarketBarSequence = BarSequence<MarketBar>; ///< Bar sequence for fully featured market bars.

} // namespace dfh

#endif // _DTH_DATA_BAR_SECQUENCE_HPP_INCLUDED
