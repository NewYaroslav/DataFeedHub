#pragma once
#ifndef _DFH_SQLITE_CONFIG_HPP_INCLUDED
#define _DFH_SQLITE_CONFIG_HPP_INCLUDED

/// \file SQLiteConfig.hpp
/// \brief

#include <string>

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
    std::string to_string(const JournalMode &mode) {
        static const std::array<std::string, 6> data = {
            "DELETE",
            "TRUNCATE",
            "PERSIST",
            "MEMORY",
            "WAL",
            "OFF"
        };
        return data[static_cast<size_t>(mode)];
    }

    /// \brief Converts SynchronousMode enum to string representation.
    /// \param mode The SynchronousMode enum value.
    /// \return String representation of the SynchronousMode.
    std::string to_string(const SynchronousMode &mode) {
        static const std::array<std::string, 6> data = {
            "OFF",
            "NORMAL",
            "FULL",
            "EXTRA"
        };
        return data[static_cast<size_t>(mode)];
    }

    /// \brief Converts LockingMode enum to string representation.
    /// \param mode The LockingMode enum value.
    /// \return String representation of the LockingMode.
    std::string to_string(const LockingMode &mode) {
        static const std::array<std::string, 6> data = {
            "NORMAL",
            "EXCLUSIVE"
        };
        return data[static_cast<size_t>(mode)];
    }

    /// \brief Converts AutoVacuumMode enum to string representation.
    /// \param mode The AutoVacuumMode enum value.
    /// \return String representation of the AutoVacuumMode.
    std::string to_string(const AutoVacuumMode &mode) {
        static const std::array<std::string, 6> data = {
            "NONE",
            "FULL",
            "INCREMENTAL"
        };
        return data[static_cast<size_t>(mode)];
    }

    /// \brief Converts TransactionMode enum to string representation.
    /// \param mode The TransactionMode enum value.
    /// \return String representation of the TransactionMode.
    std::string to_string(const TransactionMode &mode) {
        static const std::array<std::string, 3> data = {
            "DEFERRED",
            "IMMEDIATE",
            "EXCLUSIVE"
        };
        return data[static_cast<size_t>(mode)];
    }

    /// \class SQLiteConfig
    /// \brief Configuration for SQLite databases.
    class SQLiteConfig final : public IConfigDB {
    public:
        std::string db_path;                    ///< Path to the SQLite database file.
        bool read_only = false;                 ///< Whether the database is in read-only mode.
        bool use_uri   = false;                 ///< Whether to use URI for opening the database.
        bool in_memory = false;                 ///< Whether the database should be in-memory.
        bool use_async = false;                 ///< Whether to use asynchronous write.
        int user_version = -1;                  ///< User-defined version number for the database schema.
        int busy_timeout = 1000;                ///< Timeout in milliseconds for busy handler.
        int page_size  = 4096;                  ///< SQLite page size.
        int cache_size = 2000;                  ///< SQLite cache size (in pages).
        int analysis_limit = 1000;              ///< Maximum number of rows to analyze.
        int wal_autocheckpoint = 1000;          ///< WAL auto-checkpoint threshold.

        JournalMode journal_mode = JournalMode::DELETE_MODE; ///< SQLite journal mode.
        SynchronousMode synchronous = SynchronousMode::FULL; ///< SQLite synchronous mode.
        LockingMode locking_mode = LockingMode::NORMAL;      ///< SQLite locking mode.
        AutoVacuumMode auto_vacuum_mode = AutoVacuumMode::NONE; ///< SQLite auto-vacuum mode.
        TransactionMode default_txn_mode = TransactionMode::IMMEDIATE; ///< Default transaction mode.

        /// \brief Validate the SQLite configuration.
        /// \return True if the configuration is valid, false otherwise.
        bool validate() const override {
            return !db_path.empty(); // Example validation: db_path must be set.
        }

        /// \brief Set a configuration option by key.
        /// \param key The name of the configuration option.
        /// \param value The value to set for the option.
        void set_option(const std::string& key, const std::string& value) override {
            if (key == "db_path") db_path = value;
            else if (key == "read_only") read_only = (value == "true");
            // Add more key-value mappings as needed
        }

        /// \brief Get a configuration option by key.
        /// \param key The name of the configuration option.
        /// \return The value of the configuration option.
        std::string get_option(const std::string& key) const override {
            if (key == "db_path") return db_path;
            if (key == "read_only") return read_only ? "true" : "false";
            return "";
        }
    };

}; //namespace dfh::storage::sqlite3

#endif // _DFH_SQLITE_CONFIG_HPP_INCLUDED
