#pragma once
#ifndef _DFH_COMPRESSION_BAR_SERIALIZER_HPP_INCLUDED
#define _DFH_COMPRESSION_BAR_SERIALIZER_HPP_INCLUDED

/// \file BarSerializer.hpp
/// \brief Selects and applies the appropriate bar serializer based on configuration.

namespace dfh::compression {

    /// \class BarSerializer
    /// \brief Automatically selects and applies the appropriate serializer.
    ///
    /// This class chooses the correct serializer (`BarBinarySerializerV1` or another)
    /// based on the flags set in `BarCodecConfig`. It provides a unified interface for serialization.
    class BarSerializer final : public IBarSerializer {
    public:

        BarSerializer() = default;

        /// \brief Sets the configuration for encoding and decoding.
        /// \param config Configuration used to initialize the serializer.
        /// \throws std::runtime_error If no suitable serializer is found.
        void set_codec_config(const dfh::BarCodecConfig& config) override final {
            select_serializer(config);
            m_serializer->set_codec_config(config);
        }

        /// \brief Gets the current configuration.
        /// \return Const reference to the current encoding/decoding configuration.
        const dfh::BarCodecConfig& codec_config() const override final {
            static const dfh::BarCodecConfig empty_config{};
            return m_serializer ? m_serializer->codec_config() : empty_config;
        }

        /// \brief Checks if the signature of the input data matches a known format.
        /// \param input Binary input buffer.
        /// \return True if the format is recognized, otherwise false.
        bool is_valid_signature(const std::vector<uint8_t>& input) const override final {
            return m_bar_binary_v1.is_valid_signature(input);
            // || m_bar_compressor_v1.is_valid_signature(input);
        }

        /// \brief Serializes bar data into binary format.
        /// \param bars Vector of MarketBar structures.
        /// \param output Output buffer for the resulting binary data.
        /// \throws std::runtime_error If no serializer is selected.
        void serialize(
            const std::vector<dfh::MarketBar>& bars,
            std::vector<uint8_t>& output) override final {
            if (!m_serializer) throw std::runtime_error("No serializer selected.");
            m_serializer->serialize(bars, output);
        }

        /// \brief Serializes bar data with a specific configuration.
        /// \param bars Vector of MarketBar structures.
        /// \param config Configuration used for serialization.
        /// \param output Output buffer for the resulting binary data.
        /// \throws std::runtime_error If no suitable serializer is found.
        void serialize(
            const std::vector<dfh::MarketBar>& bars,
            const dfh::BarCodecConfig& config,
            std::vector<uint8_t>& output) override final {
            select_serializer(config);
            m_serializer->serialize(bars, config, output);
        }

        /// \brief Deserializes bar data from binary format.
        /// \param input Binary input buffer.
        /// \param bars Output vector for deserialized MarketBar data.
        /// \throws std::runtime_error If the format is unrecognized or no serializer is found.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<dfh::MarketBar>& bars) override final {
            select_serializer(input);
            m_serializer->deserialize(input, bars);
        }

        /// \brief Deserializes bar data and restores configuration.
        /// \param input Binary input buffer.
        /// \param bars Output vector for deserialized MarketBar data.
        /// \param config Output configuration extracted from the data header.
        /// \throws std::runtime_error If the format is unrecognized or no serializer is found.
        void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<dfh::MarketBar>& bars,
            dfh::BarCodecConfig& config) override final {
            select_serializer(input);
            m_serializer->deserialize(input, bars, config);
        }

    private:
        BarBinarySerializerV1 m_bar_binary_v1;
        // BarCompressorV1       m_bar_compressor_v1;
        IBarSerializer*        m_serializer = nullptr;

        /// \brief Selects serializer based on codec config.
        /// \param config Configuration used for selecting the serializer.
        /// \throws std::runtime_error If no suitable serializer is found.
        void select_serializer(const dfh::BarCodecConfig& config) {
            if (config.has_flag(dfh::BarStorageFlags::STORE_RAW_BINARY)) {
                m_serializer = &m_bar_binary_v1;
            } else {
                throw std::runtime_error("Invalid BarCodecConfig: No suitable serializer selected.");
            }
        }

        /// \brief Selects serializer based on binary data signature.
        /// \param input Binary input buffer.
        /// \throws std::runtime_error If no matching serializer signature is found.
        void select_serializer(const std::vector<uint8_t>& input) {
            if (m_bar_binary_v1.is_valid_signature(input)) {
                m_serializer = &m_bar_binary_v1;
            } else {
                throw std::runtime_error("Invalid data: Unknown bar serialization format.");
            }
        }
    };

} // namespace dfh::compression

#endif // _DFH_COMPRESSION_BAR_SERIALIZER_HPP_INCLUDED
