#pragma once
#ifndef _DFH_SQLITE_CONNECTION_UTILS_HPP_INCLUDED
#define _DFH_SQLITE_CONNECTION_UTILS_HPP_INCLUDED

/// \file utils.hpp
/// \brief

namespace dfh::storage::sqlite3 {

    /// \brief Executes a SQLite statement.
    /// \param stmt Pointer to the SQLite statement.
    /// \throws sqlite_exception if statement is null, or if an error occurs during execution.
    inline void execute(sqlite3_stmt *stmt) {
        if (!stmt) throw SQLiteException("Invalid statement pointer.");
        int err;
        for (;;) {
            while ((err = sqlite3_step(stmt)) == SQLITE_ROW);
            switch (err) {
            case SQLITE_DONE:
                return;
            case SQLITE_BUSY:
                sqlite3_sleep(SQLITE_CONTAINERS_BUSY_RETRY_DELAY_MS);
                continue;
            case SQLITE_FULL:
                throw SQLiteException("Disk full or IO error.", err);
            case SQLITE_IOERR:
                throw SQLiteException("Failed to insert data into database.", err);
            default:
                throw SQLiteException("SQLite error.", err);
            }
        }
    }

    /// \brief Executes a SQLite statement.
    /// \param sqlite_db Pointer to the SQLite database.
    /// \param stmt Pointer to the SQLite statement.
    /// \throws sqlite_exception if the database or statement is null, or if an error occurs during execution.
    inline void execute(sqlite3 *sqlite_db, sqlite3_stmt *stmt) {
        if (!sqlite_db || !stmt) throw SQLiteException("Invalid database or statement pointer.");
        int err;
        for (;;) {
            while ((err = sqlite3_step(stmt)) == SQLITE_ROW);
            switch (err) {
            case SQLITE_DONE:
                return;
            case SQLITE_BUSY:
                sqlite3_sleep(SQLITE_CONTAINERS_BUSY_RETRY_DELAY_MS);
                continue;
            case SQLITE_FULL:
                throw SQLiteException("Disk full or IO error: " + std::string(sqlite3_errmsg(sqlite_db)) + ". Error code: " + std::to_string(err), err);
            case SQLITE_IOERR:
                throw SQLiteException("Failed to insert data into database: " + std::string(sqlite3_errmsg(sqlite_db)) + ". Error code: " + std::to_string(err), err);
            default:
                throw SQLiteException("SQLite error: " + std::string(sqlite3_errmsg(sqlite_db)) + ". Error code: " + std::to_string(err), err);
            }
        }
    }

    /// \brief Prepares and executes a SQLite statement given as a string.
    /// \param sqlite_db Pointer to the SQLite database.
    /// \param query The SQL query to execute.
    /// \throws sqlite_exception if the database pointer is null, the request is empty, or if an error occurs during execution.
    inline void execute(sqlite3 *sqlite_db, const char *query) {
        if (!sqlite_db) throw SQLiteException("Invalid database pointer.");
        if (!query || std::strlen(query) == 0) throw SQLiteException("Empty SQL request.");
        int err;
        do {
            err = sqlite3_exec(sqlite_db, query, nullptr, nullptr, nullptr);
            if (err == SQLITE_BUSY) {
                sqlite3_sleep(SQLITE_CONTAINERS_BUSY_RETRY_DELAY_MS);
            } else
            if (err != SQLITE_OK) {
                std::string err_msg = "SQLite error during prepare: ";
                err_msg += sqlite3_errmsg(sqlite_db);
                err_msg += ". Error code: ";
                err_msg += std::to_string(err);
                throw SQLiteException(err_msg, err);
            }
        } while (err == SQLITE_BUSY);
    }

    /// \brief Prepares and executes a SQLite statement given as a string.
    /// \param sqlite_db Pointer to the SQLite database.
    /// \param query The SQL query to execute.
    /// \throws sqlite_exception if the database pointer is null, the request is empty, or if an error occurs during execution.
    inline void execute(sqlite3 *sqlite_db, const std::string &query) {
        execute(sqlite_db, query.c_str());
    }

//------------------------------------------------------------------------------

    /// \brief Gets the SQLite data type as a string for the given type.
    /// \tparam T The type for which SQLite data type is queried.
    /// \return The SQLite data type as a string.
    template<typename T>
    inline std::string get_sqlite_type(
            typename std::enable_if<std::is_integral<T>::value>::type* = 0) {
        return "INTEGER";
    }

    template<typename T>
    inline std::string get_sqlite_type(
            typename std::enable_if<std::is_floating_point<T>::value>::type* = 0) {
        return "REAL";
    }

