#pragma once
#ifndef _DFH_SQLITE_CONTAINER_UTILS_HPP_INCLUDED
#define _DFH_SQLITE_CONTAINER_UTILS_HPP_INCLUDED

/// \file SQLiteContainerUtils.hpp
/// \brief Utility functions for adding and retrieving values in STL containers.

#include <type_traits>
#include <set>
#include <unordered_set>
#include <list>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <multimap>
#include <unordered_multimap>

namespace dfh::storage::sqlite3 {

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

    /// \brief Bytewise comparison of two trivially copyable objects.
    /// \tparam T The type of objects to compare.
    /// \param a The first object.
    /// \param b The second object.
    /// \return True if the objects are bytewise equal, false otherwise.
    template <typename T>
    bool byte_compare(const T& a, const T& b) {
        return std::memcmp(&a, &b, sizeof(T)) == 0;
    }

};

#endif // _DFH_SQLITE_CONTAINER_UTILS_HPP_INCLUDED
