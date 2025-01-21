#pragma once
#ifndef _DFH_COMPRESSION_TICK_COMPRESSOR_V1_HPP_INCLUDED
#define _DFH_COMPRESSION_TICK_COMPRESSOR_V1_HPP_INCLUDED

/// \file TickCompressorV1.hpp
/// \brief Provides functionality for compressing and decompressing market tick data.

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
    class TickCompressorV1 {
    public:

        /// \brief Constructs the TickCompressorV1 object.
        /// Initializes the compression context and reserves memory for intermediate buffers.
        TickCompressorV1()
            : m_context(), m_encoder(m_context), m_decoder(m_context) {
        }

        /// \brief Sets the configuration for encoding and decoding.
        /// \param config The configuration to set.
        void set_config(const TickEncodingConfig& config) {
            m_config = config;
        }

        /// \brief Gets the current configuration.
        /// \return The current encoding/decoding configuration.
        TickEncodingConfig get_config() const {
            return m_config;
        }

        /// \brief Checks if the signature of the input data matches the expected signature.
        /// \param input A vector containing the binary data.
        /// \return True if the signature matches, otherwise false.
        bool is_valid_signature(const std::vector<uint8_t>& input) const {
            if (input.empty()) return false; // No data to check.
            constexpr uint8_t signature = 0x01;
            return input[0] == signature;
        }

        /// \brief Compresses market tick data.
        /// \param ticks A vector of MarketTick structures representing the tick data.
        /// \param output A vector where the compressed data will be stored.
        /// \throw std::invalid_argument if the configuration is invalid (e.g., precision exceeds allowed digits).
        void compress(
				const std::vector<MarketTick>& ticks,
				std::vector<uint8_t>& output) {
            if (ticks.empty()) return;
            if (!m_config.enable_trade_based_encoding) {
                throw std::invalid_argument(
                    "Trade-based encoding is disabled in the configuration. "
                    "Ensure that `enable_trade_based_encoding` is set to true in the `TickEncodingConfig` before calling compress()."
                );
            }

            constexpr size_t max_digits = 18;
            if (m_config.price_digits > max_digits ||
                m_config.volume_digits > max_digits) {
                throw std::invalid_argument("Price or volume digits exceed maximum allowed digits.");
            }

            m_context.reset();

			auto& buffer = m_context.processing_buffer;

            uint8_t header = 0x00;
            // Record the precision levels
            // Bits 0-4: Number of decimal places for the price
            // Bit 5: Flag indicating the use of tick flags
            // Bit 6: Flag indicating the use of real volume (double)
            header |= (m_config.price_digits & 0x1F);
            header |= (m_config.enable_tick_flags << 5) & 0x20;
            header |= (m_config.enable_trade_based_encoding << 6) & 0x40;
            buffer.push_back(header);

            // Bits 0-4: Number of decimal places for the volume
            header = 0x00;
            header |= (m_config.volume_digits & 0x1F);
            header |= (ticks[0].has_flag(TickUpdateFlags::LAST_UPDATED) << 5) & 0x20;
            buffer.push_back(header);

			constexpr uint64_t interval_ms = 3600000ULL;
            const uint64_t base_unix_hour = (ticks[0].time_ms / interval_ms);
            dfh::utils::append_vbyte<uint32_t>(buffer, base_unix_hour);

            const double price_scale = utils::pow10<double>(m_config.price_digits);
            const uint64_t initial_price = std::llround(ticks[0].last * price_scale);
            dfh::utils::append_vbyte<uint64_t>(buffer, initial_price);
            dfh::utils::append_vbyte<uint32_t>(buffer, ticks.size());

            m_encoder.encode_price_last(
                buffer,
                ticks.data(),
                ticks.size(),
                price_scale,
                initial_price);

            m_encoder.encode_volume(
                buffer,
                ticks.data(),
                ticks.size(),
                utils::pow10<double>(m_config.volume_digits));

            m_encoder.encode_time(
                buffer,
                ticks.data(),
                ticks.size(),
                base_unix_hour * interval_ms);

            if (m_config.enable_tick_flags) {
                m_encoder.encode_side_flags(buffer, ticks.data(), ticks.size());
            }

            constexpr uint8_t signature = 0x01;
			compress_zstd_data(
				buffer.data(), buffer.size(),
				zstd_dict_tick_compressor_v1_102400,
				sizeof(zstd_dict_tick_compressor_v1_102400),
				signature,
				output);
        }

        /// \brief Compresses market tick data with a specified configuration.
        /// \param ticks A vector of MarketTick structures.
        /// \param config The configuration to use for compression.
        /// \param output A vector where the compressed data will be stored.
        void compress(
                const std::vector<MarketTick>& ticks,
                const TickEncodingConfig& config,
                std::vector<uint8_t>& output) {
            m_config = config;
            compress(ticks, output);
        }

        /// \brief Decompresses market tick data.
        /// \param input A vector of compressed data.
        /// \param ticks A vector where the decompressed MarketTick data will be stored.
        /// \throw std::runtime_error if decompression fails.
        void decompress(
				const std::vector<uint8_t>& input,
				std::vector<MarketTick>& ticks) {
            if (input.empty()) return;
            constexpr uint8_t signature = 0x01;
            if (input[0] != signature) {
                throw std::invalid_argument(
                    "Invalid data signature. The input data does not match the expected format. "
                    "Ensure that the data was compressed using the correct version of the compressor."
                );
            }

            m_context.reset();
            auto& buffer = m_context.processing_buffer;

			decompress_zstd_data(
				input.data() + 1, input.size() - 1,
				zstd_dict_tick_compressor_v1_102400,
				sizeof(zstd_dict_tick_compressor_v1_102400),
				buffer);

			size_t offset = 0;
            uint8_t header = buffer[offset++];
            m_config.price_digits = header & 0x1F;
            m_config.enable_tick_flags           = (header & 0x20) != 0;
            m_config.enable_trade_based_encoding = (header & 0x40) != 0;

            header = buffer[offset++];
            m_config.volume_digits  = header & 0x1F;
            const bool last_updated = (header & 0x20) != 0;

			constexpr uint64_t interval_ms = 3600000ULL;
            const uint64_t base_unix_hour = dfh::utils::extract_vbyte<uint32_t>(buffer.data(), offset);
            const uint64_t base_unix_time = base_unix_hour * interval_ms;
            const uint64_t initial_price = dfh::utils::extract_vbyte<uint64_t>(buffer.data(), offset);
            const size_t num_ticks = dfh::utils::extract_vbyte<uint32_t>(buffer.data(), offset);

            ticks.resize(num_ticks);

            m_decoder.decode_price_last(
                ticks.data(),
                buffer.data(),
                offset,
                num_ticks,
                utils::pow10<double>(m_config.price_digits),
                initial_price);

            m_decoder.decode_volume(
                ticks.data(),
                buffer.data(),
                offset,
                num_ticks,
                utils::pow10<double>(m_config.volume_digits));

            m_decoder.decode_time(
                ticks.data(),
                buffer.data(),
                offset,
                num_ticks,
                base_unix_time);

            if (m_config.enable_tick_flags) {
                m_decoder.decode_side_flags(
                    ticks.data(),
                    buffer.data(),
                    offset,
                    num_ticks);
                if (last_updated) {
                    ticks[0].set_flag(TickUpdateFlags::LAST_UPDATED);
                }
            }
        }

        /// \brief Decompresses market tick data and retrieves the configuration.
        /// \param input A vector of compressed data.
        /// \param ticks A vector where the decompressed MarketTick data will be stored.
        /// \param config A reference to store the retrieved configuration.
        void decompress(
                const std::vector<uint8_t>& input,
                std::vector<MarketTick>& ticks,
                TickEncodingConfig& config) {
            decompress(input, ticks);
            config = m_config;
        }

    private:
        TickCompressionContextV1  m_context; ///< Compression context containing intermediate buffers.
        TickEncoderV1             m_encoder; ///< Encoder for market tick data.
        TickDecoderV1             m_decoder; ///< Decoder for market tick data.
        TickEncodingConfig        m_config;  ///< Configuration for encoding/decoding.
    }; // TickCompressorV1

}; // namespace dfh::compression

#endif // _DFH_COMPRESSION_TICK_COMPRESSOR_V1_HPP_INCLUDED
