#pragma once
#ifndef _DFH_COMPRESSION_BAR_BINARY_SERIALIZER_V1_HPP_INCLUDED
#define _DFH_COMPRESSION_BAR_BINARY_SERIALIZER_V1_HPP_INCLUDED

/// \file BarBinarySerializerV1.hpp
/// \brief Implements a simple binary serializer for bar data.

namespace dfh::compression {

    /// \class BarBinarySerializerV1
    /// \brief Converts bar data to a raw binary format without compression.
    class BarBinarySerializerV1 final : public IBarSerializer {
    public:

        BarBinarySerializerV1() = default;

        /// \copydoc IBarSerializer::set_codec_config
        void set_codec_config(const dfh::BarCodecConfig& config) override final {
            m_config = config;
        }

        /// \copydoc IBarSerializer::codec_config
        const dfh::BarCodecConfig& codec_config() const override final {
            return m_config;
        }

        /// \copydoc IBarSerializer::is_valid_signature
        bool is_valid_signature(const std::vector<uint8_t>& input) const override final {
            if (input.empty()) return false;
            constexpr uint8_t signature = 0x00;
            return input[0] == signature;
        }

        /// \copydoc IBarSerializer::serialize(const std::vector<MarketBar>&, std::vector<uint8_t>&)
        /// \throws std::invalid_argument If STORE_RAW_BINARY flag is not set or if any digit field exceeds allowed precision.
        void serialize(
            const std::vector<dfh::MarketBar>& bars,
            std::vector<uint8_t>& output) override final {

            if (bars.empty()) return;
            if (!m_config.has_flag(dfh::BarStorageFlags::STORE_RAW_BINARY)) {
                throw std::invalid_argument(
                    "Raw binary storage is disabled in the configuration. "
                    "Ensure that `STORE_RAW_BINARY` is set in `BarStorageFlags` before calling serialize()."
                );
            }

            constexpr uint16_t max_digits = 18;
            if (m_config.price_digits > max_digits ||
                m_config.volume_digits > max_digits ||
                m_config.quote_volume_digits > max_digits) {
                throw std::invalid_argument("One or more digit fields exceed maximum allowed digits.");
            }

            const uint64_t duration_ms = dfh::get_segment_duration_ms(m_config.time_frame);

            output.clear();
            output.reserve(bars.size() * sizeof(dfh::MarketBar) + 32);

            constexpr uint8_t signature = 0x00;
            output.push_back(signature);
            dfh::utils::append_vbyte<uint32_t>(output, static_cast<uint32_t>(bars.size()));

            uint8_t header = 0x00;
            header |= (m_config.price_digits & 0x1F);
            header |= (m_config.has_flag(dfh::BarStorageFlags::BID_BASED) << 5) & 0x20;
            header |= (m_config.has_flag(dfh::BarStorageFlags::ASK_BASED) << 6) & 0x40;
            header |= (m_config.has_flag(dfh::BarStorageFlags::LAST_BASED) << 7) & 0x80;
            output.push_back(header);

            header = 0x00;
            header |= (m_config.volume_digits & 0x1F);
            header |= (m_config.has_flag(dfh::BarStorageFlags::ENABLE_VOLUME) << 5) & 0x20;
            header |= (m_config.has_flag(dfh::BarStorageFlags::ENABLE_QUOTE_VOLUME) << 6) & 0x40;
            header |= (m_config.has_flag(dfh::BarStorageFlags::ENABLE_TICK_VOLUME) << 7) & 0x80;
            output.push_back(header);

            header = 0x00;
            header |= (m_config.quote_volume_digits & 0x1F);
            header |= (m_config.has_flag(dfh::BarStorageFlags::ENABLE_BUY_VOLUME) << 5) & 0x20;
            header |= (m_config.has_flag(dfh::BarStorageFlags::ENABLE_BUY_QUOTE_VOLUME) << 6) & 0x40;
            header |= (m_config.has_flag(dfh::BarStorageFlags::ENABLE_SPREAD) << 7) & 0x80;
            output.push_back(header);

            header = 0x00;
            header |= (m_config.has_flag(dfh::BarStorageFlags::SPREAD_LAST) << 4) & 0x10;
            header |= (m_config.has_flag(dfh::BarStorageFlags::SPREAD_AVG) << 5) & 0x20;
            header |= (m_config.has_flag(dfh::BarStorageFlags::SPREAD_MAX) << 6) & 0x40;
            header |= (m_config.has_flag(dfh::BarStorageFlags::FINALIZED_BARS) << 7) & 0x80;
            output.push_back(header);

            dfh::utils::append_vbyte<uint32_t>(output, static_cast<uint32_t>(m_config.time_frame));

            const uint64_t base_unix_interval = (bars[0].time_ms / duration_ms);
            const uint64_t base_unix_time = base_unix_interval * duration_ms;

            dfh::utils::append_vbyte<uint32_t>(output, base_unix_interval);
            dfh::utils::append_vbyte<uint64_t>(output, encode_zig_zag_int64((int64_t)m_config.expiration_time_ms - (int64_t)base_unix_time));
            dfh::utils::append_vbyte<uint64_t>(output, encode_zig_zag_int64((int64_t)m_config.next_expiration_time_ms - (int64_t)base_unix_time));

            for (const auto& bar : bars) {
                append_binary(output, bar);
            }
        }

