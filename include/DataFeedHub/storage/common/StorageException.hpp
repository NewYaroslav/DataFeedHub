#pragma once
#ifndef _DFH_STORAGE_EXCEPTION_HPP_INCLUDED
#define _DFH_STORAGE_EXCEPTION_HPP_INCLUDED

/// \file StorageException.hpp
/// \brief Defines the base exception for storage-related errors.

namespace dfh::storage {

    /// \class StorageException
    /// \brief Base class for all storage-related exceptions.
    ///
    /// Used to indicate errors during storage operations, including
    /// connection issues, read/write failures, and configuration problems.
    /// Specific backends (e.g. MDBX, SQLite) should define their own
    /// exceptions derived from this class.
    class StorageException : public std::runtime_error {
    public:

        /// \brief Constructs the exception with a message.
        /// \param message Description of the error.
        explicit StorageException(const std::string& message)
            : std::runtime_error(message) {}
    };

} // namespace dfh::storage

#endif // _DFH_STORAGE_EXCEPTION_HPP_INCLUDED
