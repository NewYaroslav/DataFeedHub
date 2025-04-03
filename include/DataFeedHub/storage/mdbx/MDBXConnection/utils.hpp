#pragma once
#ifndef _DFH_STORAGE_MDBX_UTILS_HPP_INCLUDED
#define _DFH_STORAGE_MDBX_UTILS_HPP_INCLUDED

/// \file utils.hpp
/// \brief Utility functions for working with MDBX using 32/64-bit keys.

// See: https://libmdbx.dqdkfa.ru/

namespace dfh::storage::mdbx {

    /// \brief Inserts or updates a fixed-size trivially copyable object using an integral key (32-bit or 64-bit).
    /// \tparam Key Type of the key (must be 32 or 64-bit integral type).
    /// \tparam T Type of the object (must be trivially copyable).
    /// \param txn MDBX transaction handle.
    /// \param dbi Target database handle.
    /// \param key Key to store the object under.
    /// \param data Object to store.
    /// \param batch_write Whether to use batch insert flags (MDBX_APPEND | MDBX_UPSERT).
    /// \throws MDBXException if the operation fails.
    template<typename Key, typename T>
    void put_fixed_key(MDBX_txn* txn, MDBX_dbi dbi, Key key, const T& data, bool batch_write = false) {
        static_assert(std::is_same<Key, uint32_t>::value || std::is_same<Key, uint64_t>::value,"Key must be either uint32_t or uint64_t (supported by MDBX)");

        MDBX_val db_key{std::addressof(key), sizeof(Key)};
        MDBX_val db_data{const_cast<void*>(static_cast<const void*>(std::addressof(data))), sizeof(T)};

        int rc = mdbx_put(txn, dbi, &db_key, &db_data, batch_write ? (MDBX_APPEND | MDBX_UPSERT) : MDBX_UPSERT);
        if (rc != MDBX_SUCCESS) {
            throw MDBXException("Failed to put fixed-size object: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
        }
    }

    /// \brief Inserts or updates raw binary data using an integral key (32-bit or 64-bit).
    /// \tparam Key Type of the key (must be 32-bit or 64-bit integral type).
    /// \param txn MDBX transaction handle.
    /// \param dbi Target database handle.
    /// \param key Key value (e.g., symbol key, timestamped key).
    /// \param data Pointer to raw binary data.
    /// \param size Size of binary data in bytes.
    /// \param batch_write Whether to use MDBX_APPEND | MDBX_UPSERT for batch mode.
    /// \throws MDBXException if the operation fails or input is invalid.
    template<typename Key>
    void put_raw_key(MDBX_txn* txn, MDBX_dbi dbi, Key key, const void* data, size_t size, bool batch_write = false) {
        static_assert(std::is_same<Key, uint32_t>::value || std::is_same<Key, uint64_t>::value,"Key must be either uint32_t or uint64_t (supported by MDBX)");

        if (!data || size == 0) {
            throw MDBXException("Invalid input: data is null or size is zero.");
        }

        MDBX_val db_key{std::addressof(key), sizeof(Key)};
        MDBX_val db_data{const_cast<void*>(data), size};

        int rc = mdbx_put(txn, dbi, &db_key, &db_data, batch_write ? (MDBX_APPEND | MDBX_UPSERT) : MDBX_UPSERT);
        if (rc != MDBX_SUCCESS) {
            throw MDBXException("Failed to put raw data: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
        }
    }

    /// \brief Retrieves raw binary data using an integral key (32-bit or 64-bit).
    /// \tparam Key Type of the key (must be 32-bit or 64-bit integral type).
    /// \param txn MDBX transaction handle.
    /// \param dbi Target database handle.
    /// \param key Key to retrieve data by.
    /// \param out_data Output buffer to store the raw binary data.
    /// \return True if data was found, false otherwise.
    /// \throws MDBXException if retrieval fails.
    template<typename Key>
    bool get_raw_key(MDBX_txn* txn, MDBX_dbi dbi, Key key, std::vector<uint8_t>& out_data) {
        static_assert(std::is_same<Key, uint32_t>::value || std::is_same<Key, uint64_t>::value,"Key must be either uint32_t or uint64_t (supported by MDBX)");

        MDBX_val db_key{std::addressof(key), sizeof(Key)};
        MDBX_val db_data;

        int rc = mdbx_get(txn, dbi, &db_key, &db_data);
        if (rc == MDBX_NOTFOUND) return false;
        if (rc != MDBX_SUCCESS) {
            throw MDBXException("Failed to get raw data: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
        }

        out_data.assign(
            static_cast<const uint8_t*>(db_data.iov_base),
            static_cast<const uint8_t*>(db_data.iov_base) + db_data.iov_len
        );

        return true;
    }

    /// \brief Retrieves a fixed-size object using an integral key (32-bit or 64-bit).
    /// \tparam Key Integral key type (must be 32-bit or 64-bit).
    /// \tparam T Type of object to retrieve (e.g., struct).
    /// \param txn MDBX transaction handle.
    /// \param dbi Target database handle.
    /// \param key Key to lookup.
    /// \param out_obj Output object to store result.
    /// \return True if found, false if not.
    /// \throws MDBXException if retrieval fails.
    template<typename Key, typename T>
    bool get_fixed_key(MDBX_txn* txn, MDBX_dbi dbi, Key key, T& out_obj) {
        static_assert(std::is_same<Key, uint32_t>::value || std::is_same<Key, uint64_t>::value,"Key must be either uint32_t or uint64_t (supported by MDBX)");
        static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");

        MDBX_val db_key{std::addressof(key), sizeof(Key)};
        MDBX_val db_data;

        int rc = mdbx_get(txn, dbi, &db_key, &db_data);
        if (rc == MDBX_NOTFOUND) return false;
        if (rc != MDBX_SUCCESS) {
            throw MDBXException("Failed to get object: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
        }

        if (db_data.iov_len != sizeof(T)) {
            throw MDBXException("Data size mismatch for type T.");
        }

        std::memcpy(&out_obj, db_data.iov_base, sizeof(T));
        return true;
    }

    /// \brief Deletes a record from the database using a 32-bit or 64-bit key.
    /// \tparam Key Must be uint32_t or uint64_t.
    /// \param txn MDBX transaction handle.
    /// \param dbi Target database handle.
    /// \param key Key to delete.
    /// \throws MDBXException if deletion fails (except for MDBX_NOTFOUND).
    template<typename Key>
    inline void erase_key(MDBX_txn* txn, MDBX_dbi dbi, Key key) {
        static_assert(std::is_same<Key, uint32_t>::value || std::is_same<Key, uint64_t>::value,"Key must be either uint32_t or uint64_t (supported by MDBX)");
        MDBX_val db_key{std::addressof(key), sizeof(Key)};
        int rc = mdbx_del(txn, dbi, &db_key, nullptr);
        if (rc != MDBX_SUCCESS && rc != MDBX_NOTFOUND) {
            throw MDBXException("Failed to delete key: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
        }
    }

    /// \brief Deletes all key-value pairs from the given MDBX database.
    /// \param txn MDBX transaction handle.
    /// \param dbi Target database handle.
    /// \throws MDBXException if cursor operations fail.
    inline void erase_all_entries(MDBX_txn* txn, MDBX_dbi dbi) {
        MDBX_cursor* cursor = nullptr;
        int rc = mdbx_cursor_open(txn, dbi, &cursor);
        if (rc != MDBX_SUCCESS) {
            throw MDBXException("Failed to open cursor for erase_all_entries: (" +
                                std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
        }

        MDBX_val key, data;
        while ((rc = mdbx_cursor_get(cursor, &key, &data, MDBX_NEXT)) == MDBX_SUCCESS) {
            int del_rc = mdbx_cursor_del(cursor, MDBX_CURRENT);
            if (del_rc != MDBX_SUCCESS) {
                mdbx_cursor_close(cursor);
                throw MDBXException("Failed to delete entry: (" +
                                    std::to_string(del_rc) + ") " + std::string(mdbx_strerror(del_rc)));
            }
        }

        mdbx_cursor_close(cursor);

        if (rc != MDBX_NOTFOUND) {
            throw MDBXException("Failed during erase_all_entries iteration: (" +
                                std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
        }
    }

    /// \brief Deletes entries from a table where a key (32-bit or 64-bit) matches a masked value.
    /// \tparam KeyType Either uint32_t or uint64_t.
    /// \param txn MDBX transaction handle.
    /// \param dbi Target database handle.
    /// \param mask Bitmask to apply to each key.
    /// \param masked_value Expected result after applying mask.
    /// \throws MDBXException if cursor or delete operations fail.
    template<typename KeyType>
    inline void erase_key_masked(MDBX_txn* txn, MDBX_dbi dbi, KeyType mask, KeyType masked_value) {
        static_assert(std::is_same<KeyType, uint32_t>::value || std::is_same<KeyType, uint64_t>::value, "KeyType must be uint32_t or uint64_t (supported by MDBX)");

        MDBX_cursor* cursor = nullptr;
        int rc = mdbx_cursor_open(txn, dbi, &cursor);
        if (rc != MDBX_SUCCESS) {
            throw MDBXException("Failed to open cursor for erase_key_masked: (" +
                                std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
        }

        MDBX_val db_key, db_data;
        while ((rc = mdbx_cursor_get(cursor, &db_key, &db_data, MDBX_NEXT)) == MDBX_SUCCESS) {
            if (db_key.iov_len != sizeof(KeyType)) continue;

            KeyType key;
            std::memcpy(&key, db_key.iov_base, sizeof(KeyType));

            if ((key & mask) == masked_value) {
                int del_rc = mdbx_cursor_del(cursor, MDBX_CURRENT);
                if (del_rc != MDBX_SUCCESS) {
                    mdbx_cursor_close(cursor);
                    throw MDBXException("Failed to delete entry in erase_key_masked: (" +
                                        std::to_string(del_rc) + ") " + std::string(mdbx_strerror(del_rc)));
                }
            }
        }

        mdbx_cursor_close(cursor);
        if (rc != MDBX_NOTFOUND) {
            throw MDBXException("Cursor iteration failed in erase_key_masked: (" +
                                std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
        }
    }

    /// \brief Retrieves all fixed-size records with integral keys into a vector of values.
    /// \tparam Key Must be uint32_t or uint64_t.
    /// \tparam T Value type (must be trivially copyable).
    /// \param txn MDBX transaction handle.
    /// \param dbi Target database handle.
    /// \param out_vector Vector to receive values only (keys are ignored).
    /// \return True if any data found, false if table is empty.
    /// \throws MDBXException if operation fails.
    template<typename Key, typename T>
    bool get_all_fixed_values(MDBX_txn* txn, MDBX_dbi dbi, std::vector<T>& out_vector) {
        static_assert(std::is_same<Key, uint32_t>::value || std::is_same<Key, uint64_t>::value,"Key must be either uint32_t or uint64_t (supported by MDBX)");
        static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");

        MDBX_cursor* cursor = nullptr;
        int rc = mdbx_cursor_open(txn, dbi, &cursor);
        if (rc != MDBX_SUCCESS)
            throw MDBXException("Failed to open cursor: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));

        MDBX_val db_key, db_data;
        while ((rc = mdbx_cursor_get(cursor, &db_key, &db_data, MDBX_NEXT)) == MDBX_SUCCESS) {
            if (db_key.iov_len != sizeof(Key)) {
                mdbx_cursor_close(cursor);
                throw MDBXException("Invalid key size.");
            }
            if (db_data.iov_len != sizeof(T)) {
                mdbx_cursor_close(cursor);
                throw MDBXException("Invalid data size: (" + std::to_string(db_data.iov_len) + ")");
            }

            T obj;
            std::memcpy(&obj, db_data.iov_base, sizeof(T));
            out_vector.push_back(std::move(obj));
        }

        mdbx_cursor_close(cursor);
        if (rc != MDBX_NOTFOUND) {
            throw MDBXException("Cursor iteration failed: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
        }

        return !out_vector.empty();
    }

    /// \brief Retrieves all fixed-size records with integral keys into a map.
    /// \tparam Key Must be uint32_t or uint64_t.
    /// \tparam T Type of value to read (must be trivially copyable).
    /// \param txn MDBX transaction handle.
    /// \param dbi Target database handle.
    /// \param out_map Map to receive results (key -> object).
    /// \return True if any data was read, false if table is empty.
    /// \throws MDBXException if iteration or reading fails.
    template<typename Key, typename T>
    bool get_all_fixed_map(MDBX_txn* txn, MDBX_dbi dbi, std::unordered_map<Key, T>& out_map) {
        static_assert(std::is_same<Key, uint32_t>::value || std::is_same<Key, uint64_t>::value,"Key must be either uint32_t or uint64_t (supported by MDBX)");
        static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");

        MDBX_cursor* cursor = nullptr;
        int rc = mdbx_cursor_open(txn, dbi, &cursor);
        if (rc != MDBX_SUCCESS)
            throw MDBXException("Failed to open cursor: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));

        MDBX_val db_key, db_data;
        while ((rc = mdbx_cursor_get(cursor, &db_key, &db_data, MDBX_NEXT)) == MDBX_SUCCESS) {
            if (db_key.iov_len != sizeof(Key)) {
                mdbx_cursor_close(cursor);
                throw MDBXException("Invalid key size");
            }
            if (db_data.iov_len != sizeof(T)) {
                mdbx_cursor_close(cursor);
                throw MDBXException("Invalid data size: (" + std::to_string(db_data.iov_len) + ")");
            }

            Key key;
            T obj;
            std::memcpy(&key, db_key.iov_base, sizeof(Key));
            std::memcpy(&obj, db_data.iov_base, sizeof(T));
            out_map.emplace(key, std::move(obj));
        }

        mdbx_cursor_close(cursor);
        if (rc != MDBX_NOTFOUND) {
            throw MDBXException("Cursor iteration failed: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
        }

        return !out_map.empty();
    }

}; // namespace dfh::storage::mdbx

#endif // _DFH_STORAGE_MDBX_UTILS_HPP_INCLUDED
