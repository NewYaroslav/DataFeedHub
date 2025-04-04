#pragma once
#ifndef _DFH_STORAGE_MDBX_TRANSACTION_HPP_INCLUDED
#define _DFH_STORAGE_MDBX_TRANSACTION_HPP_INCLUDED

/// \file MDBXTransaction.hpp
/// \brief Declares a transaction wrapper for the MDBX database engine.

namespace dfh::storage::mdbx {

    /// \class MDBXTransaction
    /// \brief Provides an implementation of ITransaction using the MDBX database backend.
    ///
    /// This class handles read-only and writable transactions, including beginning,
    /// committing, and rolling back operations. It manages transaction lifecycles
    /// and integrates with MDBX-specific features.
    class MDBXTransaction final : public dfh::storage::ITransaction {
    public:

        /// \brief Constructs a new transaction object.
        /// \param connection Shared pointer to the active MDBX connection.
        /// \param mode Transaction mode (read-only or writable).
        MDBXTransaction(std::shared_ptr<MDBXConnection> connection, TransactionMode mode)
            : m_connection(connection), m_mode(mode) {
        }

        /// \brief Destructor that safely closes or resets the transaction.
        ///
        /// Ensures that the transaction is properly closed or reset depending on mode.
        /// Read-only transactions are reset for reuse; writable transactions are aborted.
        virtual ~MDBXTransaction() {
			if (!m_txn) return;
			switch (m_mode) {
            case TransactionMode::READ_ONLY:
                mdbx_txn_reset(m_txn);
                m_txn = nullptr;
                break;
            case TransactionMode::WRITABLE:
                mdbx_txn_abort(m_txn);
                m_txn = nullptr;
                break;
            };
        }

        /// \copydoc ITransaction::begin
        /// \throws MDBXException if the transaction cannot be started.
        void begin() override final {
            if (m_txn) throw MDBXException("Transaction already started.");
            switch (m_mode) {
            case TransactionMode::READ_ONLY:
                m_txn = m_connection->rdonly_handle();
                m_rc = mdbx_txn_renew(m_txn);
                if (m_rc != MDBX_SUCCESS) throw MDBXException(
                        "Failed to renew transaction: (" + std::to_string(m_rc) + ") " + std::string(mdbx_strerror(m_rc)), m_rc);
                break;
            case TransactionMode::WRITABLE:
                m_rc = mdbx_txn_begin(m_connection->env_handle(), nullptr, MDBX_TXN_READWRITE, &m_txn);
                if (m_rc != MDBX_SUCCESS) throw MDBXException(
                        "Failed to begin transaction: (" + std::to_string(m_rc) + ") " + std::string(mdbx_strerror(m_rc)), m_rc);
                break;
            };
        }

        /// \copydoc ITransaction::commit
        /// \throws MDBXException if commit fails.
        void commit() override final {
            if (!m_txn) throw MDBXException("No active transaction to commit.");
            switch (m_mode) {
            case TransactionMode::READ_ONLY:
                m_rc = mdbx_txn_reset(m_txn);
                if (m_rc != MDBX_SUCCESS) throw MDBXException(
                    "Failed to reset read-only transaction: (" + std::to_string(m_rc) + ") " + std::string(mdbx_strerror(m_rc)), m_rc);
                m_txn = nullptr;
                break;
            case TransactionMode::WRITABLE:
                m_rc = mdbx_txn_commit(m_txn);
                if (m_rc != MDBX_SUCCESS) throw MDBXException(
                    "Failed to commit writable transaction: (" + std::to_string(m_rc) + ") " + std::string(mdbx_strerror(m_rc)), m_rc);
                m_txn = nullptr;
                break;
            };
        }

        /// \copydoc ITransaction::rollback
        /// \throws MDBXException if rollback fails.
        void rollback() override final {
            if (!m_txn) throw MDBXException("No active transaction to rollback.");
            switch (m_mode) {
            case TransactionMode::READ_ONLY:
                m_rc = mdbx_txn_reset(m_txn);
                if (m_rc != MDBX_SUCCESS) throw MDBXException(
                    "Failed to reset read-only transaction: (" + std::to_string(m_rc) + ") " + std::string(mdbx_strerror(m_rc)), m_rc);
                m_txn = nullptr;
                break;
            case TransactionMode::WRITABLE:
                m_rc = mdbx_txn_abort(m_txn);
                if (m_rc != MDBX_SUCCESS) throw MDBXException(
                    "Failed to abort writable transaction: (" + std::to_string(m_rc) + ") " + std::string(mdbx_strerror(m_rc)), m_rc);
                m_txn = nullptr;
                break;
            };
        }

        /// \brief Returns the raw MDBX transaction handle.
        /// \return Pointer to the internal MDBX_txn structure.
		MDBX_txn *handle() const noexcept {
			return m_txn;
		}

    private:
        std::shared_ptr<MDBXConnection> m_connection; ///< MDBX connection used to create the transaction.
        TransactionMode m_mode;                       ///< Mode of the transaction (read-only or writable).
        MDBX_txn*       m_txn = nullptr;              ///< Pointer to the active MDBX transaction.
        int             m_rc  = 0;                    ///< Return code for the last MDBX operation.
    };

}; // namespace dfh::storage::mdbx

#endif // _DFH_STORAGE_MDBX_TRANSACTION_HPP_INCLUDED
