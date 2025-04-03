#pragma once
#ifndef _DFH_STORAGE_ICONNECTION_HPP_INCLUDED
#define _DFH_STORAGE_ICONNECTION_HPP_INCLUDED

/// \file IConnection.hpp
/// \brief Declares the interface for managing storage backend connections.

namespace dfh::storage {

    /// \class IConnection
    /// \brief Interface for managing a storage backend connection.
    ///
    /// This interface provides methods to configure, establish, and terminate
    /// a connection to a storage backend (e.g., MDBX, SQLite, or others).
    /// Configuration is provided via an IConfig object.
    class IConnection {
    public:
        /// \brief Virtual destructor for safe polymorphic usage.
        virtual ~IConnection() = default;

        /// \brief Sets the backend configuration.
        /// \param config Unique pointer to the backend configuration object.
        virtual void configure(ConfigPtr config) = 0;

        /// \brief Establishes a connection to the backend using the configured parameters.
        /// \throws StorageException If the connection cannot be established.
        virtual void connect() = 0;

        /// \brief Terminates the current connection.
        /// \throws StorageException If the disconnection fails.
        virtual void disconnect() = 0;

        /// \brief Checks whether the connection is currently active.
        /// \return True if connected, false otherwise.
        virtual bool is_connected() const = 0;

    }; // class IConnection

    /// \typedef ConnectionPtr
    /// \brief Alias for a shared pointer to an IConnection.
    ///
    /// Used for managing shared access to a connection object across components.
    using ConnectionPtr = std::shared_ptr<IConnection>;

} // namespace dfh::storage

#endif // _DFH_STORAGE_ICONNECTION_HPP_INCLUDED
