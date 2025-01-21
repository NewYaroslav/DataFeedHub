#pragma once
#ifndef _DFH_SQLITE_EXCEPTION_HPP_INCLUDED
#define _DFH_SQLITE_EXCEPTION_HPP_INCLUDED

/// \file SQLiteException.hpp
/// \brief Defines a specific exception for SQLite-related errors.

namespace dfh::storage::sqlite3 {

    /// \class SQLiteException
    /// \brief Represents a specific exception for SQLite-related errors.
    ///
    /// This exception is used to handle errors that occur during SQLite operations.
    /// It extends `DatabaseException` and adds support for storing an SQLite-specific error code.
    class SQLiteException : public dfh::storage::DatabaseException {
    public:

        /// \brief Constructs a new SQLiteException with the given message and error code.
        ///
        /// This constructor allows setting a descriptive error message and an optional
        /// SQLite-specific error code.
        ///
        /// \param message The error message describing the exception.
        /// \param error_code The SQLite error code associated with this exception (default: -1).
        explicit SQLiteException(const std::string& message, const int& error_code = -1)
            : DatabaseException("SQLite error: " + message), m_error_code(error_code) {}

        /// \brief Returns the SQLite error code associated with this exception.
        ///
        /// The error code provides additional context about the nature of the SQLite error.
        ///
        /// \return The SQLite error code.
        int error_code() const noexcept {
            return m_error_code;
        }

    private:
        int m_error_code; ///< The SQLite error code associated with the exception.
    };

} // namespace dfh::storage::sqlite3

#endif // _DFH_SQLITE_EXCEPTION_HPP_INCLUDED