    template<typename T>
    inline std::string get_sqlite_type(
            typename std::enable_if<std::is_same<T, std::string>::value>::type* = 0) {
        return "TEXT";
    }

    template<typename T>
    inline std::string get_sqlite_type(
            typename std::enable_if<
                !std::is_integral<T>::value &&
                !std::is_floating_point<T>::value &&
                std::is_trivially_copyable<T>::value>::type* = 0) {
        return "BLOB";
    }

    template<typename T>
    inline typename std::enable_if<
        std::is_trivially_copyable<
            typename T::value_type>::value &&
            std::is_same<T, std::vector<typename T::value_type>>::value,
            std::string
        >::type get_sqlite_type() {
        return "BLOB";
    }

    template<typename T>
    inline typename std::enable_if<
        std::is_trivially_copyable<
            typename T::value_type>::value &&
            std::is_same<T, std::deque<typename T::value_type>>::value,
            std::string
        >::type get_sqlite_type() {
        return "BLOB";
    }

//------------------------------------------------------------------------------

    /// \brief Adds a value to a container (set or unordered_set).
    /// \tparam ContainerT The type of container (set or unordered_set).
    /// \tparam T The type of value to add.
    /// \param container The container to which the value will be added.
    /// \param value The value to add.
    template<template <class...> class ContainerT, class T>
    inline void add_value(ContainerT<T> &container, T &value,
            typename std::enable_if<
                std::is_same<ContainerT<T>, std::set<T>>::value ||
                std::is_same<ContainerT<T>, std::unordered_set<T>>::value>::type* = 0) {
        container.insert(std::move(value));
    }

    /// \brief Adds a value to a container (multiset or unordered_multiset).
    /// \tparam ContainerT The type of container (multiset or unordered_multiset).
    /// \tparam T The type of value to add.
    /// \param container The container to which the value will be added.
    /// \param value The value to add.
    /// \param value_count Number of times to add the value (default is 0, meaning add once).
    template<template <class...> class ContainerT, class T>
    inline void add_value(ContainerT<T> &container, const T &value, const size_t& value_count = 0,
            typename std::enable_if<
                std::is_same<ContainerT<T>, std::multiset<T>>::value ||
                std::is_same<ContainerT<T>, std::unordered_multiset<T>>::value>::type* = 0) {
        for (size_t i = 0; i < value_count; ++i) {
            container.insert(value);
        }
    }

    /// \brief Adds a value to a container (list, vector, or deque).
    /// \tparam ContainerT The type of container (list, vector, or deque).
    /// \tparam T The type of value to add.
    /// \param container The container to which the value will be added.
    /// \param value The value to add.
    template<template <class...> class ContainerT, class T>
    inline void add_value(ContainerT<T> &container, T &value,
            typename std::enable_if<
                std::is_same<ContainerT<T>, std::list<T>>::value ||
                std::is_same<ContainerT<T>, std::vector<T>>::value ||
                std::is_same<ContainerT<T>, std::deque<T>>::value>::type* = 0) {
        container.emplace(container.end(), std::move(value));
    }

    /// \brief Adds a value to a container (list, vector, or deque).
    /// \tparam ContainerT The type of container (list, vector, or deque).
    /// \tparam T The type of value to add.
    /// \param container The container to which the value will be added.
    /// \param value The value to add.
    /// \param value_count Number of times to add the value (default is 0, meaning add once).
    template<template <class...> class ContainerT, class T>
    inline void add_value(ContainerT<T> &container, const T &value, const size_t& value_count,
            typename std::enable_if<
                std::is_same<ContainerT<T>, std::list<T>>::value ||
                std::is_same<ContainerT<T>, std::vector<T>>::value ||
                std::is_same<ContainerT<T>, std::deque<T>>::value>::type* = 0) {
        for (size_t i = 0; i < value_count; ++i) {
            container.emplace(container.end(), value);
        }
    }

    /// \brief Adds a value to a container (std::set).
    /// \tparam ContainerT The type of container (std::set).
    /// \tparam T The type of value to add.
    /// \param container The container to which the value will be added.
    /// \param value The value to add.
    /// \param value_count Number of times to add the value (default is 0, meaning add once).
    template<template <class...> class ContainerT, class T>
    inline void add_value(ContainerT<T> &container, const T &value, const size_t& value_count,
            typename std::enable_if<
                std::is_same<ContainerT<T>, std::set<T>>::value ||
                std::is_same<ContainerT<T>, std::multiset<T>>::value ||
                std::is_same<ContainerT<T>, std::unordered_set<T>>::value ||
                std::is_same<ContainerT<T>, std::unordered_multiset<T>>::value>::type* = 0) {
        for (size_t i = 0; i < value_count; ++i) {
            container.emplace(value);
        }
    }

