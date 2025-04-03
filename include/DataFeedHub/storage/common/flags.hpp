#pragma once
#ifndef _DTH_STORAGE_FLAGS_HPP_INCLUDED
#define _DTH_STORAGE_FLAGS_HPP_INCLUDED

/// \file flags.hpp
/// \brief Flags describing which types of data are stored in a storage backend.

namespace dfh::storage {

    /// \enum StorageDataFlags
    /// \brief Bitmask flags representing the types of data stored in a storage backend.
    enum class StorageDataFlags : uint32_t {
        NONE      = 0,
        TICKS     = 1 << 0,  ///< Tick data is stored.
        BARS      = 1 << 1,  ///< Bar (OHLC) data is stored.
        METRICS   = 1 << 2,  ///< Aggregated metrics are stored.
        EVENTS    = 1 << 3,  ///< Market or custom events.
        FUNDING   = 1 << 4,  ///< Funding rate data.
        ORDERBOOK = 1 << 5   ///< Order book snapshots or updates.
    };

    /// \brief Enables bitwise OR for StorageDataFlags.
    inline StorageDataFlags operator|(StorageDataFlags a, StorageDataFlags b) {
        return static_cast<StorageDataFlags>(
            static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
    }

    /// \brief Enables bitwise AND for StorageDataFlags.
    inline StorageDataFlags operator&(StorageDataFlags a, StorageDataFlags b) {
        return static_cast<StorageDataFlags>(
            static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
    }

    /// \brief Enables bitwise OR assignment for StorageDataFlags.
    inline StorageDataFlags& operator|=(StorageDataFlags& a, StorageDataFlags b) {
        a = a | b;
        return a;
    }

    /// \brief Enables bitwise AND assignment for StorageDataFlags.
    inline StorageDataFlags& operator&=(StorageDataFlags& a, StorageDataFlags b) {
        a = a & b;
        return a;
    }

    /// \brief Enables bitwise XOR for StorageDataFlags.
    inline StorageDataFlags operator^(StorageDataFlags a, StorageDataFlags b) {
        return static_cast<StorageDataFlags>(
            static_cast<uint32_t>(a) ^ static_cast<uint32_t>(b));
    }

    /// \brief Enables bitwise XOR assignment for StorageDataFlags.
    inline StorageDataFlags& operator^=(StorageDataFlags& a, StorageDataFlags b) {
        a = a ^ b;
        return a;
    }

    /// \brief Enables bitwise NOT for StorageDataFlags.
    inline StorageDataFlags operator~(StorageDataFlags a) {
        return static_cast<StorageDataFlags>(
            ~static_cast<uint32_t>(a));
    }

};

#endif // _DTH_STORAGE_FLAGS_HPP_INCLUDED
