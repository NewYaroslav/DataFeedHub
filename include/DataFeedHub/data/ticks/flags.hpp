#pragma once
#ifndef _DFH_DATA_TICKS_FLAGS_HPP_INCLUDED
#define _DFH_DATA_TICKS_FLAGS_HPP_INCLUDED

/// \file flags.hpp
/// \brief Defines flags for tick data status, updates, and storage.

#include <cstdint>

namespace dfh {

    /// \enum TickStatusFlags
    /// \brief Flags indicating the status of tick data.
    enum class TickStatusFlags : std::uint64_t {
        NONE        = 0,       ///< No flags set.
        REALTIME    = 1 << 0,  ///< Data received in real-time.
        INITIALIZED = 1 << 1   ///< Data has been initialized.
    };

    /// \enum TickUpdateFlags
    /// \brief Flags describing updates in tick data.
    enum class TickUpdateFlags : std::uint64_t {
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
    enum class TickStorageFlags : std::uint64_t {
        NONE               = 0,       ///< No special flags.
        TRADE_BASED        = 1 << 0,  ///< Encode as trade-based data (e.g., only last price).
        ENABLE_TICK_FLAGS  = 1 << 1,  ///< Encode TickUpdateFlags.
        ENABLE_RECV_TIME   = 1 << 2,  ///< Include received_time in encoded data.
        ENABLE_VOLUME      = 1 << 3,  ///< Store base asset volume.
        ENABLE_TRADE_ID    = 1 << 4,  ///< Store trade identifier for TradeTick.
        STORE_RAW_BINARY   = 1 << 5,  ///< Use raw binary format (no compression).
        L1_TWO_VOLUMES     = 1 << 6   ///< Mark presence of bid and ask volumes for L1 ticks.
    };
	
//----------------------------------------------------------------------------
// TickStorageFlags
//----------------------------------------------------------------------------

    /// \enum TickStorageFlags
    /// - Low bits (0..15): stream schema = which time-series/fields are present.
    /// - Mid/high bits: tick kind + modifiers.
    /// \brief Flags controlling tick data encoding, compression, and storage.
    enum class TickStorageFlags : std::uint64_t {
        NONE = 0,

        // --- Schema: time-series / fields present (0..15) ---
        HAS_LAST        = 1ull << 0,   ///< Stream contains "last" price series.
        HAS_BID         = 1ull << 1,   ///< Stream contains "bid" price series.
        HAS_ASK         = 1ull << 2,   ///< Stream contains "ask" price series.
        HAS_VALUE       = 1ull << 3,   ///< Stream contains a single "value" series (ValueTick-like).
        HAS_VOLUME      = 1ull << 4,   ///< Stream contains single volume series (trade volume or provider volume).
        HAS_BID_VOLUME  = 1ull << 5,   ///< Stream contains bid volume (L1).
        HAS_ASK_VOLUME  = 1ull << 6,   ///< Stream contains ask volume (L1).
        HAS_RECV_TIME   = 1ull << 7,   ///< Stream contains received timestamp per tick.
        HAS_TICK_FLAGS  = 1ull << 8,   ///< Stream contains TickUpdateFlags per tick (MarketTick-like).
        HAS_TRADE_ID    = 1ull << 9,   ///< Stream contains trade id (TradeTick-like).
        HAS_TRADE_SIDE  = 1ull << 10,  ///< Stream contains trade side (if not packed into trade id).

        // --- Tick kind (24..31): pick ONE ---
        TICK_KIND_VALUE      = 1ull << 24, ///< ValueTick: time + value.
        TICK_KIND_TRADE      = 1ull << 25, ///< TradeTick: time + (id/side) + price + volume.
        TICK_KIND_QUOTE      = 1ull << 26, ///< QuoteTick: time + bid/ask.
        TICK_KIND_QUOTE_VOL  = 1ull << 27, ///< QuoteTickVol: time + bid/ask + volume.
        TICK_KIND_QUOTE_L1   = 1ull << 28, ///< QuoteTickL1: time + bid/ask + bid_volume/ask_volume.
        TICK_KIND_MARKET     = 1ull << 29, ///< MarketTick: time + recv + bid/ask/last + volume + flags.

        // --- Modifiers (32..) ---
        STORE_RAW_BINARY     = 1ull << 32, ///< Store as raw binary (no compression).
        RESERVED_33          = 1ull << 33,
        RESERVED_34          = 1ull << 34
    };

//------------------------------------------------------------------------------
// TickUpdateFlags operators
//------------------------------------------------------------------------------

    /// \brief Enables bitwise AND for TickUpdateFlags.
    [[nodiscard]] constexpr TickUpdateFlags operator&(TickUpdateFlags a, TickUpdateFlags b) noexcept {
        return static_cast<TickUpdateFlags>(static_cast<std::uint64_t>(a) & static_cast<std::uint64_t>(b));
    }

    /// \brief Enables bitwise OR for TickUpdateFlags.
    [[nodiscard]] constexpr TickUpdateFlags operator|(TickUpdateFlags a, TickUpdateFlags b) noexcept {
        return static_cast<TickUpdateFlags>(static_cast<std::uint64_t>(a) | static_cast<std::uint64_t>(b));
    }

    /// \brief Enables bitwise NOT for TickUpdateFlags.
    [[nodiscard]] constexpr TickUpdateFlags operator~(TickUpdateFlags a) noexcept {
        return static_cast<TickUpdateFlags>(~static_cast<std::uint64_t>(a));
    }

