#pragma once
#ifndef _DFH_SQLITE_ENUMS_HPP_INCLUDED
#define _DFH_SQLITE_ENUMS_HPP_INCLUDED

/// \file enums.hpp
/// \brief Defines SQLite-related enumerations and conversion functions.

#include <string>
#include <vector>

namespace dfh::storage::sqlite3 {

    /// \enum JournalMode
    /// \brief SQLite journal modes enumeration.
    enum class JournalMode {
        DELETE_MODE, ///< Delete journal mode.
        TRUNCATE,    ///< Truncate journal mode.
        PERSIST,     ///< Persist journal mode.
        MEMORY,      ///< Memory journal mode.
        WAL,         ///< Write-ahead logging (WAL) mode.
        OFF          ///< Off journal mode.
    };

    /// \enum SynchronousMode
    /// \brief SQLite synchronous modes enumeration.
    enum class SynchronousMode {
        OFF,        ///< Synchronous mode off.
        NORMAL,     ///< Normal synchronous mode.
        FULL,       ///< Full synchronous mode.
        EXTRA       ///< Extra synchronous mode.
    };

    /// \enum LockingMode
    /// \brief SQLite locking modes enumeration.
    enum class LockingMode {
        NORMAL,     ///< Normal locking mode.
        EXCLUSIVE   ///< Exclusive locking mode.
    };

    /// \enum AutoVacuumMode
    /// \brief SQLite auto-vacuum modes enumeration.
    enum class AutoVacuumMode {
        NONE,       ///< No auto-vacuuming.
        FULL,       ///< Full auto-vacuuming.
        INCREMENTAL ///< Incremental auto-vacuuming.
    };

    /// \enum TransactionMode
    /// \brief Defines SQLite transaction modes.
    enum class TransactionMode {
        DEFERRED,   ///< Waits to lock the database until a write operation is requested.
        IMMEDIATE,  ///< Locks the database for writing at the start, allowing only read operations by others.
        EXCLUSIVE   ///< Locks the database for both reading and writing, blocking other transactions.
    };

    /// \brief Converts JournalMode enum to string representation.
    /// \param mode The JournalMode enum value.
    /// \return String representation of the JournalMode.
    inline const std::string &to_str(JournalMode mode) {
        static const std::array<std::string, 6> str_data = {
            "DELETE",
            "TRUNCATE",
            "PERSIST",
            "MEMORY",
            "WAL",
            "OFF"
        };
        return str_data[static_cast<size_t>(mode)];
    }

    /// \brief Converts SynchronousMode enum to string representation.
    /// \param mode The SynchronousMode enum value.
    /// \return String representation of the SynchronousMode.
    inline const std::string &to_str(SynchronousMode mode) {
        static const std::array<std::string, 6> str_data = {
            "OFF",
            "NORMAL",
            "FULL",
            "EXTRA"
        };
        return str_data[static_cast<size_t>(mode)];
    }

    /// \brief Converts LockingMode enum to string representation.
    /// \param mode The LockingMode enum value.
    /// \return String representation of the LockingMode.
    inline const std::string &to_str(LockingMode mode) {
        static const std::array<std::string, 6> str_data = {
            "NORMAL",
            "EXCLUSIVE"
        };
        return str_data[static_cast<size_t>(mode)];
    }

    /// \brief Converts AutoVacuumMode enum to string representation.
    /// \param mode The AutoVacuumMode enum value.
    /// \return String representation of the AutoVacuumMode.
    inline const std::string &to_str(AutoVacuumMode mode) {
        static const std::array<std::string, 6> str_data = {
            "NONE",
            "FULL",
            "INCREMENTAL"
        };
        return str_data[static_cast<size_t>(mode)];
    }

    /// \brief Converts TransactionMode enum to string representation.
    /// \param mode The TransactionMode enum value.
    /// \return String representation of the TransactionMode.
    inline const std::string &to_str(TransactionMode mode) {
        static const std::array<std::string, 3> str_data = {
            "DEFERRED",
            "IMMEDIATE",
            "EXCLUSIVE"
        };
        return str_data[static_cast<size_t>(mode)];
    }

    /// \brief Stream output operator for JournalMode.
	std::ostream& operator<<(std::ostream& os, JournalMode value) {
        os << dfh::storage::sqlite3::to_str(value);
        return os;
    }

    /// \brief Stream output operator for LockingMode.
	std::ostream& operator<<(std::ostream& os, LockingMode value) {
        os << dfh::storage::sqlite3::to_str(value);
        return os;
    }

    /// \brief Stream output operator for AutoVacuumMode.
	std::ostream& operator<<(std::ostream& os, AutoVacuumMode value) {
        os << dfh::storage::sqlite3::to_str(value);
        return os;
    }

    /// \brief Stream output operator for TransactionMode.
	std::ostream& operator<<(std::ostream& os, TransactionMode value) {
        os << dfh::storage::sqlite3::to_str(value);
        return os;
    }

};

#endif // _DFH_SQLITE_ENUMS_HPP_INCLUDED
