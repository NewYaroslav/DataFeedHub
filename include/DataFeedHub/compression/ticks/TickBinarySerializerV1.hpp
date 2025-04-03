#pragma once
#ifndef _DFH_COMPRESSION_TICK_BINARY_SERIALIZER_V1_HPP_INCLUDED
#define _DFH_COMPRESSION_TICK_BINARY_SERIALIZER_V1_HPP_INCLUDED

/// \file TickBinarySerializerV1.hpp
/// \brief Implements a simple binary serializer for tick data.

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

            if (ticks.empty()) return;
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

            output.clear();
            output.reserve(ticks.size() * sizeof(MarketTick) + 24);

			constexpr uint8_t signature = 0x00;
			output.push_back(signature);
			dfh::utils::append_vbyte<uint32_t>(output, ticks.size());

			uint8_t header = 0x00;
			// Record the precision levels
            // Bits 0-4: Number of decimal places for the price
            // Bit 5: ENABLE_TICK_FLAGS — whether TickUpdateFlags are encoded
            // Bit 6: Flag indicating the use of real volume (double)
            header |= (m_config.price_digits & 0x1F);
            header |= (m_config.has_flag(TickStorageFlags::ENABLE_TICK_FLAGS) << 5) & 0x20;
            header |= (m_config.has_flag(TickStorageFlags::TRADE_BASED) << 6) & 0x40;
            header |= (m_config.has_flag(TickStorageFlags::ENABLE_VOLUME) << 7) & 0x80;
            output.push_back(header);

            // Bits 0-4: Number of decimal places for the volume
            header = 0x00;
            header |= (m_config.volume_digits & 0x1F);
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
        }

        /// \copydoc ITickSerializer::serialize(const std::vector<MarketTick>&, const TickCodecConfig&, std::vector<uint8_t>&)
        /// \throws std::invalid_argument If STORE_RAW_BINARY flag is not set or if digits exceed allowed precision.
        void serialize(
            const std::vector<MarketTick>& ticks,
            const TickCodecConfig& config,
            std::vector<uint8_t>& output) override final {

            m_config = config;
            serialize(ticks, output);
        }

        /// \copydoc ITickSerializer::deserialize(const std::vector<uint8_t>&, std::vector<MarketTick>&)
        /// \throws std::invalid_argument If the binary signature is invalid.
        /// \throws std::runtime_error If the binary buffer is too small for the expected number of ticks.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<MarketTick>& ticks) override final {

            if (input.empty()) return;
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
        }

        /// \copydoc ITickSerializer::deserialize(const std::vector<uint8_t>&, std::vector<MarketTick>&, TickCodecConfig&)
        /// \throws std::invalid_argument If the binary signature is invalid.
        /// \throws std::runtime_error If the binary buffer is too small for the expected number of ticks.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<MarketTick>& ticks,
            TickCodecConfig& config) override final {

            deserialize(input, ticks);
            config = m_config;
        }

    private:
        TickCodecConfig m_config; ///< Configuration used for encoding and decoding.

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
