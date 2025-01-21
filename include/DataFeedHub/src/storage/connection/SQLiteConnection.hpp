#pragma once
#ifndef _DFH_SQLITE_CONNECTION_HPP_INCLUDED
#define _DFH_SQLITE_CONNECTION_HPP_INCLUDED

namespace dfh::storage::sqlite3 {

    /// \class SQLiteConnection
    /// \brief
    class SQLiteConnection final : public IConnectionDB {
    public:

        /// \brief Configures the database connection.
        ///
        /// Sets the configuration for the database connection using an object
        /// implementing the IConfigDB interface.
        ///
        /// \param config A unique pointer to the database configuration object.
        /// \throws std::invalid_argument if the configuration is null.
        void configure(std::unique_ptr<IConfigDB> config) override {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            m_config = std::move(config);
        }

        /// \brief Establishes the database connection.
        ///
        /// Opens a connection to the SQLite database and applies configuration settings.
        /// \throws SQLiteException if the connection fails or configuration is invalid.
        void connect() override {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            if (m_sqlite_ptr) {
                throw SQLiteException("Database connection already exists.");
            }
            if (!m_config) {
                throw SQLiteException("No configuration provided.");
            }
            try {
                create_directories();
                open_db();
                configure_db();
            } catch (...) {
                sqlite3_close_v2(m_sqlite_ptr);
                m_sqlite_ptr = nullptr;
                throw;
            }
        }

        /// \brief Disconnect from the database.
        ///
        /// This method closes the current database connection.
        void disconnect() override {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            if (!m_sqlite_ptr) return;
            sqlite3_close_v2(m_sqlite_ptr);
            m_sqlite_ptr = nullptr;
        }

        /// \brief Checks if the database connection is active.
        /// \return True if the connection is active, false otherwise.
        bool is_connected() const override {
            std::lock_guard<std::mutex> locker(m_sqlite_mutex);
            return (m_sqlite_ptr != nullptr);
        }

        /// \brief Begins a database transaction.
        /// Starts a new transaction with the specified mode.
        /// \param mode The transaction mode (default: TransactionMode::DEFERRED).
        /// \throws SQLiteException if the transaction cannot be started.
        void begin(TransactionMode mode = TransactionMode::DEFERRED) override {
            m_stmt_begin[static_cast<size_t>(mode)].execute(m_sqlite_ptr);
        }

        /// \brief Commits the current transaction.
        /// Finalizes the transaction, making changes permanent.
        /// \throws SQLiteException if the transaction cannot be committed.
        void commit() override {
            m_stmt_commit.execute(m_sqlite_ptr);
        }

        /// \brief Rolls back the current transaction.
        /// Reverts changes made during the current transaction.
        /// \throws SQLiteException if the transaction cannot be rolled back.
        void rollback() override {
            m_stmt_rollback.execute(m_sqlite_ptr);
        }

        /// \brief Executes a raw SQL query.
        /// Allows executing raw SQL queries directly on the database.
        /// \param query A string containing the SQL query to execute.
        /// \throws SQLiteException if the query execution fails.
        void execute(const std::string& query) {
            execute(m_sqlite_ptr, query);
        }

        /// \brief Destructor that ensures proper cleanup of resources.
        virtual ~SQLiteConnection() {
            disconnect();
        }

    protected:
        sqlite3*            m_sqlite_ptr = nullptr;
        mutable std::mutex  m_sqlite_mutex;

        /// \brief Handles an exception by resetting and clearing bindings of prepared SQL statements.
        /// \param ex A pointer to the current exception (`std::exception_ptr`) that needs to be handled.
        /// \param stmts A vector of pointers to `SqliteStmt` objects, which will be reset and cleared.
        /// \param message A custom error message used when an unknown exception is encountered. Defaults to "Unknown error occurred."
        /// \throws sqlite_exception If an unknown exception or `std::exception` is encountered, it will be rethrown as `sqlite_exception`. If the caught exception is already `sqlite_exception`, it will be rethrown as is.
        void handle_exception(
                std::exception_ptr ex,
                const std::vector<SQLiteStatement*>& stmts,
                const std::string& message = "Unknown error occurred.") const {
            try {
                for (auto* stmt : stmts) {
                    stmt->reset();
                    stmt->clear_bindings();
                }
                if (ex) std::rethrow_exception(ex);
            } catch (const SQLiteException&) {
                throw;
            } catch (const std::exception& e) {
                throw SQLiteException(e.what());
            } catch (...) {
                throw SQLiteException(message);
            }
        }

