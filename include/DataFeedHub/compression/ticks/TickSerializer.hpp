#pragma once
#ifndef _DFH_COMPRESSION_TICK_SERIALIZER_HPP_INCLUDED
#define _DFH_COMPRESSION_TICK_SERIALIZER_HPP_INCLUDED

/// \file TickSerializer.hpp
/// \brief Selects and applies the appropriate tick serializer based on configuration.

#include "../../data/ticks/QuoteTick.hpp"
#include "../../data/ticks/QuoteTickVol.hpp"
#include "../../data/ticks/QuoteTickL1.hpp"
#include "../../data/ticks/QuoteTickConversions.hpp"
#include "../../data/ticks/QuoteTickVol.hpp"
#include "../../data/ticks/QuoteTickL1.hpp"
#include "TickBinarySerializerV1.hpp"
#include "TickCompressorV1.hpp"

namespace dfh::compression {

    /// \class TickSerializer
    /// \brief Automatically selects and applies the appropriate serializer.
    ///
    /// This class chooses the correct serializer (`TickCompressorV1` or `TickBinarySerializerV1`)
    /// based on the flags set in `TickCodecConfig`. It provides a unified interface for serialization.
    class TickSerializer final : public ITickSerializer {
    public:

        TickSerializer() = default;

        /// \brief Sets the configuration for encoding and decoding.
        /// \param config The configuration to set.
        /// \throws std::runtime_error If no suitable serializer is found.
        void set_codec_config(const dfh::TickCodecConfig& config) override final {
            select_serializer(config);
            m_serializer->set_codec_config(config);
        }

        /// \brief Gets the current configuration.
        /// \return The current encoding/decoding configuration.
        const dfh::TickCodecConfig& codec_config() const override final {
            static const dfh::TickCodecConfig empty_config{};
            return m_serializer ? m_serializer->codec_config() : empty_config;
        }

        /// \brief Checks if the signature of the input data matches the expected signature.
        /// \param input A vector containing the binary data.
        /// \return True if the signature matches, otherwise false.
        bool is_valid_signature(const std::vector<uint8_t>& input) const override final {
            return m_tick_raw_binary_v1.is_valid_signature(input)
                || m_tick_compressor_v1.is_valid_signature(input);
        }

        /// \brief Serializes tick data into a binary format.
        /// \param ticks A vector of MarketTick structures.
        /// \param output A vector where the binary data will be stored.
        /// \throws std::runtime_error If no serializer is selected.
        /// \throws std::invalid_argument If the tick data is invalid.
        void serialize(
                const std::vector<dfh::MarketTick>& ticks,
                std::vector<uint8_t>& output) override final {
            if (!m_serializer) throw std::runtime_error("No serializer selected.");
            m_serializer->serialize(ticks, output);
        }

        /// \brief Serializes tick data with a specified configuration.
        /// \param ticks A vector of MarketTick structures.
        /// \param config The serialization configuration.
        /// \param output A vector where the binary data will be stored.
        /// \throws std::runtime_error If no suitable serializer is found.
        /// \throws std::invalid_argument If the tick data is invalid.
        void serialize(
                const std::vector<dfh::MarketTick>& ticks,
                const dfh::TickCodecConfig& config,
                std::vector<uint8_t>& output) override final {
            select_serializer(config);
            m_serializer->serialize(ticks, config, output);
        }

        /// \brief Serializes quote tick data into a binary format.
        void serialize(
                const std::vector<dfh::QuoteTick>& ticks,
                std::vector<uint8_t>& output) override final {
            if (!m_serializer) throw std::runtime_error("No serializer selected.");
            m_serializer->serialize(ticks, output);
        }

        /// \brief Serializes quote tick data with a specified configuration.
        void serialize(
                const std::vector<dfh::QuoteTick>& ticks,
                const dfh::TickCodecConfig& config,
                std::vector<uint8_t>& output) override final {
            select_serializer(config);
            m_serializer->serialize(ticks, config, output);
        }

        /// \brief Serializes QuoteTickVol data into a binary format.
        void serialize(
                const std::vector<dfh::QuoteTickVol>& ticks,
                std::vector<uint8_t>& output) override final {
            if (!m_serializer) throw std::runtime_error("No serializer selected.");
            m_serializer->serialize(ticks, output);
        }

        /// \brief Serializes QuoteTickVol data with a specified configuration.
        void serialize(
                const std::vector<dfh::QuoteTickVol>& ticks,
                const dfh::TickCodecConfig& config,
                std::vector<uint8_t>& output) override final {
            select_serializer(config);
            m_serializer->serialize(ticks, config, output);
        }

        /// \brief Serializes QuoteTickL1 data into a binary format.
        void serialize(
                const std::vector<dfh::QuoteTickL1>& ticks,
                std::vector<uint8_t>& output) override final {
            if (!m_serializer) throw std::runtime_error("No serializer selected.");
            m_serializer->serialize(ticks, output);
        }