        /// \copydoc IBarSerializer::serialize(const std::vector<MarketBar>&, const BarCodecConfig&, std::vector<uint8_t>&)
        /// \throws std::invalid_argument If STORE_RAW_BINARY flag is not set or if any digit field exceeds allowed precision.
        void serialize(
            const std::vector<dfh::MarketBar>& bars,
            const dfh::BarCodecConfig& config,
            std::vector<uint8_t>& output) override final {

            m_config = config;
            serialize(bars, output);
        }

        /// \copydoc IBarSerializer::deserialize(const std::vector<uint8_t>&, std::vector<MarketBar>&)
        /// \throws std::invalid_argument If the binary signature is invalid.
        /// \throws std::runtime_error If the binary buffer is too small for the expected number of bars.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<MarketBar>& bars) override final {

            if (input.empty()) return;
            size_t offset = 0;

            constexpr uint8_t signature = 0x00;
            if (input[offset++] != signature) {
                throw std::invalid_argument("Invalid data signature for MarketBar binary format.");
            }

            m_config.set_flag(BarStorageFlags::STORE_RAW_BINARY, true);

            const size_t num_bars = dfh::utils::extract_vbyte<uint32_t>(input.data(), offset);

            uint8_t header = input[offset++];
            m_config.flags = BarStorageFlags::NONE;
            m_config.price_digits = header & 0x1F;
            m_config.set_flag(BarStorageFlags::BID_BASED, (header & 0x20) != 0);
            m_config.set_flag(BarStorageFlags::ASK_BASED, (header & 0x40) != 0);
            m_config.set_flag(BarStorageFlags::LAST_BASED, (header & 0x80) != 0);

            header = input[offset++];
            m_config.volume_digits = header & 0x1F;
            m_config.set_flag(BarStorageFlags::ENABLE_VOLUME, (header & 0x20) != 0);
            m_config.set_flag(BarStorageFlags::ENABLE_QUOTE_VOLUME, (header & 0x40) != 0);
            m_config.set_flag(BarStorageFlags::ENABLE_TICK_VOLUME, (header & 0x80) != 0);

            header = input[offset++];
            m_config.quote_volume_digits = header & 0x1F;
            m_config.set_flag(BarStorageFlags::ENABLE_BUY_VOLUME, (header & 0x20) != 0);
            m_config.set_flag(BarStorageFlags::ENABLE_BUY_QUOTE_VOLUME, (header & 0x40) != 0);
            m_config.set_flag(BarStorageFlags::ENABLE_SPREAD, (header & 0x80) != 0);

            header = input[offset++];
            m_config.set_flag(BarStorageFlags::SPREAD_LAST, (header & 0x10) != 0);
            m_config.set_flag(BarStorageFlags::SPREAD_AVG, (header & 0x20) != 0);
            m_config.set_flag(BarStorageFlags::SPREAD_MAX, (header & 0x40) != 0);
            m_config.set_flag(BarStorageFlags::FINALIZED_BARS, (header & 0x80) != 0);

            m_config.time_frame = static_cast<dfh::TimeFrame>(dfh::utils::extract_vbyte<uint32_t>(input.data(), offset));

            const uint64_t duration_ms = dfh::get_segment_duration_ms(m_config.time_frame);
            const uint64_t base_unix_interval = dfh::utils::extract_vbyte<uint32_t>(input.data(), offset);
            const uint64_t base_unix_time     = base_unix_interval * duration_ms;
            m_config.expiration_time_ms       = base_unix_time + decode_zig_zag_int64(dfh::utils::extract_vbyte<uint64_t>(input.data(), offset));
            m_config.next_expiration_time_ms  = base_unix_time + decode_zig_zag_int64(dfh::utils::extract_vbyte<uint64_t>(input.data(), offset));

            const size_t expected_size = num_bars * sizeof(dfh::MarketBar);
            if ((offset + expected_size) > input.size()) {
                throw std::runtime_error("Input buffer is too small for expected MarketBar data.");
            }

            bars.resize(num_bars);
            std::memcpy(bars.data(), input.data() + offset, num_bars * sizeof(dfh::MarketBar));
        }

        /// \copydoc IBarSerializer::deserialize(const std::vector<uint8_t>&, std::vector<MarketBar>&, BarCodecConfig&)
        /// \throws std::invalid_argument If the binary signature is invalid.
        /// \throws std::runtime_error If the binary buffer is too small for the expected number of bars.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<dfh::MarketBar>& bars,
            BarCodecConfig& config) override final {

            deserialize(input, bars);
            config = m_config;
        }

    private:
        dfh::BarCodecConfig m_config; ///< Configuration used for encoding and decoding.

        /// \brief Appends a MarketBar structure to the binary output buffer.
        /// \param buffer Binary buffer to append to.
        /// \param bar Bar data to append in raw memory form.
        static void append_binary(std::vector<uint8_t>& buffer, const dfh::MarketBar& bar) {
            const uint8_t* data = reinterpret_cast<const uint8_t*>(&bar);
            buffer.insert(buffer.end(), data, data + sizeof(dfh::MarketBar));
        }
    };

} // namespace dfh::compression

#endif // _DFH_COMPRESSION_BAR_BINARY_SERIALIZER_V1_HPP_INCLUDED
