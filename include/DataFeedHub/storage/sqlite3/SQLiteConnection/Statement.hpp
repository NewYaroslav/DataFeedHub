#pragma once
#ifndef _DFH_SQLITE_STATEMENT_HPP_INCLUDED
#define _DFH_SQLITE_STATEMENT_HPP_INCLUDED

/// \file Statement.hpp
/// \brief Wrapper class for SQLite prepared statements.

#include <sqlite3.h>
#include <string>
#include <vector>
#include <cstring>
#include <stdexcept>

#ifndef DFH_SQLITE_BUSY_RETRY_DELAY_MS
#define DFH_SQLITE_BUSY_RETRY_DELAY_MS 50
#endif

namespace dfh::storage::sqlite3 {

    /// \class Statement
    /// \brief Represents a prepared statement in SQLite.
    class Statement {
    public:

        /// \brief Default constructor.
        Statement() = default;

        /// \brief Destructor finalizes the prepared statement.
        ~Statement() {
            finalize();
        }

        /// \brief Initializes the statement with a query.
        /// \param sqlite_ptr Pointer to the SQLite database.
        /// \param query SQL query to prepare.
        /// \throws SQLiteException if the query preparation fails.
        void init(sqlite3 *sqlite_ptr, const char *query) {
            int err;
            do {
                err = sqlite3_prepare_v2(sqlite_ptr, query, -1, &m_stmt, nullptr);
                if (err == SQLITE_BUSY) {
                    sqlite3_sleep(DFH_SQLITE_BUSY_RETRY_DELAY_MS);
                } else
                if (err != SQLITE_OK) {
                    std::string err_msg = "Failed to prepare SQL statement: ";
                    err_msg += std::string(query);
                    err_msg += ". Error code: ";
                    err_msg += std::to_string(err);
                    throw SQLiteException(err_msg, err);
                }
            } while (err == SQLITE_BUSY);
            m_max_length = sqlite3_limit(sqlite_ptr, SQLITE_LIMIT_LENGTH, -1);
        }

        /// \brief Initializes the statement.
        /// \param sqlite_ptr Pointer to the SQLite database.
        /// \param query SQL query to prepare.
        /// \throws SQLiteException if the query preparation fails.
        void init(sqlite3 *sqlite_ptr, const std::string &query) {
            init(sqlite_ptr, query.c_str());
        }

        /// \brief Gets the prepared SQLite statement.
        /// \return Pointer to the prepared SQLite statement.
        sqlite3_stmt *get_stmt() noexcept {
            return m_stmt;
        }

        /// \brief Extracts a value from a SQLite statement column.
        /// \param index Index of the column to extract.
        /// \return The extracted value.
        template<typename T>
        T extract_column(const int &index,
                typename std::enable_if<std::is_integral<T>::value>::type* = 0) {
            return static_cast<T>(sqlite3_column_int64(m_stmt, index));
        }

        template<typename T>
        T extract_column(const int &index,
                typename std::enable_if<std::is_floating_point<T>::value>::type* = 0) {
            return static_cast<T>(sqlite3_column_double(m_stmt, index));
        }

        template<typename T>
        T extract_column(const int &index,
                typename std::enable_if<std::is_same<T, std::string>::value>::type* = 0) {
            const unsigned char *text = sqlite3_column_text(m_stmt, index);
            return text ? std::string(reinterpret_cast<const char*>(text)) : std::string();
        }

        template<typename T>
        T extract_column(const int &index,
                typename std::enable_if<std::is_same<T, std::vector<char>>::value>::type* = 0) {
            const void* blob = sqlite3_column_blob(m_stmt, index);
            const int blob_size = sqlite3_column_bytes(m_stmt, index);
            return std::vector<char>(static_cast<const char*>(blob), static_cast<const char*>(blob) + blob_size);
        }

        template<typename T>
        T extract_column(const int &index,
                typename std::enable_if<std::is_same<T, std::vector<uint8_t>>::value>::type* = 0) {
            const void* blob = sqlite3_column_blob(m_stmt, index);
            const int blob_size = sqlite3_column_bytes(m_stmt, index);
            return std::vector<uint8_t>(static_cast<const uint8_t*>(blob), static_cast<const uint8_t*>(blob) + blob_size);
        }

        template<typename T>
        inline T extract_column(const int &index,
                typename std::enable_if<
                    !std::is_integral<T>::value &&
                    !std::is_floating_point<T>::value &&
                    !std::is_same<T, std::string>::value &&
                    !std::is_same<T, std::vector<char>>::value &&
                    !std::is_same<T, std::vector<uint8_t>>::value &&
                    std::is_trivially_copyable<T>::value
                >::type* = 0) {
            T value;
            const void* blob = sqlite3_column_blob(m_stmt, index);
            const int blob_size = sqlite3_column_bytes(m_stmt, index);
            if (blob_size == sizeof(T)) {
                std::memcpy(&value, blob, sizeof(T));
            } else {
                throw SQLiteException("Blob size does not match POD size.");
            }
            return value;
        }

        /// \brief Binds a value to a SQLite statement.
        /// \param index Index of the parameter to bind.
        /// \param value The value to bind.
        /// \throws SQLiteException if the binding operation fails.
        template<typename T>
        void bind_value(const int &index, const T& value,
                typename std::enable_if<std::is_integral<T>::value>::type* = 0) {
            int result = sqlite3_bind_int64(m_stmt, index, static_cast<int64_t>(value));
            if (result != SQLITE_OK) {
                throw SQLiteException("Failed to bind integer value. Error code: " + std::to_string(result), result);
            }
        }

