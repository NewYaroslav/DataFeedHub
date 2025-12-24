#pragma once
#ifndef _DFH_COMPRESSION_TICK_BINARY_SERIALIZER_V1_HPP_INCLUDED
#define _DFH_COMPRESSION_TICK_BINARY_SERIALIZER_V1_HPP_INCLUDED

/// \file TickBinarySerializerV1.hpp
/// \brief Implements a simple binary serializer for tick data.

#include <cstddef>
#include <cstring>

#include "DataFeedHub/data/ticks/QuoteTick.hpp"
#include "DataFeedHub/data/ticks/QuoteTickVol.hpp"
#include "DataFeedHub/data/ticks/QuoteTickL1.hpp"
#include "DataFeedHub/data/ticks/TradeTick.hpp"
#include "DataFeedHub/data/ticks/QuoteTickConversions.hpp"
#include "DataFeedHub/data/ticks/TickCodecConfig.hpp"
#include "TradeIdCodec.hpp"
#include "ITickSerializer.hpp"

namespace dfh::compression {

    /// \class TickBinarySerializerV1
    /// \brief Converts tick data to a raw binary format without compression.
    class TickBinarySerializerV1 final : public ITickSerializer {
    public:

        TickBinarySerializerV1() = default;

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
            constexpr uint8_t signature = 0x00;
            return input[0] == signature;
        }

        /// \copydoc ITickSerializer::serialize(const std::vector<MarketTick>&, std::vector<uint8_t>&)
        /// \throws std::invalid_argument If STORE_RAW_BINARY flag is not set or if digits exceed allowed precision.
        void serialize(
            const std::vector<MarketTick>& ticks,
            std::vector<uint8_t>& output) override final {
            serialize_market_ticks(ticks, output, nullptr);
        }

        /// \copydoc ITickSerializer::serialize(const std::vector<MarketTick>&, const TickCodecConfig&, std::vector<uint8_t>&)
        /// \throws std::invalid_argument If STORE_RAW_BINARY flag is not set or if digits exceed allowed precision.
        void serialize(
            const std::vector<MarketTick>& ticks,
            const TickCodecConfig& config,
            std::vector<uint8_t>& output) override final {

            m_config = config;
            serialize_market_ticks(ticks, output, nullptr);
        }

        /// \copydoc ITickSerializer::deserialize(const std::vector<uint8_t>&, std::vector<MarketTick>&)
        /// \throws std::invalid_argument If the binary signature is invalid.
        /// \throws std::runtime_error If the binary buffer is too small for the expected number of ticks.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<MarketTick>& ticks) override final {

