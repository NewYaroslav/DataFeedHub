#pragma once
#ifndef _DFH_DATA_TICKS_FLAGS_HPP_INCLUDED
#define _DFH_DATA_TICKS_FLAGS_HPP_INCLUDED

/// \file flags.hpp
/// \brief Defines flags for tick data status, updates, and storage.

namespace dfh {

    /// \enum TickStatusFlags
    /// \brief Flags indicating the status of tick data.
    enum class TickStatusFlags : uint64_t {
        NONE        = 0,       ///< No flags set.
        REALTIME    = 1 << 0,  ///< Data received in real-time.
        INITIALIZED = 1 << 1   ///< Data has been initialized.
    };

    /// \enum TickUpdateFlags
    /// \brief Flags describing updates in tick data.
    enum class TickUpdateFlags : uint64_t {
        NONE            = 0,        ///< No updates.
        BID_UPDATED     = 1 << 0,   ///< Bid price updated.
        ASK_UPDATED     = 1 << 1,   ///< Ask price updated.
        LAST_UPDATED    = 1 << 2,   ///< Last price updated.
        VOLUME_UPDATED  = 1 << 3,   ///< Volume updated.
        TICK_FROM_BUY   = 1 << 4,   ///< Tick resulted from a buy trade.
        TICK_FROM_SELL  = 1 << 5,   ///< Tick resulted from a sell trade.
        BEST_MATH       = 1 << 6    ///< Tick matched the best price in the order book.
    };

    /// \enum TickStorageFlags
    /// \brief Flags controlling tick data encoding, compression, and storage.
    enum class TickStorageFlags : uint8_t {
        NONE               = 0,       ///< No special flags.
        TRADE_BASED        = 1 << 0,  ///< Encode as trade-based data (e.g., only last price).
        ENABLE_TICK_FLAGS  = 1 << 1,  ///< Encode TickUpdateFlags.
        ENABLE_RECV_TIME   = 1 << 2,  ///< Include received_time in encoded data.
        ENABLE_VOLUME      = 1 << 3,  ///< Store base asset volume.
        STORE_RAW_BINARY   = 1 << 5   ///< Use raw binary format (no compression).
    };

//------------------------------------------------------------------------------
// TickUpdateFlags operators
//------------------------------------------------------------------------------

    /// \brief Enables bitwise AND for TickUpdateFlags.
    inline TickUpdateFlags operator&(TickUpdateFlags a, TickUpdateFlags b) {
        return static_cast<TickUpdateFlags>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
    }

    /// \brief Enables bitwise OR for TickUpdateFlags.
    inline TickUpdateFlags operator|(TickUpdateFlags a, TickUpdateFlags b) {
        return static_cast<TickUpdateFlags>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
    }

    /// \brief Enables bitwise NOT for TickUpdateFlags.
    inline TickUpdateFlags operator~(TickUpdateFlags a) {
        return static_cast<TickUpdateFlags>(~static_cast<uint64_t>(a));
    }

    /// \brief Enables |= operator for TickUpdateFlags.
    inline TickUpdateFlags& operator|=(TickUpdateFlags& a, TickUpdateFlags b) {
        a = a | b;
        return a;
    }

    /// \brief Enables &= operator for TickUpdateFlags.
    inline TickUpdateFlags& operator&=(TickUpdateFlags& a, TickUpdateFlags b) {
        a = a & b;
        return a;
    }

    /// \brief Enables comparison with zero for TickUpdateFlags.
    inline bool operator!=(TickUpdateFlags a, int b) {
        return static_cast<uint64_t>(a) != static_cast<uint64_t>(b);
    }

    /// \brief Enables bitwise right shift for TickUpdateFlags
    inline TickUpdateFlags operator>>(TickUpdateFlags a, size_t shift) {
        return static_cast<TickUpdateFlags>(static_cast<uint64_t>(a) >> shift);
    }

    /// \brief Enables bitwise left shift for TickUpdateFlags
    inline TickUpdateFlags operator<<(TickUpdateFlags a, size_t shift) {
        return static_cast<TickUpdateFlags>(static_cast<uint64_t>(a) << shift);
    }

