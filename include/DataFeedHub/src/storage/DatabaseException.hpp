#pragma once
#ifndef _DFH_DATABASE_EXCEPTION_HPP_INCLUDED
#define _DFH_DATABASE_EXCEPTION_HPP_INCLUDED

/// \file DatabaseException.hpp
/// \brief Defines a general exception for database-related errors.

#include <stdexcept>
#include <string>

namespace dfh::storage {

    /// \class DatabaseException
    /// \brief Represents a general exception for database-related errors.
    ///
    /// This exception is intended to be the base class for all exceptions
    /// related to database operations. Specific database exceptions, such
    /// as `SQLiteException`, should inherit from this class.
    class DatabaseException : public std::runtime_error {
    public:

        /// \brief Constructs a new DatabaseException with the given error message.
        /// \param message The error message describing the exception.
        explicit DatabaseException(const std::string& message)
            : std::runtime_error(message) {}
    };

} // namespace dfh::storage

#endif // _DFH_DATABASE_EXCEPTION_HPP_INCLUDED