        /// \brief Serializes QuoteTickL1 data with a specified configuration.
        void serialize(
                const std::vector<dfh::QuoteTickL1>& ticks,
                const dfh::TickCodecConfig& config,
                std::vector<uint8_t>& output) override final {
            select_serializer(config);
            m_serializer->serialize(ticks, config, output);
        }

        /// \brief Deserializes tick data from binary format.
        /// \param input A vector of binary data.
        /// \param ticks A vector where the deserialized tick data will be stored.
        /// \throws std::runtime_error If no suitable serializer is found.
        /// \throws std::invalid_argument If the input data format is invalid.
        void deserialize(
                const std::vector<uint8_t>& input,
                std::vector<dfh::MarketTick>& ticks) override final {
            select_serializer(input);
            m_serializer->deserialize(input, ticks);
        }

        /// \brief Deserializes tick data and retrieves the configuration.
        /// \param input A vector of binary data.
        /// \param ticks A vector where the deserialized tick data will be stored.
        /// \param config A reference to store the retrieved configuration.
        /// \throws std::runtime_error If no suitable serializer is found.
        /// \throws std::invalid_argument If the input data format is invalid.
        void deserialize(
                const std::vector<uint8_t>& input,
                std::vector<dfh::MarketTick>& ticks,
                dfh::TickCodecConfig& config) override final {
            select_serializer(input);
            m_serializer->deserialize(input, ticks, config);
        }

        /// \brief Deserializes quote tick data from binary format.
        void deserialize(
                const std::vector<uint8_t>& input,
                std::vector<dfh::QuoteTick>& ticks) override final {
            select_serializer(input);
            m_serializer->deserialize(input, ticks);
        }

        /// \brief Deserializes quote tick data and retrieves the configuration.
        void deserialize(
                const std::vector<uint8_t>& input,
                std::vector<dfh::QuoteTick>& ticks,
                dfh::TickCodecConfig& config) override final {
            select_serializer(input);
            m_serializer->deserialize(input, ticks, config);
        }

        /// \brief Deserializes QuoteTickVol data from binary format.
        void deserialize(
                const std::vector<uint8_t>& input,
                std::vector<dfh::QuoteTickVol>& ticks) override final {
            select_serializer(input);
            m_serializer->deserialize(input, ticks);
        }

        /// \brief Deserializes QuoteTickVol data and retrieves the configuration.
        void deserialize(
                const std::vector<uint8_t>& input,
                std::vector<dfh::QuoteTickVol>& ticks,
                dfh::TickCodecConfig& config) override final {
            select_serializer(input);
            m_serializer->deserialize(input, ticks, config);
        }

        /// \brief Deserializes QuoteTickL1 data from binary format.
        void deserialize(
                const std::vector<uint8_t>& input,
                std::vector<dfh::QuoteTickL1>& ticks) override final {
            select_serializer(input);
            m_serializer->deserialize(input, ticks);
        }

        /// \brief Deserializes QuoteTickL1 data and retrieves the configuration.
        void deserialize(
                const std::vector<uint8_t>& input,
                std::vector<dfh::QuoteTickL1>& ticks,
                dfh::TickCodecConfig& config) override final {
            select_serializer(input);
            m_serializer->deserialize(input, ticks, config);
        }

    private:
        TickBinarySerializerV1 m_tick_raw_binary_v1;
        TickCompressorV1       m_tick_compressor_v1;
        ITickSerializer*       m_serializer = nullptr;

        /// \brief Selects the appropriate serializer based on the provided configuration.
        /// \param config The configuration used to determine the serializer.
        /// \throws std::runtime_error If no suitable serializer is found.
        void select_serializer(const dfh::TickCodecConfig& config) {
            if (config.has_flag(dfh::TickStorageFlags::STORE_RAW_BINARY)) {
                m_serializer = &m_tick_raw_binary_v1;
            } else if (config.has_flag(dfh::TickStorageFlags::TRADE_BASED)) {
                m_serializer = &m_tick_compressor_v1;
            } else {
                throw std::runtime_error("Invalid TickCodecConfig: No suitable serializer selected.");
            }
        }

        /// \brief Selects the appropriate serializer based on input data signature.
        /// \param input The binary data from which to determine the serializer.
        /// \throws std::runtime_error If no suitable serializer is found.
        void select_serializer(const std::vector<uint8_t>& input) {
            if (m_tick_raw_binary_v1.is_valid_signature(input)) {
                m_serializer = &m_tick_raw_binary_v1;
            } else if (m_tick_compressor_v1.is_valid_signature(input)) {
                m_serializer = &m_tick_compressor_v1;
            } else {
                throw std::runtime_error("Invalid data: Unknown tick serialization format.");
            }
        }
    };

} // namespace dfh::compression

#endif // _DFH_COMPRESSION_TICK_SERIALIZER_HPP_INCLUDED