    /// \brief Enables bitwise AND assignment for TickUpdateFlags.
    inline TickUpdateFlags& operator&=(TickUpdateFlags& a, uint64_t b) {
        a = static_cast<TickUpdateFlags>(static_cast<uint64_t>(a) & b);
        return a;
    }

    /// \brief Enables bitwise OR assignment for TickUpdateFlags.
    inline TickUpdateFlags& operator|=(TickUpdateFlags& a, uint64_t b) {
        a = static_cast<TickUpdateFlags>(static_cast<uint64_t>(a) | b);
        return a;
    }

    /// \brief Enables bitwise AND for TickUpdateFlags.
    inline TickUpdateFlags operator&(TickUpdateFlags a, uint64_t b) {
        return static_cast<TickUpdateFlags>(static_cast<uint64_t>(a) & b);
    }

    /// \brief Enables bitwise OR for TickUpdateFlags.
    inline TickUpdateFlags operator|(TickUpdateFlags a, uint64_t b) {
        return static_cast<TickUpdateFlags>(static_cast<uint64_t>(a) | b);
    }

    /// \brief Checks if a specific flag is set in a bitmask.
    /// \param flags Bitmask of flags.
    /// \param flag Flag to check.
    /// \return True if the flag is set.
    inline bool has_flag(uint64_t flags, TickUpdateFlags flag) {
        return (flags & static_cast<uint64_t>(flag)) != 0;
    }

    /// \brief Sets a specific flag in a bitmask in-place.
    /// \param flags Bitmask to modify.
    /// \param flag Flag to set.
    inline void set_flag_in_place(uint64_t& flags, TickUpdateFlags flag) {
        flags |= static_cast<uint64_t>(flag);
    }

    /// \brief Returns a bitmask with a flag set.
    /// \param flags Original bitmask.
    /// \param flag Flag to set.
    /// \return New bitmask with the flag set.
    inline uint64_t set_flag(uint64_t flags, TickUpdateFlags flag) {
        return flags | static_cast<uint64_t>(flag);
    }

    /// \brief Clears a specific flag in a bitmask.
    /// \param flags Bitmask to modify.
    /// \param flag Flag to clear.
    /// \return New bitmask with the flag cleared.
    inline uint64_t clear_flag(uint64_t flags, TickUpdateFlags flag) {
        return flags & ~static_cast<uint64_t>(flag);
    }

//------------------------------------------------------------------------------
// TickStorageFlags operators
//------------------------------------------------------------------------------

    /// \brief Enables bitwise OR for TickStorageFlags.
    inline TickStorageFlags operator|(TickStorageFlags a, TickStorageFlags b) {
        return static_cast<TickStorageFlags>(static_cast<uint8_t>(a) | static_cast<uint8_t>(b));
    }

    /// \brief Enables bitwise AND for TickStorageFlags.
    inline TickStorageFlags operator&(TickStorageFlags a, TickStorageFlags b) {
        return static_cast<TickStorageFlags>(static_cast<uint8_t>(a) & static_cast<uint8_t>(b));
    }

    /// \brief Enables bitwise NOT for TickStorageFlags.
    inline TickStorageFlags operator~(TickStorageFlags a) {
        return static_cast<TickStorageFlags>(~static_cast<uint8_t>(a));
    }

    /// \brief Enables |= operator for TickStorageFlags.
    inline TickStorageFlags& operator|=(TickStorageFlags& a, TickStorageFlags b) {
        a = a | b;
        return a;
    }

    /// \brief Enables &= operator for TickStorageFlags.
    inline TickStorageFlags& operator&=(TickStorageFlags& a, TickStorageFlags b) {
        a = a & b;
        return a;
    }

    /// \brief Enables comparison with zero for TickStorageFlags.
    inline bool operator!=(TickStorageFlags a, int b) {
        return static_cast<uint8_t>(a) != static_cast<uint8_t>(b);
    }

} // namespace dfh

#endif // _DFH_DATA_TICKS_FLAGS_HPP_INCLUDED
