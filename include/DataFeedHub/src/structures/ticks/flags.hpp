#pragma once
#ifndef _DTH_TICK_FLAGS_HPP_INCLUDED
#define _DTH_TICK_FLAGS_HPP_INCLUDED

/// \file flags.hpp
/// \brief Defines flags for tick data status and update events

#include <cstdint>

namespace dfh {

    /// \brief Flags indicating the status of tick data
    enum class TickStatusFlags : uint64_t {
        NONE = 0,               ///< No flags set
        REALTIME = 1 << 0,      ///< Data received in real-time
        INITIALIZED = 1 << 1    ///< Data has been initialized
    };

    /// \brief Flags describing updates in tick data
    enum class TickUpdateFlags : uint64_t {
        NONE = 0,                   ///< No updates
        BID_UPDATED    = 1 << 0,    ///< Bid price updated
        ASK_UPDATED    = 1 << 1,    ///< Ask price updated
		LAST_UPDATED   = 1 << 2,    ///< Last price updated
        VOLUME_UPDATED = 1 << 3,    ///< Volume updated
        TICK_FROM_BUY  = 1 << 4,    ///< Tick resulted from a buy trade
        TICK_FROM_SELL = 1 << 5,    ///< Tick resulted from a sell trade
        BEST_MATH      = 1 << 6     ///< Tick matched the best price in the order book at the time of execution
    };

    /// \brief Combines two UpdateFlags using bitwise OR
    /// \param a First flag
    /// \param b Second flag
    /// \return Combined flags
    inline TickUpdateFlags operator|(TickUpdateFlags a, TickUpdateFlags b) {
        return static_cast<TickUpdateFlags>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
    }

    /// \brief Checks if specific flags are set in a bitmask
    /// \param flags The bitmask of flags
    /// \param flag The flag to check
    /// \return True if the flag is set, false otherwise
    inline bool has_flag(uint64_t flags, TickUpdateFlags flag) {
        return (flags & static_cast<uint64_t>(flag)) != 0;
    }

	/// \brief Sets a specific flag in a bitmask.
	/// \param flags The bitmask of flags.
	/// \param flag The flag to set.
	inline void set_flag_in_place(uint64_t& flags, TickUpdateFlags flag) {
		flags = flags | static_cast<uint64_t>(flag);
	}

	/// \brief Sets a specific flag in a bitmask.
	/// \param flags The bitmask of flags.
	/// \param flag The flag to set.
	/// \return The updated bitmask with the specified flag set.
	inline uint64_t set_flag(uint64_t flags, TickUpdateFlags flag) {
		return flags | static_cast<uint64_t>(flag);
	}

	/// \brief Clears a specific flag in a bitmask.
	/// \param flags The bitmask of flags.
	/// \param flag The flag to clear.
	/// \return The updated bitmask with the specified flag cleared.
	inline uint64_t clear_flag(uint64_t flags, TickUpdateFlags flag) {
		return flags & ~static_cast<uint64_t>(flag);
	}

} // namespace dfh

#endif // _DTH_TICK_FLAGS_HPP_INCLUDED
