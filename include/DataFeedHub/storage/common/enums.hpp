#pragma once
#ifndef _DTH_STORAGE_ENUMS_HPP_INCLUDED
#define _DTH_STORAGE_ENUMS_HPP_INCLUDED

/// \file enums.hpp
/// \brief Defines core enums related to storage backend types and transaction modes.

namespace dfh::storage {

    /// \enum StorageBackend
    /// \brief Represents the type of underlying storage backend.
    ///
    /// Used to identify which storage engine is used to store market data.
    enum class StorageBackend {
        MDBX,   ///< Uses the MDBX key-value store backend.
        SQLITE  ///< Uses the SQLite relational database backend.
    };

    /// \enum TransactionMode
    /// \brief Specifies the access mode of a transaction.
    ///
    /// Determines whether a transaction is intended for read-only access or full read-write operations.
    enum class TransactionMode {
        READ_ONLY,  ///< Read-only transaction (no write operations allowed)
        WRITABLE    ///< Writable transaction (allows inserts, updates, deletes)
    };

}; // namespace dfh::storage

#endif // _DTH_STORAGE_ENUMS_HPP_INCLUDED
