#pragma once
#ifndef _DFH_STORAGE_MDBX_METADATA_BD_HPP_INCLUDED
#define _DFH_STORAGE_MDBX_METADATA_BD_HPP_INCLUDED

/// \file MetadataBD.hpp
/// \brief Defines the MetadataBD class for managing high-level storage metadata in an MDBX database.

namespace dfh::storage::mdbx {

    /// \class MetadataBD
    /// \brief Handles saving, loading, and management of global storage metadata in an MDBX database.
    class MetadataBD {
    public:
        /// \brief Constructs the metadata backend with the given MDBX connection.
        /// \param connection Pointer to an active MDBXConnection instance.
        MetadataBD(MDBXConnection* connection)
            : m_connection(connection) {
        }

        /// \brief Destructor that closes the database handle if it was opened.
        ~MetadataBD() {
            if (!m_dbi_metadata) return;
            mdbx_dbi_close(m_connection->env_handle(), m_dbi_metadata);
        }

        /// \brief Opens the metadata database (creating if necessary).
        /// \param txn Active transaction to use for database opening.
        /// \throws MDBXException if the operation fails.
        void start(MDBXTransaction* txn) {
            int rc = mdbx_dbi_open(txn->handle(), "metadata", MDBX_DB_DEFAULTS | MDBX_CREATE, &m_dbi_metadata);
            if (rc != MDBX_SUCCESS) throw MDBXException("Failed to open 'metadata' database: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
        }

        /// \brief Closes the metadata database.
        /// \throws MDBXException if closing fails.
        void stop() {
            if (!m_dbi_metadata) return;
            int rc = mdbx_dbi_close(m_connection->env_handle(), m_dbi_metadata);
            if (rc != MDBX_SUCCESS) throw MDBXException("Failed to close database: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
        }

        /// \brief Loads global storage metadata from the database.
        /// \param txn Active MDBX transaction.
        /// \param metadata Output structure to populate with loaded metadata.
        /// \return True if metadata was found, false otherwise.
        /// \throws MDBXException if a read error occurs.
        bool fetch(MDBXTransaction *txn, dfh::storage::StorageMetadata &metadata) {
            static constexpr char key_str[] = "storage_metadata";
            MDBX_val db_key{ const_cast<char*>(key_str), sizeof(key_str) - 1 };
            MDBX_val db_data;
            int rc = mdbx_get(txn->handle(), m_dbi_metadata, &db_key, &db_data);
            if (rc == MDBX_NOTFOUND) return false;
            if (rc != MDBX_SUCCESS) throw MDBXException("Failed to get 'storage_metadata': (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
            metadata.deserialize(static_cast<const uint8_t*>(db_data.iov_base), db_data.iov_len);
            return true;
        }

        /// \brief Saves (or replaces) the current storage metadata in the database.
        /// \param txn Active MDBX transaction.
        /// \param metadata Metadata object to serialize and store.
        /// \throws MDBXException if the write operation fails.
        void upsert(MDBXTransaction *txn, const dfh::storage::StorageMetadata &metadata) {
            std::vector<uint8_t> out = metadata.serialize();
            static constexpr char key_str[] = "storage_metadata";
            MDBX_val db_key{ const_cast<char*>(key_str), sizeof(key_str) - 1 };
            MDBX_val db_data{ out.data(), out.size() };
            int rc = mdbx_put(txn->handle(), m_dbi_metadata, &db_key, &db_data, MDBX_UPSERT);
            if (rc != MDBX_SUCCESS) throw MDBXException("Failed to put 'storage_metadata': (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
        }

        /// \brief Erases all metadata records from the database.
        /// \param txn Active transaction.
        /// \warning This operation is irreversible.
        void erase_all_data(MDBXTransaction *txn) {
            erase_all_entries(txn->handle(), m_dbi_metadata);
        }

    private:
        MDBXConnection*               m_connection;       ///< Pointer to the MDBX connection.
        dfh::storage::StorageMetadata m_metadata;         ///< Cached metadata (currently unused internally).
        MDBX_dbi                      m_dbi_metadata = 0; ///< Database handle for metadata.
    };

} // namespace dfh::storage::mdbx

#endif // _DFH_STORAGE_MDBX_METADATA_BD_HPP_INCLUDED
