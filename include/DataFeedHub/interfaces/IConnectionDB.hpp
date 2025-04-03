#pragma once
#ifndef _DFH_ICONNECTION_DB_HPP_INCLUDED
#define _DFH_ICONNECTION_DB_HPP_INCLUDED

/// \file IConnectionDB.hpp
/// \brief Interface for database connection management.
///
/// This file defines the interface for managing database connections,
/// including configuration, connection lifecycle, and optional transaction handling.

#include <memory>
#include <string>

namespace dfh {

    /// \class IConnectionDB
    /// \brief Interface for database connection management.
    ///
    /// Provides methods for configuring, connecting, and disconnecting
    /// from a database. It also includes optional methods for handling
    /// transactions and executing raw queries.
    class IConnectionDB {
    public:

        /// \brief Configure the database connection.
        ///
        /// This method sets the configuration for the database connection
        /// using an object implementing the IConfigDB interface.
        ///
        /// \param config A unique pointer to the database configuration object.
        virtual void configure(std::unique_ptr<IConfigDB> config) = 0;

        /// \brief Establish the database connection.
        ///
        /// This method establishes a connection to the database using
        /// the previously provided configuration.
        virtual void connect() = 0;

        /// \brief Disconnect from the database.
        ///
        /// This method closes the current database connection.
        virtual void disconnect() = 0;

        /// \brief Check if the connection to the database is active.
        ///
        /// \return True if the connection is active, false otherwise.
        virtual bool is_connected() const = 0;

        /// \brief Begin a database transaction.
        ///
        /// Starts a new transaction. Subsequent queries will be part of
        /// this transaction until it is committed or rolled back.
        virtual void begin() = 0;

        /// \brief Commit the current transaction.
        ///
        /// Finalizes the current transaction, making all changes permanent.
        virtual void commit() = 0;

        /// \brief Roll back the current transaction.
        ///
        /// Reverts all changes made during the current transaction.
        virtual void rollback() = 0;

        /// \brief Execute a raw SQL query.
        ///
        /// Allows executing raw SQL queries directly on the database.
        ///
        /// \param query A string containing the SQL query to execute.
        virtual void execute(const std::string& query) = 0;

        /// \brief Virtual destructor for the interface.
        virtual ~IConnectionDB() = default;

    }; // class IConnectionDB

} // namespace dfh

#endif // _DFH_ICONNECTION_DB_HPP_INCLUDED