        /// \brief Binds a floating-point value to a SQLite statement.
        /// \param index Index of the parameter to bind.
        /// \param value The value to bind.
        /// \throws SQLiteException if the binding operation fails.
        template<typename T>
        void bind_value(const int &index, const T& value,
                typename std::enable_if<std::is_floating_point<T>::value>::type* = 0) {
            int result = sqlite3_bind_double(m_stmt, index, static_cast<double>(value));
            if (result != SQLITE_OK) {
                throw SQLiteException("Failed to bind floating-point value. Error code: " + std::to_string(result), result);
            }
        }

        /// \brief Binds a string value to a SQLite statement.
        /// \param index Index of the parameter to bind.
        /// \param value The value to bind.
        /// \throws SQLiteException if the binding operation fails.
        template<typename T>
        void bind_value(const int &index, const T& value,
                typename std::enable_if<std::is_same<T, std::string>::value>::type* = 0) {
            if (value.size() > m_max_length) {
                throw SQLiteException("String value exceeds maximum length.");
            }
            int result = sqlite3_bind_text(m_stmt, index, value.c_str(), -1, SQLITE_STATIC);
            if (result != SQLITE_OK) {
                throw SQLiteException("Failed to bind string value. Error code: " + std::to_string(result), result);
            }
        }

        /// \brief Binds a vector of char as a blob to a SQLite statement.
        /// \param index Index of the parameter to bind.
        /// \param value The value to bind.
        /// \throws SQLiteException if the binding operation fails.
        template<typename T>
        void bind_value(const int &index, const T& value,
                typename std::enable_if<std::is_same<T, std::vector<char>>::value>::type* = 0) {
            if (value.size() > m_max_length) {
                throw SQLiteException("Blob value exceeds maximum length.");
            }
            int result = sqlite3_bind_blob(m_stmt, index, value.data(), value.size(), SQLITE_STATIC);
            if (result != SQLITE_OK) {
                throw SQLiteException("Failed to bind blob value (vector<char>). Error code: " + std::to_string(result), result);
            }
        }

        /// \brief Binds a vector of uint8_t as a blob to a SQLite statement.
        /// \param index Index of the parameter to bind.
        /// \param value The value to bind.
        /// \throws SQLiteException if the binding operation fails.
        template<typename T>
        void bind_value(const int &index, const T& value,
                typename std::enable_if<std::is_same<T, std::vector<uint8_t>>::value>::type* = 0) {
            if (value.size() > m_max_length) {
                throw SQLiteException("Blob value exceeds maximum length.");
            }
            int result = sqlite3_bind_blob(m_stmt, index, value.data(), value.size(), SQLITE_STATIC);
            if (result != SQLITE_OK) {
                throw SQLiteException("Failed to bind blob value (vector<uint8_t>). Error code: " + std::to_string(result), result);
            }
        }

        /// \brief Binds a trivially copyable POD value as a blob to a SQLite statement.
        /// \param index Index of the parameter to bind.
        /// \param value The value to bind.
        /// \throws SQLiteException if the binding operation fails.
        template<typename T>
        void bind_value(const int &index, const T& value,
                typename std::enable_if<
                    !std::is_integral<T>::value &&
                    !std::is_floating_point<T>::value &&
                    !std::is_same<T, std::string>::value &&
                    !std::is_same<T, std::vector<char>>::value &&
                    !std::is_same<T, std::vector<uint8_t>>::value &&
                    std::is_trivially_copyable<T>::value
                >::type* = 0) {
            int result = sqlite3_bind_blob(m_stmt, index, &value, sizeof(T), SQLITE_STATIC);
            if (result != SQLITE_OK) {
                throw SQLiteException("Failed to bind POD value as blob. Error code: " + std::to_string(result), result);
            }
        }

        /// \brief Resets the prepared statement.
        /// \throws SQLiteException if the reset operation fails.
        void reset() {
            const int err = sqlite3_reset(m_stmt);
            if (err == SQLITE_OK) return;
            throw SQLiteException("Failed to reset SQL statement. Error code: " + std::to_string(err), err);
        }

        /// \brief Clears all bindings on the prepared statement.
        /// \throws SQLiteException if the clear bindings operation fails.
        void clear_bindings() {
            const int err = sqlite3_clear_bindings(m_stmt);
            if (err == SQLITE_OK) return;
            throw SQLiteException("Failed to clear bindings on SQL statement. Error code: " + std::to_string(err), err);
        }

        /// \brief Executes the prepared statement.
        /// \param sqlite_ptr Pointer to the SQLite database.
        /// \throws SQLiteException if the execution fails.
        void execute(sqlite3 *sqlite_ptr) {
            dfh::storage::sqlite3::execute(sqlite_ptr, m_stmt);
        }

        /// \brief Executes the prepared statement.
        /// \throws SQLiteException if the execution fails.
        void execute() {
            dfh::storage::sqlite3::execute(m_stmt);
        }

        /// \brief Advances the prepared statement to the next row.
        /// \return SQLITE_ROW, SQLITE_DONE, or an error code.
        int step() {
            return sqlite3_step(m_stmt);
        }

        /// \brief Finalizes the prepared statement.
        void finalize() {
            if (!m_stmt) return;
            sqlite3_finalize(m_stmt);
            m_stmt = nullptr;
        }

    private:
        sqlite3_stmt *m_stmt = nullptr; ///< Pointer to the prepared SQLite statement.
        int m_max_length = 0;
    };

}; // namespace dfh::storage::sqlite3

#endif // _DFH_SQLITE_STATEMENT_HPP_INCLUDED
