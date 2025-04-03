#pragma once
#ifndef _DFH_COMPRESSION_ITICK_SERIALIZER_HPP_INCLUDED
#define _DFH_COMPRESSION_ITICK_SERIALIZER_HPP_INCLUDED

/// \file ITickSerializer.hpp
/// \brief Defines an interface for tick serialization and deserialization.

namespace dfh::compression {

    /// \class ITickSerializer
    /// \brief Interface for market tick serialization and deserialization.
    class ITickSerializer {
    public:
        virtual ~ITickSerializer() = default;

		/// \brief Checks if the signature of the input data matches the expected signature.
		/// \param input A vector containing the binary data.
		/// \return True if the signature matches, otherwise false.
		virtual bool is_valid_signature(const std::vector<uint8_t>& input) const = 0;

		/// \brief Sets the configuration for encoding and decoding.
        /// \param config The configuration to set.
        virtual void set_codec_config(const dfh::TickCodecConfig& config) = 0;

        /// \brief Gets the current configuration.
        /// \return The current encoding/decoding configuration.
        virtual const dfh::TickCodecConfig& codec_config() const = 0;

        /// \brief Serializes tick data into a binary format.
        /// \param ticks A vector of MarketTick structures.
        /// \param output A vector where the binary data will be stored.
        virtual void serialize(
            const std::vector<dfh::MarketTick>& ticks,
            std::vector<uint8_t>& output) = 0;

        /// \brief Serializes tick data with a specified configuration.
        /// \param ticks A vector of MarketTick structures.
        /// \param config The serialization configuration.
        /// \param output A vector where the binary data will be stored.
        virtual void serialize(
            const std::vector<dfh::MarketTick>& ticks,
            const dfh::TickCodecConfig& config,
            std::vector<uint8_t>& output) = 0;

        /// \brief Deserializes tick data from binary format.
        /// \param input A vector of binary data.
        /// \param ticks A vector where the deserialized tick data will be stored.
        virtual void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<dfh::MarketTick>& ticks) = 0;

        /// \brief Deserializes tick data and retrieves the configuration.
        /// \param input A vector of binary data.
        /// \param ticks A vector where the deserialized tick data will be stored.
        /// \param config A reference to store the retrieved configuration.
        virtual void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<dfh::MarketTick>& ticks,
            dfh::TickCodecConfig& config) = 0;
    };

} // namespace dfh::compression

#endif // _DFH_COMPRESSION_ITICK_SERIALIZER_HPP_INCLUDED
