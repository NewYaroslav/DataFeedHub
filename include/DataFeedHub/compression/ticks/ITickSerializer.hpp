#pragma once
#ifndef _DFH_COMPRESSION_ITICK_SERIALIZER_HPP_INCLUDED
#define _DFH_COMPRESSION_ITICK_SERIALIZER_HPP_INCLUDED

/// \file ITickSerializer.hpp
/// \brief Defines an interface for tick serialization and deserialization.

#include "DataFeedHub/data.hpp"
#include <vector>

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

        /// \brief Serializes quote tick data into a binary format.
        /// \param ticks A vector of QuoteTick structures.
        /// \param output A vector where the binary data will be stored.
        virtual void serialize(
            const std::vector<dfh::QuoteTick>& ticks,
            std::vector<uint8_t>& output) = 0;

        /// \brief Serializes quote tick data with a specified configuration.
        /// \param ticks A vector of QuoteTick structures.
        /// \param config The serialization configuration.
        /// \param output A vector where the binary data will be stored.
        virtual void serialize(
            const std::vector<dfh::QuoteTick>& ticks,
            const dfh::TickCodecConfig& config,
            std::vector<uint8_t>& output) = 0;

        /// \brief Deserializes quote tick data from binary format.
        /// \param input A vector of binary data.
        /// \param ticks A vector where the deserialized quote tick data will be stored.
        virtual void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<dfh::QuoteTick>& ticks) = 0;

        /// \brief Deserializes quote tick data and retrieves the configuration.
        /// \param input A vector of binary data.
        /// \param ticks A vector where the deserialized quote tick data will be stored.
        /// \param config A reference to store the retrieved configuration.
        virtual void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<dfh::QuoteTick>& ticks,
            dfh::TickCodecConfig& config) = 0;

        /// \brief Serializes quote tick data with a provider volume into a binary format.
        /// \param ticks A vector of QuoteTickVol structures.
        /// \param output A vector where the binary data will be stored.
        virtual void serialize(
            const std::vector<dfh::QuoteTickVol>& ticks,
            std::vector<uint8_t>& output) = 0;

        /// \brief Serializes quote tick data with provider volume and configuration.
        /// \param ticks A vector of QuoteTickVol structures.
        /// \param config The serialization configuration.
        /// \param output A vector where the binary data will be stored.
        virtual void serialize(
            const std::vector<dfh::QuoteTickVol>& ticks,
            const dfh::TickCodecConfig& config,
            std::vector<uint8_t>& output) = 0;

        /// \brief Deserializes quote tick data with provider volume.
        /// \param input A vector of binary data.
        /// \param ticks A vector to store the deserialized QuoteTickVol data.
        virtual void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<dfh::QuoteTickVol>& ticks) = 0;

        /// \brief Deserializes quote ticks with provider volume and retrieves the configuration.
        /// \param input A vector of binary data.
        /// \param ticks A vector to store the deserialized QuoteTickVol data.
        /// \param config A reference to store the retrieved configuration.
        virtual void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<dfh::QuoteTickVol>& ticks,
            dfh::TickCodecConfig& config) = 0;

        /// \brief Serializes trade tick data into a binary format.
        /// \param ticks A vector of TradeTick structures.
        /// \param output A vector where the binary data will be stored.
        virtual void serialize(
            const std::vector<dfh::TradeTick>& ticks,
            std::vector<uint8_t>& output) = 0;

        /// \brief Serializes trade tick data with a specified configuration.
        /// \param ticks A vector of TradeTick structures.
        /// \param config The serialization configuration.
        /// \param output A vector where the binary data will be stored.
        virtual void serialize(
            const std::vector<dfh::TradeTick>& ticks,
            const dfh::TickCodecConfig& config,
            std::vector<uint8_t>& output) = 0;

        /// \brief Deserializes trade tick data from binary format.
        /// \param input A vector of binary data.
        /// \param ticks A vector where the deserialized trade tick data will be stored.
        virtual void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<dfh::TradeTick>& ticks) = 0;

        /// \brief Deserializes trade tick data and retrieves the configuration.
        /// \param input A vector of binary data.
        /// \param ticks A vector where the deserialized trade tick data will be stored.
        /// \param config A reference to store the retrieved configuration.
        virtual void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<dfh::TradeTick>& ticks,
            dfh::TickCodecConfig& config) = 0;

        /// \brief Serializes L1 quote tick data into a binary format.
        /// \param ticks A vector of QuoteTickL1 structures.
        /// \param output A vector where the binary data will be stored.
        virtual void serialize(
            const std::vector<dfh::QuoteTickL1>& ticks,
            std::vector<uint8_t>& output) = 0;

        /// \brief Serializes L1 quote ticks with configuration.
        /// \param ticks A vector of QuoteTickL1 structures.
        /// \param config The serialization configuration.
        /// \param output A vector where the binary data will be stored.
        virtual void serialize(
            const std::vector<dfh::QuoteTickL1>& ticks,
            const dfh::TickCodecConfig& config,
            std::vector<uint8_t>& output) = 0;

        /// \brief Deserializes L1 quote tick data from binary format.
        /// \param input A vector of binary data.
        /// \param ticks A vector to store the deserialized QuoteTickL1 data.
        virtual void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<dfh::QuoteTickL1>& ticks) = 0;

        /// \brief Deserializes L1 quote tick data and retrieves the configuration.
        /// \param input A vector of binary data.
        /// \param ticks A vector to store the deserialized QuoteTickL1 data.
        /// \param config A reference to store the retrieved configuration.
        virtual void deserialize(
            const std::vector<uint8_t>& input,
            std::vector<dfh::QuoteTickL1>& ticks,
            dfh::TickCodecConfig& config) = 0;
    };

} // namespace dfh::compression

#endif // _DFH_COMPRESSION_ITICK_SERIALIZER_HPP_INCLUDED
