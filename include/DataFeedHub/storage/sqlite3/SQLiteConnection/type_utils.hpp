#pragma once
#ifndef _DFH_SQLITE_TYPE_UTILS_HPP_INCLUDED
#define _DFH_SQLITE_TYPE_UTILS_HPP_INCLUDED

/// \file type_utils.hpp
/// \brief Utility functions for mapping C++ types to SQLite data types.

#include <string>
#include <type_traits>
#include <vector>
#include <deque>

namespace dfh::storage::sqlite3 {

    /// \brief Gets the SQLite data type as a string for the given type.
    /// \tparam T The type for which SQLite data type is queried.
    /// \return The SQLite data type as a string.
    template<typename T>
    inline std::string get_sqlite_type(
            typename std::enable_if<std::is_integral<T>::value>::type* = 0) {
        return "INTEGER";
    }

    /// \brief Maps C++ floating-point types to the SQLite "REAL" type.
    template<typename T>
    inline std::string get_sqlite_type(
            typename std::enable_if<std::is_floating_point<T>::value>::type* = 0) {
        return "REAL";
    }

    /// \brief Maps `std::string` to the SQLite "TEXT" type.
    template<typename T>
    inline std::string get_sqlite_type(
            typename std::enable_if<std::is_same<T, std::string>::value>::type* = 0) {
        return "TEXT";
    }

    /// \brief
    template<typename T>
    inline std::string get_sqlite_type(
            typename std::enable_if<
                !std::is_integral<T>::value &&
                !std::is_floating_point<T>::value &&
                std::is_trivially_copyable<T>::value>::type* = 0) {
        return "BLOB";
    }

    /// \brief
    template<typename T>
    inline typename std::enable_if<
        std::is_trivially_copyable<
            typename T::value_type>::value &&
            std::is_same<T, std::vector<typename T::value_type>>::value,
            std::string
        >::type get_sqlite_type() {
        return "BLOB";
    }

    /// \brief
    template<typename T>
    inline typename std::enable_if<
        std::is_trivially_copyable<
            typename T::value_type>::value &&
            std::is_same<T, std::deque<typename T::value_type>>::value,
            std::string
        >::type get_sqlite_type() {
        return "BLOB";
    }

}; // namespace dfh::storage::sqlite3

#endif // _DFH_SQLITE_TYPE_UTILS_HPP_INCLUDED