        /// \brief Executes an operation inside a transaction.
        /// \param operation The operation to execute.
        /// \param mode The transaction mode.
        /// \throws sqlite_exception if an error occurs during execution.
        template<typename Func>
        void transaction(Func operation, TransactionMode mode = TransactionMode::DEFERRED) {
            try {
                m_stmt_begin[static_cast<size_t>(mode)].execute(m_sqlite_ptr);
                operation();
                m_stmt_commit.execute(m_sqlite_ptr);
            } catch (...) {
                m_stmt_rollback.execute(m_sqlite_ptr);
                throw;
            }
        }

    private:
        sqlite3* m_sqlite_ptr = nullptr;        ///< Pointer to the SQLite database connection.
        mutable std::mutex m_sqlite_mutex;      ///< Mutex for thread-safe access.
        std::unique_ptr<SQLiteConfig> m_config; ///< Database configuration object.

        std::array<SQLiteStatement, 3> m_stmt_begin;    ///< Prepared statements for BEGIN TRANSACTION.
        SQLiteStatement                m_stmt_commit;   ///< Prepared statement for COMMIT.
        SQLiteStatement                m_stmt_rollback; ///< Prepared statement for ROLLBACK.

        /// \brief Creates necessary directories for the database file.
        /// Ensures the parent directory of the database file exists.
        /// \throws SQLiteException if directories cannot be created.
        void create_directories() {
            fs::path file_path(m_config->db_path);
            fs::path parent_dir = file_path.parent_path();
            if (parent_dir.empty()) return;
            if (!fs::exists(parent_dir)) {
                if (!fs::create_directories(parent_dir)) {
                    throw SQLiteException("Failed to create directories for path: " + parent_dir.string());
                }
            }
        }

        /// \brief Opens the SQLite database.
        /// Configures and opens the SQLite database based on the configuration settings.
        /// \throws SQLiteException if the database cannot be opened.
        void open_db() {
            int flags = 0;
            flags |= m_config->read_only ? SQLITE_OPEN_READONLY : (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
            flags |= m_config->use_uri ? SQLITE_OPEN_URI : 0;
            flags |= m_config->in_memory ? SQLITE_OPEN_MEMORY : 0;
            flags |= SQLITE_OPEN_FULLMUTEX;
            int err = 0;
            const char* db_name = m_config->in_memory ? ":memory:" : m_config->db_path.c_str();
            if ((err = sqlite3_open_v2(db_name, &m_sqlite_ptr, flags, nullptr)) != SQLITE_OK) {
                std::string error_message = "Cannot open database: ";
                error_message += sqlite3_errmsg(m_sqlite_db);
                error_message += " (Error code: ";
                error_message += std::to_string(err);
                error_message += ")";
                throw SQLiteException(error_message);
            }
        }

        /// \brief Configures the SQLite database.
        /// Applies various SQLite PRAGMA settings based on the configuration.
        /// \throws SQLiteException if any configuration step fails.
        void configure_db() {
            execute(m_sqlite_ptr, "PRAGMA busy_timeout = " + std::to_string(m_config->busy_timeout) + ";");
            execute(m_sqlite_ptr, "PRAGMA page_size = " + std::to_string(m_config->page_size) + ";");
            execute(m_sqlite_ptr, "PRAGMA cache_size = " + std::to_string(m_config->cache_size) + ";");
            execute(m_sqlite_ptr, "PRAGMA analysis_limit = " + std::to_string(m_config->analysis_limit) + ";");
            execute(m_sqlite_ptr, "PRAGMA wal_autocheckpoint = " + std::to_string(m_config->wal_autocheckpoint) + ";");
            execute(m_sqlite_ptr, "PRAGMA journal_mode = " + to_string(m_config->journal_mode) + ";");
            execute(m_sqlite_ptr, "PRAGMA synchronous = " + to_string(m_config->synchronous) + ";");
            execute(m_sqlite_ptr, "PRAGMA locking_mode = " + to_string(m_config->locking_mode) + ";");
            execute(m_sqlite_ptr, "PRAGMA auto_vacuum = " + to_string(m_config->auto_vacuum_mode) + ";");

            for (size_t i = 0; i < m_stmt_begin.size(); ++i) {
                m_stmt_begin[i].init(m_sqlite_ptr, "BEGIN " + to_string(static_cast<TransactionMode>(i)) + " TRANSACTION");
            }
            m_stmt_commit.init(m_sqlite_ptr, "COMMIT");
            m_stmt_rollback.init(m_sqlite_ptr, "ROLLBACK");

            if (m_config->user_version > 0) {
                execute(m_sqlite_ptr, "PRAGMA user_version = " + std::to_string(m_config->user_version) + ";");
            }
        }
    }

} // namespace dfh::storage::sqlite3

#endif // _DFH_SQLITE_CONNECTION_HPP_INCLUDED