    /// \brief Adds a key-value pair to a container (multimap or unordered_multimap).
    /// \tparam ContainerT The type of container (multimap or unordered_multimap).
    /// \tparam KeyT The type of key.
    /// \tparam ValueT The type of value.
    /// \param container The container to which the key-value pair will be added.
    /// \param key The key to add.
    /// \param value The value to add.
    /// \param value_count Number of times to add the key-value pair (default is 0, meaning add once).
    template <template <class...> class ContainerT, typename KeyT, typename ValueT>
    inline void add_value(ContainerT<KeyT, ValueT> &container, KeyT& key, ValueT& value, const size_t& value_count,
        typename std::enable_if<
            std::is_same<ContainerT<KeyT, ValueT>, std::multimap<KeyT, ValueT>>::value ||
            std::is_same<ContainerT<KeyT, ValueT>, std::unordered_multimap<KeyT, ValueT>>::value>::type* = 0) {
        for (size_t i = 0; i < value_count; ++i) {
            container.emplace(key, value);
        }
    }

//------------------------------------------------------------------------------

    /// \brief Retrieves values from a container (map or unordered_map) based on the key.
    /// \tparam ContainerT The type of container (map or unordered_map).
    /// \tparam KeyT The type of key.
    /// \tparam ValueT The type of value.
    /// \param container The container from which to retrieve values.
    /// \param key The key to search for.
    /// \return A list of values associated with the key (empty if key not found).
    template <template <class...> class ContainerT, typename KeyT, typename ValueT>
    inline std::list<ValueT> get_values(ContainerT<KeyT, ValueT> &container, const KeyT& key,
        typename std::enable_if<
            std::is_same<ContainerT<KeyT, ValueT>, std::map<KeyT, ValueT>>::value ||
            std::is_same<ContainerT<KeyT, ValueT>, std::unordered_map<KeyT, ValueT>>::value>::type* = 0) {
        std::list<ValueT> values;
        auto it = container.find(key);
        if (it == container.end()) return values;
        values.push_back(it->second);
        return values;
    }

    /// \brief Retrieves values from a container (map or unordered_map) based on the key.
    /// \tparam ContainerT The type of container (map or unordered_map).
    /// \tparam ValueContainerT The type of container used for values (e.g., vector, list).
    /// \tparam KeyT The type of key.
    /// \tparam ValueT The type of value.
    /// \param container The container from which to retrieve values.
    /// \param key The key to search for.
    /// \return A list of values associated with the key (empty if key not found).
    template<template <class...> class ContainerT, template <class...> class ValueContainerT, typename KeyT, typename ValueT>
    inline std::list<ValueT> get_values(ContainerT<KeyT, ValueContainerT<ValueT>> &container, const KeyT& key,
        typename std::enable_if<
            std::is_same<ContainerT<KeyT, ValueT>, std::map<KeyT, ValueContainerT<ValueT>>>::value ||
            std::is_same<ContainerT<KeyT, ValueT>, std::unordered_map<KeyT, ValueContainerT<ValueT>>>::value>::type* = 0) {
        auto it = container.find(key);
        std::list<ValueT> values;
        for (const auto& item : it->second) {
            values.push_back(item);
        }
        return values;
    }

    /// \brief Retrieves values from a container (multimap or unordered_multimap) based on the key.
    /// \tparam ContainerT The type of container (multimap or unordered_multimap).
    /// \tparam KeyT The type of key.
    /// \tparam ValueT The type of value.
    /// \param container The container from which to retrieve values.
    /// \param key The key to search for.
    /// \return A list of values associated with the key (empty if key not found).
    template <template <class...> class ContainerT, typename KeyT, typename ValueT>
    inline std::list<ValueT> get_values(ContainerT<KeyT, ValueT> &container, const KeyT& key,
        typename std::enable_if<
            std::is_same<ContainerT<KeyT, ValueT>, std::multimap<KeyT, ValueT>>::value ||
            std::is_same<ContainerT<KeyT, ValueT>, std::unordered_multimap<KeyT, ValueT>>::value>::type* = 0) {
        auto range = container.equal_range(key);
        std::list<ValueT> values;
        for (auto it = range.first; it != range.second; ++it) {
            values.push_back(it->second);
        }
        return values;
    }

//------------------------------------------------------------------------------

    template <typename T>
    bool byte_compare(const T& a, const T& b) {
        return std::memcmp(&a, &b, sizeof(T)) == 0;
    }

}; // namespace dfh::storage::sqlite3

#endif // _DFH_SQLITE_CONNECTION_UTILS_HPP_INCLUDED
