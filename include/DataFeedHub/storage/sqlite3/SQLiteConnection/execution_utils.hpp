#pragma once
#ifndef _DFH_SQLITE_EXECUTION_UTILS_HPP_INCLUDED
#define _DFH_SQLITE_EXECUTION_UTILS_HPP_INCLUDED

/// \file execution_utils.hpp
/// \brief Utility functions for executing SQLite statements.

#include <sqlite3.h>
#include <string>
#include <stdexcept>

#ifndef DFH_SQLITE_BUSY_RETRY_DELAY_MS
#define DFH_SQLITE_BUSY_RETRY_DELAY_MS 50
#endif

namespace dfh::storage::sqlite3 {

    /// \brief Executes a SQLite statement.
    /// \param stmt Pointer to the SQLite statement.
    /// \throws SQLiteException if statement is null, or if an error occurs during execution.
    inline void execute(sqlite3_stmt *stmt) {
        if (!stmt) throw SQLiteException("Invalid statement pointer.");
        int err;
        for (;;) {
            while ((err = sqlite3_step(stmt)) == SQLITE_ROW);
            switch (err) {
            case SQLITE_DONE:
                return;
            case SQLITE_BUSY:
                sqlite3_sleep(DFH_SQLITE_BUSY_RETRY_DELAY_MS);
                continue;
            case SQLITE_FULL:
                throw SQLiteException("Disk full or IO error.", err);
            case SQLITE_IOERR:
                throw SQLiteException("Failed to insert data into database.", err);
            default:
                throw SQLiteException("SQLite error.", err);
            }
        }
    }

    /// \brief Executes a SQLite statement.
    /// \param sqlite_db Pointer to the SQLite database.
    /// \param stmt Pointer to the SQLite statement.
    /// \throws SQLiteException if the database or statement is null, or if an error occurs during execution.
    inline void execute(sqlite3 *sqlite_db, sqlite3_stmt *stmt) {
        if (!sqlite_db || !stmt) throw SQLiteException("Invalid database or statement pointer.");
        int err;
        for (;;) {
            while ((err = sqlite3_step(stmt)) == SQLITE_ROW);
            switch (err) {
            case SQLITE_DONE:
                return;
            case SQLITE_BUSY:
                sqlite3_sleep(DFH_SQLITE_BUSY_RETRY_DELAY_MS);
                continue;
            case SQLITE_FULL:
                throw SQLiteException("Disk full or IO error: " + std::string(sqlite3_errmsg(sqlite_db)) + ". Error code: " + std::to_string(err), err);
            case SQLITE_IOERR:
                throw SQLiteException("Failed to insert data into database: " + std::string(sqlite3_errmsg(sqlite_db)) + ". Error code: " + std::to_string(err), err);
            default:
                throw SQLiteException("SQLite error: " + std::string(sqlite3_errmsg(sqlite_db)) + ". Error code: " + std::to_string(err), err);
            }
        }
    }

    /// \brief Prepares and executes a SQLite statement given as a string.
    /// \param sqlite_db Pointer to the SQLite database.
    /// \param query The SQL query to execute.
    /// \throws SQLiteException if the database pointer is null, the request is empty, or if an error occurs during execution.
    inline void execute(sqlite3 *sqlite_db, const char *query) {
        if (!sqlite_db) throw SQLiteException("Invalid database pointer.");
        if (!query || std::strlen(query) == 0) throw SQLiteException("Empty SQL request.");
        int err;
        do {
            err = sqlite3_exec(sqlite_db, query, nullptr, nullptr, nullptr);
            if (err == SQLITE_BUSY) {
                sqlite3_sleep(DFH_SQLITE_BUSY_RETRY_DELAY_MS);
            } else
            if (err != SQLITE_OK) {
                std::string err_msg = "SQLite error during prepare: ";
                err_msg += sqlite3_errmsg(sqlite_db);
                err_msg += ". Error code: ";
                err_msg += std::to_string(err);
                throw SQLiteException(err_msg, err);
            }
        } while (err == SQLITE_BUSY);
    }

    /// \brief Prepares and executes a SQLite statement given as a string.
    /// \param sqlite_db Pointer to the SQLite database.
    /// \param query The SQL query to execute.
    /// \throws SQLiteException if the database pointer is null, the request is empty, or if an error occurs during execution.
    inline void execute(sqlite3 *sqlite_db, const std::string &query) {
        execute(sqlite_db, query.c_str());
    }

}; // namespace dfh::storage::sqlite3

#endif // _DFH_SQLITE_EXECUTION_UTILS_HPP_INCLUDED