    /// \brief Enables |= operator for TickUpdateFlags.
    constexpr TickUpdateFlags& operator|=(TickUpdateFlags& a, TickUpdateFlags b) noexcept {
        a = a | b;
        return a;
    }

    /// \brief Enables &= operator for TickUpdateFlags.
    constexpr TickUpdateFlags& operator&=(TickUpdateFlags& a, TickUpdateFlags b) noexcept {
        a = a & b;
        return a;
    }

    /// \brief Enables bitwise right shift for TickUpdateFlags
    [[nodiscard]] constexpr TickUpdateFlags operator>>(TickUpdateFlags a, std::size_t shift) noexcept {
        return static_cast<TickUpdateFlags>(static_cast<std::uint64_t>(a) >> shift);
    }

    /// \brief Enables bitwise left shift for TickUpdateFlags
    [[nodiscard]] constexpr TickUpdateFlags operator<<(TickUpdateFlags a, std::size_t shift) noexcept {
        return static_cast<TickUpdateFlags>(static_cast<std::uint64_t>(a) << shift);
    }

    /// \brief Enables bitwise AND assignment for TickUpdateFlags.
    constexpr TickUpdateFlags& operator&=(TickUpdateFlags& a, std::uint64_t b) noexcept {
        a = static_cast<TickUpdateFlags>(static_cast<std::uint64_t>(a) & b);
        return a;
    }

    /// \brief Enables bitwise OR assignment for TickUpdateFlags.
    constexpr TickUpdateFlags& operator|=(TickUpdateFlags& a, std::uint64_t b) noexcept {
        a = static_cast<TickUpdateFlags>(static_cast<std::uint64_t>(a) | b);
        return a;
    }

    /// \brief Enables bitwise AND for TickUpdateFlags.
    [[nodiscard]] constexpr TickUpdateFlags operator&(TickUpdateFlags a, std::uint64_t b) noexcept {
        return static_cast<TickUpdateFlags>(static_cast<std::uint64_t>(a) & b);
    }

    /// \brief Enables bitwise OR for TickUpdateFlags.
    [[nodiscard]] constexpr TickUpdateFlags operator|(TickUpdateFlags a, std::uint64_t b) noexcept {
        return static_cast<TickUpdateFlags>(static_cast<std::uint64_t>(a) | b);
    }

    /// \brief Checks if a specific flag is set in a bitmask.
    /// \param flags Bitmask of flags.
    /// \param flag Flag to check.
    /// \return True if the flag is set.
    [[nodiscard]] constexpr bool has_flag(std::uint64_t flags, TickUpdateFlags flag) noexcept {
        return (flags & static_cast<std::uint64_t>(flag)) != 0U;
    }

    /// \brief Sets a specific flag in a bitmask in-place.
    /// \param flags Bitmask to modify.
    /// \param flag Flag to set.
    constexpr void set_flag_in_place(std::uint64_t& flags, TickUpdateFlags flag) noexcept {
        flags |= static_cast<std::uint64_t>(flag);
    }

    /// \brief Returns a bitmask with a flag set.
    /// \param flags Original bitmask.
    /// \param flag Flag to set.
    /// \return New bitmask with the flag set.
    [[nodiscard]] constexpr std::uint64_t set_flag(std::uint64_t flags, TickUpdateFlags flag) noexcept {
        return flags | static_cast<std::uint64_t>(flag);
    }

    /// \brief Clears a specific flag in a bitmask.
    /// \param flags Bitmask to modify.
    /// \param flag Flag to clear.
    /// \return New bitmask with the flag cleared.
    [[nodiscard]] constexpr std::uint64_t clear_flag(std::uint64_t flags, TickUpdateFlags flag) noexcept {
        return flags & ~static_cast<std::uint64_t>(flag);
    }

//------------------------------------------------------------------------------
// TickStorageFlags operators
//------------------------------------------------------------------------------

    /// \brief Enables bitwise OR for TickStorageFlags.
    [[nodiscard]] constexpr TickStorageFlags operator|(TickStorageFlags a, TickStorageFlags b) noexcept {
        return static_cast<TickStorageFlags>(static_cast<std::uint64_t>(a) | static_cast<std::uint64_t>(b));
    }

    /// \brief Enables bitwise AND for TickStorageFlags.
    [[nodiscard]] constexpr TickStorageFlags operator&(TickStorageFlags a, TickStorageFlags b) noexcept {
        return static_cast<TickStorageFlags>(static_cast<std::uint64_t>(a) & static_cast<std::uint64_t>(b));
    }

    /// \brief Enables bitwise NOT for TickStorageFlags.
    [[nodiscard]] constexpr TickStorageFlags operator~(TickStorageFlags a) noexcept {
        return static_cast<TickStorageFlags>(~static_cast<std::uint64_t>(a));
    }

    /// \brief Enables |= operator for TickStorageFlags.
    constexpr TickStorageFlags& operator|=(TickStorageFlags& a, TickStorageFlags b) noexcept {
        a = a | b;
        return a;
    }

    /// \brief Enables &= operator for TickStorageFlags.
    constexpr TickStorageFlags& operator&=(TickStorageFlags& a, TickStorageFlags b) noexcept {
        a = a & b;
        return a;
    }

    /// \brief Checks if a TickStorageFlags mask contains a specific flag.
    [[nodiscard]] constexpr bool has_flag(TickStorageFlags flags, TickStorageFlags flag) noexcept {
        return (flags & flag) != TickStorageFlags::NONE;
    }

} // namespace dfh

#endif // _DFH_DATA_TICKS_FLAGS_HPP_INCLUDED