            deserialize_market_ticks(input, ticks, nullptr, nullptr);
        }

        /// \copydoc ITickSerializer::deserialize(const std::vector<uint8_t>&, std::vector<MarketTick>&, TickCodecConfig&)
        /// \throws std::invalid_argument If the binary signature is invalid.
        /// \throws std::runtime_error If the binary buffer is too small for the expected number of ticks.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<MarketTick>& ticks,
            TickCodecConfig& config) override final {

            deserialize_market_ticks(input, ticks, &config, nullptr);
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

        /// \brief Serializes QuoteTickVol with a configuration.
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

        /// \brief Serializes QuoteTickL1 data into a binary format.
        void serialize(
            const std::vector<QuoteTickL1>& ticks,
            std::vector<uint8_t>& output) override final {
            serialize_quote(ticks, output, true, true);
        }

        /// \brief Serializes QuoteTickL1 with a configuration.
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

    private:
        TickCodecConfig m_config; ///< Configuration used for encoding and decoding.

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
                m_config = adjusted_config;
                serialize_market_ticks(market_ticks, output, has_trade_ids ? &trade_ids : nullptr);
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
                m_config = adjusted_config;
                serialize_market_ticks(market_ticks, output, has_trade_ids ? &trade_ids : nullptr);
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
            deserialize_market_ticks(input, market_ticks, nullptr, &trade_ids);
            append_quote_ticks(market_ticks, ticks, &trade_ids);
        }

        template<typename QuoteType>
        void deserialize_quote(
            const std::vector<uint8_t>& input,
            std::vector<QuoteType>& ticks,
            TickCodecConfig& config) {

            std::vector<MarketTick> market_ticks;
            std::vector<uint64_t> trade_ids;
            deserialize_market_ticks(input, market_ticks, &config, &trade_ids);
            append_quote_ticks(market_ticks, ticks, &trade_ids);
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

        void serialize_market_ticks(
            const std::vector<MarketTick>& ticks,
            std::vector<uint8_t>& output,
            const std::vector<uint64_t>* trade_ids) {

            if (ticks.empty()) {
                return;
            }
            if (!m_config.has_flag(TickStorageFlags::STORE_RAW_BINARY)) {
                throw std::invalid_argument(
                    "Raw binary storage is disabled in the configuration. "
                    "Ensure that `STORE_RAW_BINARY` is set in `TickStorageFlags` before calling compress()."
                );
            }

            constexpr uint16_t max_digits = 18;
            if (m_config.price_digits > max_digits ||
                m_config.volume_digits > max_digits) {
                throw std::invalid_argument("Price or volume digits exceed maximum allowed digits.");
            }

            if (trade_ids && trade_ids->size() != ticks.size()) {
                throw std::invalid_argument("Trade identifier count must match number of ticks.");
            }

            output.clear();
            output.reserve(ticks.size() * sizeof(MarketTick) + 24);

            constexpr uint8_t signature = 0x00;
            output.push_back(signature);
            dfh::utils::append_vbyte<uint32_t>(output, ticks.size());

            uint8_t header = 0x00;
            header |= (m_config.price_digits & 0x1F);
            header |= (m_config.has_flag(TickStorageFlags::ENABLE_TICK_FLAGS) << 5) & 0x20;
            header |= (m_config.has_flag(TickStorageFlags::TRADE_BASED) << 6) & 0x40;
            header |= (m_config.has_flag(TickStorageFlags::ENABLE_VOLUME) << 7) & 0x80;
            output.push_back(header);

            header = 0x00;
            header |= (m_config.volume_digits & 0x1F);
            header |= (m_config.has_flag(TickStorageFlags::L1_TWO_VOLUMES) << 6) & 0x40;
            header |= (m_config.has_flag(TickStorageFlags::ENABLE_TRADE_ID) << 7) & 0x80;
            output.push_back(header);

            constexpr uint64_t interval_ms = 3600000ULL;
            const uint64_t base_unix_hour = (ticks[0].time_ms / interval_ms);
            const uint64_t base_unix_time = base_unix_hour * interval_ms;

            dfh::utils::append_vbyte<uint32_t>(output, base_unix_hour);
            dfh::utils::append_vbyte<uint64_t>(output, encode_zig_zag_int64((int64_t)m_config.expiration_time_ms - (int64_t)base_unix_time));
            dfh::utils::append_vbyte<uint64_t>(output, encode_zig_zag_int64((int64_t)m_config.next_expiration_time_ms - (int64_t)base_unix_time));

            for (const auto& tick : ticks) {
                append_binary(output, tick);
            }

            if (m_config.has_flag(TickStorageFlags::ENABLE_TRADE_ID) && trade_ids && !trade_ids->empty()) {
                encode_trade_id_deltas(output, *trade_ids);
            }
        }

        void deserialize_market_ticks(
            const std::vector<uint8_t>& input,
            std::vector<MarketTick>& ticks,
            TickCodecConfig* config,
            std::vector<uint64_t>* trade_ids) {

            ticks.clear();
            if (input.empty()) {
                if (config) {
                    config->flags = TickStorageFlags::NONE;
                }
                return;
            }

            size_t offset = 0;
            constexpr uint8_t signature = 0x00;
            if (input[offset++] != signature) {
                throw std::invalid_argument(
                    "Invalid data signature. The input data does not match the expected format. "
                    "Ensure that the data was compressed using the correct version of the compressor."
                );
            }

            const size_t num_ticks = dfh::utils::extract_vbyte<uint32_t>(input.data(), offset);

            uint8_t header = input[offset++];
            m_config.flags = TickStorageFlags::NONE;
            m_config.price_digits = header & 0x1F;
            m_config.set_flag(TickStorageFlags::ENABLE_TICK_FLAGS, (header & 0x20) != 0);
            m_config.set_flag(TickStorageFlags::TRADE_BASED, (header & 0x40) != 0);
            m_config.set_flag(TickStorageFlags::ENABLE_VOLUME, (header & 0x80) != 0);

            header = input[offset++];
            m_config.volume_digits  = header & 0x1F;
            m_config.set_flag(TickStorageFlags::L1_TWO_VOLUMES, (header & 0x40) != 0);
            m_config.set_flag(TickStorageFlags::ENABLE_TRADE_ID, (header & 0x80) != 0);
            m_config.set_flag(TickStorageFlags::STORE_RAW_BINARY, true);

            constexpr uint64_t interval_ms = 3600000ULL;
            const uint64_t base_unix_hour    = dfh::utils::extract_vbyte<uint32_t>(input.data(), offset);
            const uint64_t base_unix_time    = base_unix_hour * interval_ms;
            m_config.expiration_time_ms      = base_unix_time + decode_zig_zag_int64(dfh::utils::extract_vbyte<uint64_t>(input.data(), offset));
            m_config.next_expiration_time_ms = base_unix_time + decode_zig_zag_int64(dfh::utils::extract_vbyte<uint64_t>(input.data(), offset));

            const size_t expected_size = num_ticks * sizeof(MarketTick);
            if ((offset + expected_size) > input.size()) {
                throw std::runtime_error("Input buffer is too small for expected tick data.");
            }

            ticks.resize(num_ticks);
            std::memcpy(ticks.data(), input.data() + offset, num_ticks * sizeof(MarketTick));
            offset += expected_size;

            if (m_config.has_flag(TickStorageFlags::ENABLE_TRADE_ID)) {
                decode_trade_id_deltas(input.data(), offset, num_ticks, trade_ids);
            }

            if (config) {
                *config = m_config;
            }
        }

        /// \brief Appends a MarketTick structure to the binary output buffer.
        /// \param buffer Binary buffer to append to.
        /// \param tick Tick data to append in raw memory form.
        static void append_binary(std::vector<uint8_t>& buffer, const MarketTick& tick) {
            const uint8_t* data = reinterpret_cast<const uint8_t*>(&tick);
            buffer.insert(buffer.end(), data, data + sizeof(MarketTick));
        }
    };

} // namespace dfh::compression

#endif // _DFH_COMPRESSION_TICK_BINARY_SERIALIZER_V1_HPP_INCLUDED
