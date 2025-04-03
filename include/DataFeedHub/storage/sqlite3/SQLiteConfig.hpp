#pragma once
#ifndef _DFH_SQLITE_CONFIG_HPP_INCLUDED
#define _DFH_SQLITE_CONFIG_HPP_INCLUDED

/// \file SQLiteConfig.hpp
/// \brief

namespace dfh::storage::sqlite3 {

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
