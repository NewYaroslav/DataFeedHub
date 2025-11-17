#pragma once
#ifndef _DFH_UTILS_SYMBOL_KEY_UTILS_HPP_INCLUDED
#define _DFH_UTILS_SYMBOL_KEY_UTILS_HPP_INCLUDED

/// \file symbol_key_utils.hpp
/// \brief Utility functions for packing and unpacking market symbol keys.

#include <cstdint>

#include <DataFeedHub/data/common/enums.hpp>

namespace dfh {

    /// \brief Bitmask for extracting the 35-bit timestamp part from a 64-bit symbol key.
    /// Used when decoding the lower part of the key (milliseconds since epoch).
    constexpr uint64_t KEY64_TIMESTAMP_MASK = 0x7FFFFFFFFULL;

    /// \brief Bitmask for extracting the 32-bit symbol part (market type, exchange ID, symbol ID) from a 64-bit key.
    /// Useful for filtering or isolating all non-timestamp data in a key.
    constexpr uint64_t KEY64_SYMBOL_PART_MASK = 0xFFFFFFFE00000000ULL;

    /// \brief Packs market_type, exchange_id, and symbol_id into a 32-bit key.
    /// \param market_type Market type (max 3 bits: 0–7).
    /// \param exchange_id Exchange identifier (max 10 bits: 0–1023).
    /// \param symbol_id Symbol identifier (max 16 bits: 0–65535).
    /// \return Packed 32-bit key.
    constexpr uint32_t make_symbol_key32(
            MarketType market_type,
            uint16_t exchange_id,
            uint16_t symbol_id) noexcept {
        return (static_cast<uint32_t>(static_cast<uint8_t>(market_type) & 0x07) << 26)
             | (static_cast<uint32_t>(exchange_id & 0x03FF) << 16)
             | static_cast<uint32_t>(symbol_id);
    }

    /// \brief Extracts market type from packed symbol key.
    /// \param key 32-bit packed key.
    /// \return Market type (3-bit value).
    inline MarketType extract_market_type(uint32_t key) noexcept {
        return static_cast<MarketType>((key >> 26) & 0x07);
    }

    /// \brief Extracts exchange ID from packed symbol key.
    /// \param key 32-bit packed key.
    /// \return Exchange identifier (10-bit value).
    inline uint16_t extract_exchange_id(uint32_t key) noexcept {
        return static_cast<uint16_t>((key >> 16) & 0x03FF);
    }

    /// \brief Extracts symbol ID from packed symbol key.
    /// \param key 32-bit packed key.
    /// \return Symbol identifier (16-bit value).
    inline uint16_t extract_symbol_id(uint32_t key) noexcept {
        return static_cast<uint16_t>(key & 0xFFFF);
    }

    /// \brief Extracts all three fields from a packed symbol key.
    /// \param key 32-bit packed key.
    /// \param market_type Output: market type.
    /// \param exchange_id Output: exchange ID.
    /// \param symbol_id Output: symbol ID.
    inline void extract_symbol_key32(
            uint32_t key,
            MarketType& market_type,
            uint16_t& exchange_id,
            uint16_t& symbol_id) noexcept {
        market_type = static_cast<MarketType>(key >> 26);         // Bits 31-26 (3 bits used)
        exchange_id = static_cast<uint16_t>((key >> 16) & 0x3FF); // Bits 25-16 (10 bits)
        symbol_id   = static_cast<uint16_t>(key & 0xFFFF);        // Bits 15-0 (16 bits)
    }

    /// \brief Packs market type, exchange ID, symbol ID, and timestamp into a 64-bit key.
    /// \param market_type Market type (3 bits max).
    /// \param exchange_id Exchange ID (up to 10 bits).
    /// \param symbol_id Symbol ID (16 bits).
    /// \param timestamp Timestamp value (35 bits, typically in milliseconds or seconds).
    /// \return Packed 64-bit key.
    constexpr uint64_t make_symbol_key64(
            MarketType market_type,
            uint16_t exchange_id,
            uint16_t symbol_id,
            uint64_t timestamp) noexcept {
        return (static_cast<uint64_t>(market_type)   << 61) |
               (static_cast<uint64_t>(exchange_id)   << 51) |
               (static_cast<uint64_t>(symbol_id)     << 35) |
               (timestamp & 0x7FFFFFFFF); // 35 bits mask
    }

    /// \brief Extracts all fields from a 64-bit packed symbol key.
    /// \param key 64-bit packed key.
    /// \param market_type Output: market type (3 bits).
    /// \param exchange_id Output: exchange ID (10 bits).
    /// \param symbol_id Output: symbol ID (16 bits).
    /// \param timestamp Output: timestamp (35 bits).
    inline void extract_symbol_key64(
            uint64_t key,
            MarketType& market_type,
            uint16_t& exchange_id,
            uint16_t& symbol_id,
            uint64_t& timestamp) noexcept {
        timestamp    = key & 0x7FFFFFFFFULL;                        // Lower 35 bits
        symbol_id    = static_cast<uint16_t>((key >> 35) & 0xFFFF); // Next 16 bits
        exchange_id  = static_cast<uint16_t>((key >> 51) & 0x3FF);  // 10 bits
        market_type  = static_cast<MarketType>((key >> 61) & 0x07);    // 3 bits
    }

    /// \brief Combines a 32-bit symbol key and timestamp into a 64-bit key.
    /// \param key32 32-bit key created via make_symbol_key().
    /// \param timestamp Timestamp value (35 bits, typically in milliseconds).
    /// \return Combined 64-bit key.
    constexpr uint64_t make_symbol_key64(uint32_t key32, uint64_t timestamp) noexcept {
        return (static_cast<uint64_t>(key32) << 35) | (timestamp & 0x7FFFFFFFFULL); // 35 bits for timestamp
    }

    /// \brief Extracts the 32-bit symbol key and timestamp from a 64-bit key.
    /// \param key64 64-bit packed key.
    /// \param key32 Output: 32-bit symbol key.
    /// \param timestamp Output: timestamp (35 bits).
    inline void extract_symbol_key64(
            uint64_t key64,
            uint32_t& key32,
            uint64_t& timestamp) noexcept {
        timestamp = key64 & 0x7FFFFFFFFULL;           // Lower 35 bits
        key32     = static_cast<uint32_t>(key64 >> 35); // Upper 29 bits
    }

} // namespace dfh

#endif // _DFH_UTILS_SYMBOL_KEY_UTILS_HPP_INCLUDED
