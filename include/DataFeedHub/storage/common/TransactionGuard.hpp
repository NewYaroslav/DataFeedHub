#pragma once
#ifndef _DFH_STORAGE_TRANSACTION_GUARD_HPP_INCLUDED
#define _DFH_STORAGE_TRANSACTION_GUARD_HPP_INCLUDED

/// \file TransactionGuard.hpp
/// \brief Declares a RAII transaction manager for managing transactions across multiple storage backends.

namespace dfh::storage {

    /// \class TransactionGuard
    /// \brief RAII transaction manager for multiple market data storage backends.
    ///
    /// This class manages a set of transactions across multiple storage backends.
    /// When destroyed without an explicit commit or rollback, it automatically rolls back all transactions.
    class TransactionGuard {
        friend class IMarketDataStorage;
        friend class TransactionGuardAccess;
    public:

        /// \brief Destructor that rolls back all uncommitted transactions.
        ~TransactionGuard() {
            if (!m_started || m_completed) return;
            for (size_t i = 0; i < m_transaction_list.size(); ++i) {
                if (!m_transaction_list[i]) continue;
                try {
                    m_transaction_list[i]->rollback();
                } catch(...) {};
            }
        }

        /// \brief Begins all transactions in the list.
        ///
        /// This method must be called before commit(), rollback(), or transaction().
        /// It is idempotent and safe to call multiple times.
        /// \throws StorageException If any individual transaction fails to begin.
        void begin() {
            if (m_started) return;
            for (size_t i = 0; i < m_transaction_list.size(); ++i) {
                if (!m_transaction_list[i]) continue;
                m_transaction_list[i]->begin();
            }
            for (size_t i = 0; i < m_storage_list.size(); ++i) {
                m_storage_list[i]->before_transaction(m_transaction_list[i]);
            }
            m_started = true;
        }

        /// \brief Commits all active transactions.
        /// \throws StorageException If begin() was not called or commit fails in a backend.
        void commit() {
            if (!m_started) throw StorageException("TransactionGuard::commit() called before begin().");
            if (m_completed) return;
            for (size_t i = 0; i < m_storage_list.size(); ++i) {
                m_storage_list[i]->after_transaction(transaction(i));
            }
            for (size_t i = 0; i < m_transaction_list.size(); ++i) {
                if (!m_transaction_list[i]) continue;
                m_transaction_list[i]->commit();
            }
            m_completed = true;
        }

        /// \brief Rolls back all active transactions.
        /// \throws StorageException If begin() was not called or rollback fails in a backend.
        void rollback() {
            if (!m_started) throw StorageException("TransactionGuard::rollback() called before begin().");
            if (m_completed) return;
            for (size_t i = 0; i < m_transaction_list.size(); ++i) {
                if (!m_transaction_list[i]) continue;
                m_transaction_list[i]->rollback();
            }
            m_completed = true;
        }

    private:
        std::vector<MarketDataStoragePtr>& m_storage_list; ///< List of storage backends participating in the transaction.
        std::vector<TransactionPtr>        m_transaction_list; ///< Corresponding transactions for each backend.
        bool m_started = false;   ///< Indicates whether begin() was called.
        bool m_completed = false; ///< Indicates whether commit or rollback has been performed.

        /// \brief Constructs the transaction guard and begins transactions on all storages.
        /// \param mode Transaction mode (read or write).
        /// \param storage_list List of market data storage backends to include in the transaction.
        TransactionGuard(
                TransactionMode mode,
                std::vector<MarketDataStoragePtr>& storage_list)
            : m_storage_list(storage_list) {
            m_transaction_list.reserve(m_storage_list.size());
            for (auto& storage : m_storage_list) {
                m_transaction_list.push_back(storage->create_transaction(mode));
            }
        }

        /// \brief Provides mutable access to the transaction object associated with the specified backend index.
        /// \param index Index of the backend in the storage list.
        /// \return Reference to the unique pointer holding the transaction.
        /// \throws StorageException If begin() was not called prior to accessing the transaction.
        const TransactionPtr& transaction(size_t index) {
            if (!m_started) {
                throw StorageException("TransactionGuard::transaction() accessed before begin().");
            }
            return m_transaction_list[index];
        }
    };

    /// \typedef TransactionGuardPtr
    /// \brief Alias for a unique pointer to TransactionGuard.
    ///
    /// Used to manage transaction lifetimes in an RAII-compliant way.
    using TransactionGuardPtr = std::unique_ptr<TransactionGuard>;

    /// \class TransactionGuardAccess
    /// \brief Friend helper class for accessing private and protected members of TransactionGuard.
    /// \details This class provides static helper functions to create TransactionGuard instances
    /// and to retrieve internal transaction objects. It is intended solely for use by friend classes.
    class TransactionGuardAccess {
    protected:

        /// \brief Creates a new TransactionGuard instance for the specified storage backends.
        /// \param mode Transaction mode (read-only or writable).
        /// \param storage_list List of backend storages to manage transactions for.
        /// \return A unique pointer to the created TransactionGuard.
        static TransactionGuardPtr create_guard(TransactionMode mode, std::vector<MarketDataStoragePtr>& storage_list) {
            return TransactionGuardPtr(new TransactionGuard(mode, storage_list));
        }

        /// \brief Retrieves a reference to a specific transaction within a TransactionGuard.
        /// \param guard Pointer to the TransactionGuard instance.
        /// \param index Index of the storage backend.
        /// \return Reference to the transaction associated with the specified index.
        static const TransactionPtr& get_transaction(TransactionGuard* guard, size_t index) {
            return guard->transaction(index);
        }

    public:

        /// \brief Virtual destructor.
        virtual ~TransactionGuardAccess() = default;
    };

}; // namespace dfh::storage

#endif // _DFH_STORAGE_TRANSACTION_GUARD_HPP_INCLUDED
