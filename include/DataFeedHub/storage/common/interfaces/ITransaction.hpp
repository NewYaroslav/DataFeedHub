#pragma once
#ifndef _DFH_STORAGE_ITRANSACTION_HPP_INCLUDED
#define _DFH_STORAGE_ITRANSACTION_HPP_INCLUDED

/// \file ITransaction.hpp
/// \brief Declares the interface for a generic transaction object used in storage systems.

namespace dfh::storage {

    /// \class ITransaction
    /// \brief Abstract interface for a generic transaction object.
    class ITransaction {
    public:
        /// \brief Virtual destructor for safe polymorphic deletion.
        virtual ~ITransaction() = default;

        /// \brief Begins a new transaction.
        /// \throws StorageException If the transaction cannot be started.
        virtual void begin() = 0;

        /// \brief Commits the transaction explicitly.
        /// \throws StorageException If the commit operation fails.
        virtual void commit() = 0;

        /// \brief Rolls back the transaction explicitly.
        /// \throws StorageException If the rollback operation fails.
        virtual void rollback() = 0;
    };

    /// \typedef TransactionPtr
    /// \brief Alias for a unique pointer to an ITransaction.
    ///
    /// Used for owning and managing transaction lifetimes in a safe RAII style.
    using TransactionPtr = std::unique_ptr<ITransaction>;

}; // namespace dfh::storage

#endif // _DFH_STORAGE_ITRANSACTION_HPP_INCLUDED
