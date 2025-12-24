#pragma once
#ifndef _DFH_COMPRESSION_TICK_COMPRESSOR_V1_HPP_INCLUDED
#define _DFH_COMPRESSION_TICK_COMPRESSOR_V1_HPP_INCLUDED

/// \file TickCompressorV1.hpp
/// \brief Provides functionality for compressing and decompressing market tick data.

#include "DataFeedHub/data/ticks/QuoteTick.hpp"
#include "DataFeedHub/data/ticks/QuoteTickVol.hpp"
#include "DataFeedHub/data/ticks/QuoteTickL1.hpp"
#include "DataFeedHub/data/ticks/TradeTick.hpp"
#include "DataFeedHub/data/ticks/QuoteTickConversions.hpp"
#include "TradeIdCodec.hpp"
#include "TickCompressorV1/TickCompressionContextV1.hpp"
#include "TickCompressorV1/TickEncoderV1.hpp"
#include "TickCompressorV1/TickDecoderV1.hpp"
#include "TickCompressorV1/zstd_dict_102400.hpp"

namespace dfh::compression {

    /// \class TickCompressorV1
    /// \brief Implements tick data compression and decompression using ZSTD with a custom dictionary.
    ///
    /// This class is specifically designed to handle market tick data based on trade information.
    /// It supports data where the following fields are present:
    /// - **Price (`last`)**: The last known trade price.
    /// - **Volume**: The trade volume.
    /// - **Side (`side`)**: Direction of the trade, indicating whether the trade was initiated by a buyer (buy) or a seller (sell).
    /// - **Last update flag**: Indicates if the last price was updated during the trade.
    ///
    /// \note This compressor is not suitable for tick data that lacks these fields, such as order book updates
    /// or trades without directional information.
    class TickCompressorV1 final : public ITickSerializer {
    public:

        /// \brief Constructs the TickCompressorV1 object.
        /// Initializes the compression context and reserves memory for intermediate buffers.
        TickCompressorV1()
            : m_context(), m_encoder(m_context), m_decoder(m_context) {
        }

        /// \copydoc ITickSerializer::set_codec_config()
        void set_codec_config(const TickCodecConfig& config) override final {
            m_config = config;
        }

        /// \copydoc ITickSerializer::codec_config()
        const TickCodecConfig& codec_config() const override final {
            return m_config;
        }

        /// \copydoc ITickSerializer::is_valid_signature()
		bool is_valid_signature(const std::vector<uint8_t>& input) const override final {
            if (input.empty()) return false; // No data to check.
            constexpr uint8_t signature = 0x01;
            return input[0] == signature;
        }

		/// \copydoc ITickSerializer::serialize(const std::vector<MarketTick>&, std::vector<uint8_t>&)
        /// \throw std::invalid_argument if the configuration is invalid (e.g., precision exceeds allowed digits).
        void serialize(
            const std::vector<MarketTick>& ticks,
            std::vector<uint8_t>& output) override final {
            compress(ticks, output);
        }

        /// \copydoc ITickSerializer::serialize(const std::vector<MarketTick>&, const TickCodecConfig&, std::vector<uint8_t>&)
        /// \throw std::invalid_argument if the configuration is invalid (e.g., precision exceeds allowed digits).
        void serialize(
            const std::vector<MarketTick>& ticks,
            const TickCodecConfig& config,
            std::vector<uint8_t>& output) override final {
            set_codec_config(config);
            compress(ticks, output);
        }

        /// \copydoc ITickSerializer::deserialize(const std::vector<uint8_t>&, std::vector<MarketTick>&)
        /// \throw std::runtime_error if decompression fails.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<MarketTick>& ticks) override final {
            decompress(input, ticks);
        }

