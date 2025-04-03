#pragma once
#ifndef _DFH_COMPRESSION_IBAR_SERIALIZER_HPP_INCLUDED
#define _DFH_COMPRESSION_IBAR_SERIALIZER_HPP_INCLUDED

/// \file IBarSerializer.hpp
/// \brief Defines an interface for market bar serialization and deserialization.

namespace dfh::compression {

    /// \class IBarSerializer
    /// \brief Interface for market bar serialization and deserialization.
    class IBarSerializer {
    public:
        virtual ~IBarSerializer() = default;

        /// \brief Checks if the signature of the input data matches the expected signature.
        /// \param input A vector containing the binary data.
        /// \return True if the signature matches, otherwise false.
        virtual bool is_valid_signature(const std::vector<uint8_t>& input) const = 0;

        /// \brief Sets the configuration for encoding and decoding.
        /// \param config The configuration to set.
        virtual void set_codec_config(const dfh::BarCodecConfig& config) = 0;

        /// \brief Gets the current configuration.
        /// \return The current encoding/decoding configuration.
        virtual const dfh::BarCodecConfig& codec_config() const = 0;

        /// \brief Serializes bar data into a binary format.
        /// \param bars A vector of MarketBar structures.
        /// \param output A vector where the binary data will be stored.
        virtual void serialize(
            const std::vector<dfh::MarketBar>& bars,
            std::vector<uint8_t>& output) = 0;

        /// \brief Serializes bar data with a specified configuration.
        /// \param bars A vector of MarketBar structures.
        /// \param config The serialization configuration.
        /// \param output A vector where the binary data will be stored.
        virtual void serialize(
            const std::vector<dfh::MarketBar>& bars,
            const dfh::BarCodecConfig& config,
            std::vector<uint8_t>& output) = 0;

        /// \brief Deserializes bar data from binary format.
        /// \param input A vector of binary data.
        /// \param bars A vector where the deserialized bar data will be stored.
        virtual void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<dfh::MarketBar>& bars) = 0;

        /// \brief Deserializes bar data and retrieves the configuration.
        /// \param input A vector of binary data.
        /// \param bars A vector where the deserialized bar data will be stored.
        /// \param config A reference to store the retrieved configuration.
        virtual void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<dfh::MarketBar>& bars,
            dfh::BarCodecConfig& config) = 0;
    };

} // namespace dfh::compression

#endif // _DFH_COMPRESSION_IBAR_SERIALIZER_HPP_INCLUDED
