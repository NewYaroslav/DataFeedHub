#pragma once
#ifndef _DFH_STORAGE_MDBX_EXCEPTION_HPP_INCLUDED
#define _DFH_STORAGE_MDBX_EXCEPTION_HPP_INCLUDED

/// \file MDBXException.hpp
/// \brief Defines a specific exception for MDBX-related errors.

namespace dfh::storage::mdbx {

    /// \class MDBXException
    /// \brief Represents a specific exception for MDBX-related errors.
    ///
    /// This exception is used to handle errors that occur during MDBX operations.
    /// It extends `StorageException` and adds support for storing an MDBX-specific error code.
    class MDBXException : public dfh::storage::StorageException {
    public:

        /// \brief Constructs a new MDBXException with the given message and error code.
        ///
        /// \param message The error message describing the exception.
        /// \param error_code The MDBX error code associated with this exception (default: -1).
        explicit MDBXException(const std::string& message, int error_code = -1)
            : StorageException("MDBX error: " + message), m_error_code(error_code) {}

        /// \brief Returns the MDBX error code associated with this exception.
        /// \return The MDBX error code.
        int error_code() const noexcept {
            return m_error_code;
        }

    private:
        int m_error_code; ///< The MDBX error code associated with the exception.
    };

} // namespace dfh::storage::mdbx

#endif // _DFH_STORAGE_MDBX_EXCEPTION_HPP_INCLUDED