        /// \copydoc ITickSerializer::deserialize(const std::vector<uint8_t>&, std::vector<MarketTick>&, TickCodecConfig&)
        /// \throw std::runtime_error if decompression fails.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<MarketTick>& ticks,
            TickCodecConfig& config) override final {
            decompress(input, ticks);
            config = m_config;
        }

        /// \brief Serializes QuoteTick data into a binary format.
        void serialize(
            const std::vector<QuoteTick>& ticks,
            std::vector<uint8_t>& output) override final {
            serialize_quote(ticks, output, false, false);
        }

        /// \brief Serializes QuoteTick data with a configuration.
        void serialize(
            const std::vector<QuoteTick>& ticks,
            const TickCodecConfig& config,
            std::vector<uint8_t>& output) override final {
            serialize_quote(ticks, config, output, false, false);
        }

        /// \brief Deserializes QuoteTick data from binary format.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<QuoteTick>& ticks) override final {
            deserialize_quote(input, ticks);
        }

        /// \brief Deserializes QuoteTick data and retrieves the configuration.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<QuoteTick>& ticks,
            TickCodecConfig& config) override final {
            deserialize_quote(input, ticks, config);
        }

        /// \brief Serializes QuoteTickVol data into a binary format.
        void serialize(
            const std::vector<QuoteTickVol>& ticks,
            std::vector<uint8_t>& output) override final {
            serialize_quote(ticks, output, true, false);
        }

        /// \brief Serializes QuoteTickVol data with a configuration.
        void serialize(
            const std::vector<QuoteTickVol>& ticks,
            const TickCodecConfig& config,
            std::vector<uint8_t>& output) override final {
            serialize_quote(ticks, config, output, true, false);
        }

        /// \brief Deserializes QuoteTickVol data from binary format.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<QuoteTickVol>& ticks) override final {
            deserialize_quote(input, ticks);
        }

        /// \brief Deserializes QuoteTickVol data and retrieves the configuration.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<QuoteTickVol>& ticks,
            TickCodecConfig& config) override final {
            deserialize_quote(input, ticks, config);
        }

        /// \brief Serializes QuoteTickL1 data into a binary format.
        void serialize(
            const std::vector<QuoteTickL1>& ticks,
            std::vector<uint8_t>& output) override final {
            serialize_quote(ticks, output, true, true);
        }

        /// \brief Serializes QuoteTickL1 data with a configuration.
        void serialize(
            const std::vector<QuoteTickL1>& ticks,
            const TickCodecConfig& config,
            std::vector<uint8_t>& output) override final {
            serialize_quote(ticks, config, output, true, true);
        }

        /// \brief Deserializes QuoteTickL1 data from binary format.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<QuoteTickL1>& ticks) override final {
            deserialize_quote(input, ticks);
        }

        /// \brief Deserializes QuoteTickL1 data and retrieves the configuration.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<QuoteTickL1>& ticks,
            TickCodecConfig& config) override final {
            deserialize_quote(input, ticks, config);
        }

        /// \brief Serializes TradeTick data into a binary format.
        void serialize(
            const std::vector<TradeTick>& ticks,
            std::vector<uint8_t>& output) override final {
            serialize_quote(ticks, output, true, false);
        }

        /// \brief Serializes TradeTick data with a configuration.
        void serialize(
            const std::vector<TradeTick>& ticks,
            const TickCodecConfig& config,
            std::vector<uint8_t>& output) override final {
            serialize_quote(ticks, config, output, true, false);
        }

        /// \brief Deserializes TradeTick data from binary format.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<TradeTick>& ticks) override final {
            deserialize_quote(input, ticks);
        }

        /// \brief Deserializes TradeTick data and retrieves the configuration.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<TradeTick>& ticks,
            TickCodecConfig& config) override final {
            deserialize_quote(input, ticks, config);
        }

    private:

        template<typename QuoteType>
        void serialize_quote(
            const std::vector<QuoteType>& ticks,
            std::vector<uint8_t>& output,
            bool force_volume,
            bool mark_l1) {

            if (ticks.empty()) return;
            std::vector<MarketTick> market_ticks;
            market_ticks.reserve(ticks.size());
            std::vector<uint64_t> trade_ids;
            trade_ids.reserve(ticks.size());
            fill_market_ticks(ticks, market_ticks, trade_ids);

            const bool has_trade_ids = !trade_ids.empty();
            const TickCodecConfig original_config = m_config;
            const TickCodecConfig adjusted_config = prepare_quote_config(original_config, force_volume, mark_l1, has_trade_ids);
            try {
                compress(market_ticks, output, has_trade_ids ? &trade_ids : nullptr);
            } catch (...) {
                m_config = original_config;
                throw;
            }
            m_config = original_config;
        }

        template<typename QuoteType>
        void serialize_quote(
            const std::vector<QuoteType>& ticks,
            const TickCodecConfig& config,
            std::vector<uint8_t>& output,
            bool force_volume,
            bool mark_l1) {

            if (ticks.empty()) {
                set_codec_config(config);
                return;
            }
            std::vector<MarketTick> market_ticks;
            market_ticks.reserve(ticks.size());
            std::vector<uint64_t> trade_ids;
            trade_ids.reserve(ticks.size());
            fill_market_ticks(ticks, market_ticks, trade_ids);

            const bool has_trade_ids = !trade_ids.empty();
            const TickCodecConfig adjusted_config = prepare_quote_config(config, force_volume, mark_l1, has_trade_ids);
            try {
                compress(market_ticks, output, has_trade_ids ? &trade_ids : nullptr);
            } catch (...) {
                set_codec_config(config);
                throw;
            }
            set_codec_config(config);
        }

        template<typename QuoteType>
        void deserialize_quote(
            const std::vector<uint8_t>& input,
            std::vector<QuoteType>& ticks) {

            std::vector<MarketTick> market_ticks;
            std::vector<uint64_t> trade_ids;
            decompress(input, market_ticks, &trade_ids);
            append_quote_ticks(market_ticks, ticks, &trade_ids);
        }

        template<typename QuoteType>
        void deserialize_quote(
            const std::vector<uint8_t>& input,
            std::vector<QuoteType>& ticks,
            TickCodecConfig& config) {

            std::vector<MarketTick> market_ticks;
            std::vector<uint64_t> trade_ids;
            decompress(input, market_ticks, &trade_ids);
            append_quote_ticks(market_ticks, ticks, &trade_ids);
            config = m_config;
        }

        template<typename QuoteType>
        static void fill_market_ticks(
            const std::vector<QuoteType>& source,
            std::vector<MarketTick>& target,
            std::vector<uint64_t>& trade_ids) {

            target.clear();
            target.reserve(source.size());
            trade_ids.clear();
            trade_ids.reserve(source.size());

            for (const auto& quote : source) {
                target.push_back(dfh::QuoteTickConversion<QuoteType>::to(quote));
                dfh::QuoteTickConversion<QuoteType>::collect_trade_ids(quote, trade_ids);
            }
        }

        template<typename QuoteType>
        static void append_quote_ticks(
            const std::vector<MarketTick>& source,
            std::vector<QuoteType>& target,
            const std::vector<uint64_t>* trade_ids) {

            target.reserve(target.size() + source.size());
            for (size_t i = 0; i < source.size(); ++i) {
                const auto& tick = source[i];
                const std::uint64_t trade_id = (trade_ids && i < trade_ids->size()) ? (*trade_ids)[i] : 0;
                target.push_back(dfh::QuoteTickConversion<QuoteType>::from(tick, trade_id));
            }
        }

        static TickCodecConfig prepare_quote_config(
            const TickCodecConfig& base,
            bool force_volume,
            bool mark_l1,
            bool has_trade_ids) {

            TickCodecConfig cfg = base;
            cfg.set_flag(TickStorageFlags::ENABLE_TICK_FLAGS, false);
            if (force_volume) cfg.set_flag(TickStorageFlags::ENABLE_VOLUME, true);
            if (mark_l1) cfg.set_flag(TickStorageFlags::L1_TWO_VOLUMES, true);
            cfg.set_flag(TickStorageFlags::ENABLE_TRADE_ID, has_trade_ids);
            return cfg;
        }

        /// \brief Compresses market tick data.
        /// \param ticks A vector of MarketTick structures representing the tick data.
        /// \param output A vector where the compressed data will be stored.
        /// \throw std::invalid_argument if the configuration is invalid (e.g., precision exceeds allowed digits).
        void compress(
                const std::vector<MarketTick>& ticks,
                std::vector<uint8_t>& output,
                const std::vector<uint64_t>* trade_ids = nullptr) {
            if (ticks.empty()) return;
            if (!m_config.has_flag(TickStorageFlags::ENABLE_TICK_FLAGS)) {
                throw std::invalid_argument(
                    "Trade-based encoding is disabled in the configuration. "
                    "Ensure that `TRADE_BASED` is set in `TickStorageFlags` before calling compress()."
                );
            }

            constexpr uint16_t max_digits = 18;
            if (m_config.price_digits > max_digits ||
                m_config.volume_digits > max_digits) {
                throw std::invalid_argument("Price or volume digits exceed maximum allowed digits.");
            }

            m_context.reset();

            auto& buffer = m_context.processing_buffer;

            uint8_t header = 0x00;
            // Record the precision levels
            // Bits 0-4: Number of decimal places for the price
            // Bit 5: ENABLE_TICK_FLAGS — whether TickUpdateFlags are encoded
            // Bit 6: TRADE_BASED — trade-only ticks (only `last`, no bid/ask)
            // Bit 7: ENABLE_VOLUME — whether volume compression is enabled
            header |= (m_config.price_digits & 0x1F);
            header |= (m_config.has_flag(TickStorageFlags::ENABLE_TICK_FLAGS) << 5) & 0x20;
            header |= (m_config.has_flag(TickStorageFlags::TRADE_BASED) << 6) & 0x40;
            header |= (m_config.has_flag(TickStorageFlags::ENABLE_VOLUME) << 7) & 0x80;
            buffer.push_back(header);

            // Bits 0-4: Number of decimal places for the volume
            // Bit 5: Indicates if the first tick has the LAST_UPDATED flag set
            header = 0x00;
            header |= (m_config.volume_digits & 0x1F);
            header |= (ticks[0].has_flag(TickUpdateFlags::LAST_UPDATED) << 5) & 0x20;
            header |= (m_config.has_flag(TickStorageFlags::L1_TWO_VOLUMES) << 6) & 0x40;
            header |= (m_config.has_flag(TickStorageFlags::ENABLE_TRADE_ID) << 7) & 0x80;
            buffer.push_back(header);

            constexpr uint64_t interval_ms = 3600000ULL;
            const uint64_t base_unix_hour = (ticks[0].time_ms / interval_ms);
            const uint64_t base_unix_time = base_unix_hour * interval_ms;

            dfh::utils::append_vbyte<uint32_t>(buffer, base_unix_hour);
            dfh::utils::append_vbyte<uint64_t>(buffer, encode_zig_zag_int64((int64_t)m_config.expiration_time_ms - (int64_t)base_unix_time));
            dfh::utils::append_vbyte<uint64_t>(buffer, encode_zig_zag_int64((int64_t)m_config.next_expiration_time_ms - (int64_t)base_unix_time));

            const double price_scale = dfh::utils::pow10<double>(m_config.price_digits);
            const uint64_t initial_price = std::llround(ticks[0].last * price_scale);
            const uint64_t tick_size = std::llround(m_config.tick_size * price_scale);

            dfh::utils::append_vbyte<uint64_t>(buffer, initial_price);
            dfh::utils::append_vbyte<uint64_t>(buffer, tick_size);

            m_encoder.encode_price_last(
                buffer,
                ticks.data(),
                ticks.size(),
                price_scale,
                initial_price);

            if (m_config.has_flag(TickStorageFlags::ENABLE_VOLUME)) {
                m_encoder.encode_volume(
                    buffer,
                    ticks.data(),
                    ticks.size(),
                    dfh::utils::pow10<double>(m_config.volume_digits));
            }

            m_encoder.encode_time(
                buffer,
                ticks.data(),
                ticks.size(),
                base_unix_time);

            if (m_config.has_flag(TickStorageFlags::ENABLE_TRADE_ID) && trade_ids) {
                encode_trade_id_deltas(buffer, *trade_ids);
            }

            if (m_config.has_flag(TickStorageFlags::ENABLE_TICK_FLAGS)) {
                m_encoder.encode_side_flags(buffer, ticks.data(), ticks.size());
            }

            constexpr uint8_t signature = 0x01;
            compress_zstd_data(
                buffer.data(), buffer.size(),
                zstd_dict_tick_compressor_v1_102400,
                sizeof(zstd_dict_tick_compressor_v1_102400),
                signature,
                ticks.size(),
                output);
        }

        /// \brief Compresses market tick data with a specified configuration.
        /// \param ticks A vector of MarketTick structures.
        /// \param config The configuration to use for compression.
        /// \param output A vector where the compressed data will be stored.
        void compress(
                const std::vector<MarketTick>& ticks,
                const TickCodecConfig& config,
                std::vector<uint8_t>& output) {
            m_config = config;
            compress(ticks, output);
        }

        /// \brief Decompresses market tick data.
        /// \param input A vector of compressed data.
        /// \param ticks A vector where the decompressed MarketTick data will be stored.
        ///        The new ticks will be appended to the end of the vector without clearing existing data.
        /// \throw std::runtime_error if decompression fails.
        /// \note This function **appends** new ticks to `ticks` without clearing its contents.
        void decompress(
                const std::vector<uint8_t>& input,
                std::vector<MarketTick>& ticks,
                std::vector<uint64_t>* trade_ids = nullptr) {
            if (input.empty()) return;
            constexpr uint8_t signature = 0x01;
            if (input[0] != signature) {
                throw std::invalid_argument(
                    "Invalid data signature. The input data does not match the expected format. "
                    "Ensure that the data was compressed using the correct version of the compressor."
                );
            }

            size_t offset = 1;
            const size_t num_ticks = dfh::utils::extract_vbyte<uint32_t>(input.data(), offset);

            m_context.reset();
            auto& buffer = m_context.processing_buffer;

            decompress_zstd_data(
                input.data() + offset, input.size() - offset,
                zstd_dict_tick_compressor_v1_102400,
                sizeof(zstd_dict_tick_compressor_v1_102400),
                buffer);

            offset = 0;
            uint8_t header = buffer[offset++];
            m_config.flags = TickStorageFlags::NONE;
            m_config.price_digits = header & 0x1F;
            m_config.set_flag(TickStorageFlags::ENABLE_TICK_FLAGS, (header & 0x20) != 0);
            m_config.set_flag(TickStorageFlags::TRADE_BASED, (header & 0x40) != 0);
            const bool enable_volume = (header & 0x80) != 0;
            m_config.set_flag(TickStorageFlags::ENABLE_VOLUME, enable_volume);

            header = buffer[offset++];
            m_config.volume_digits  = header & 0x1F;
            const bool last_updated = (header & 0x20) != 0;
            m_config.set_flag(TickStorageFlags::L1_TWO_VOLUMES, (header & 0x40) != 0);
            m_config.set_flag(TickStorageFlags::ENABLE_TRADE_ID, (header & 0x80) != 0);

            constexpr uint64_t interval_ms = 3600000ULL;
            const uint64_t base_unix_hour = dfh::utils::extract_vbyte<uint32_t>(buffer.data(), offset);
            const uint64_t base_unix_time    = base_unix_hour * interval_ms;
            m_config.expiration_time_ms      = base_unix_time + decode_zig_zag_int64(dfh::utils::extract_vbyte<uint64_t>(buffer.data(), offset));
            m_config.next_expiration_time_ms = base_unix_time + decode_zig_zag_int64(dfh::utils::extract_vbyte<uint64_t>(buffer.data(), offset));

            const uint64_t initial_price  = dfh::utils::extract_vbyte<uint64_t>(buffer.data(), offset);
            const uint64_t tick_size      = dfh::utils::extract_vbyte<uint64_t>(buffer.data(), offset);
            const double   price_scale    = dfh::utils::pow10<double>(m_config.price_digits);
            m_config.tick_size            = price_scale == 0.0 ? 0.0 : (double)tick_size / price_scale;
            const size_t initial_size     = ticks.size();
            ticks.resize(initial_size + num_ticks);

            MarketTick* ticks_ptr = ticks.data() + initial_size;

            m_decoder.decode_price_last(
                ticks_ptr,
                buffer.data(),
                offset,
                num_ticks,
                price_scale,
                initial_price);

            if (enable_volume) {
                m_decoder.decode_volume(
                    ticks_ptr,
                    buffer.data(),
                    offset,
                    num_ticks,
                    dfh::utils::pow10<double>(m_config.volume_digits));
            }

            m_decoder.decode_time(
                ticks_ptr,
                buffer.data(),
                offset,
                num_ticks,
                base_unix_time);

            if (m_config.has_flag(TickStorageFlags::ENABLE_TRADE_ID)) {
                decode_trade_id_deltas(buffer.data(), offset, num_ticks, trade_ids);
            }

            if (m_config.has_flag(TickStorageFlags::ENABLE_TICK_FLAGS)) {
                m_decoder.decode_side_flags(
                    ticks_ptr,
                    buffer.data(),
                    offset,
                    num_ticks);
                if (last_updated) {
                    ticks[ticks.size() - num_ticks].set_flag(TickUpdateFlags::LAST_UPDATED);
                }
                if (enable_volume) {
                    for (size_t i = 0; i < num_ticks; ++i) {
                        ticks_ptr[i].flags |= TickUpdateFlags::VOLUME_UPDATED;
                    }
                }
            }
        }

        /// \brief Decompresses market tick data and retrieves the configuration.
        /// \param input A vector of compressed data.
        /// \param ticks A vector where the decompressed MarketTick data will be stored.
        ///        The new ticks will be appended to the end of the vector without clearing existing data.
        /// \param config A reference to store the retrieved configuration.
        /// \throw std::runtime_error if decompression fails.
        /// \note This function **appends** new ticks to `ticks` without clearing its contents.
        void decompress(
                const std::vector<uint8_t>& input,
                std::vector<MarketTick>& ticks,
                TickCodecConfig& config) {
            decompress(input, ticks);
            config = m_config;
        }

        TickCompressionContextV1  m_context; ///< Compression context containing intermediate buffers.
        TickEncoderV1             m_encoder; ///< Encoder for market tick data.
        TickDecoderV1             m_decoder; ///< Decoder for market tick data.
        TickCodecConfig           m_config;  ///< Configuration for encoding/decoding.
    }; // TickCompressorV1

}; // namespace dfh::compression

#endif // _DFH_COMPRESSION_TICK_COMPRESSOR_V1_HPP_INCLUDED
