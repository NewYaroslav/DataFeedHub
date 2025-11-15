#pragma once
#ifndef _DFH_STORAGE_MDBX_CONNECTION_HPP_INCLUDED
#define _DFH_STORAGE_MDBX_CONNECTION_HPP_INCLUDED

/// \file MDBXConnection.hpp
/// \brief Manages an MDBX database connection using a provided configuration.

#include "MDBXConnection/MDBXException.hpp"
#include "MDBXConnection/utils.hpp"

namespace dfh::storage::mdbx {

    /// \class MDBXConnection
    /// \brief Manages an MDBX database connection using a provided configuration.
    class MDBXConnection final : public dfh::storage::IConnection {
    public:

		/// \brief Default constructor.
		MDBXConnection() = default;

        /// \brief Constructs a connection using the given MDBX configuration.
        /// \param config A unique pointer to a derived MDBXConfig.
        /// \throws MDBXException if the config is null or not of type MDBXConfig.
        MDBXConnection(ConfigPtr config) {
            if (!config) throw MDBXException("MDBXConfig cannot be null.");
            IConfig* raw_base = config.release();
            auto* raw = dynamic_cast<MDBXConfig*>(raw_base);
            if (!raw) {
                delete raw_base; // очистка вручную, т.к. владение уже снято
                throw MDBXException("Config must be of type MDBXConfig");
            }
            m_config = std::unique_ptr<MDBXConfig>(raw);
        }

        /// \brief Destructor that ensures proper cleanup of resources.
        virtual ~MDBXConnection() {
			if (!m_env) return;
            mdbx_env_close(m_env);
            m_env = nullptr;
        }

        /// \brief Sets the configuration for this MDBX connection.
        /// \param config A unique pointer to a derived MDBXConfig.
        /// \throws MDBXException if the config is null or not of correct type.
        void configure(ConfigPtr config) override final {
            std::lock_guard<std::mutex> locker(m_mdbx_mutex);
			if (!config) throw MDBXException("MDBXConfig cannot be null.");
            IConfig* raw_base = config.release();
            auto* raw = dynamic_cast<MDBXConfig*>(raw_base);
            if (!raw) {
                delete raw_base; // очистка вручную, т.к. владение уже снято
                throw MDBXException("Config must be of type MDBXConfig");
            }
            m_config = std::unique_ptr<MDBXConfig>(raw);
        }

        /// \brief Establishes and configures the MDBX environment and opens a read-only transaction.
        /// \throws MDBXException if the connection fails or configuration is invalid.
        void connect() override final {
            std::lock_guard<std::mutex> locker(m_mdbx_mutex);
            if (m_env) throw MDBXException("Database connection already exists.");
            if (!m_config) throw MDBXException("No configuration provided.");
            try {
                create_directories();
                db_init();
            } catch (...) {
                if (m_env) {
					int rc = mdbx_env_close(m_env);
					if (rc != MDBX_SUCCESS) throw MDBXException("Failed to close environment: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)), rc);
				}
                m_env = nullptr;
                throw;
            }
        }

        /// \brief Closes the MDBX environment if it is open.
        /// \throws MDBXException if closing the environment fails.
        void disconnect() override final {
            std::lock_guard<std::mutex> locker(m_mdbx_mutex);
            if (m_env) {
				int rc = mdbx_env_close(m_env);
				if (rc != MDBX_SUCCESS) throw MDBXException("Failed to close environment: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)), rc);
			}
            m_env = nullptr;
        }

        /// \brief Checks if the database connection is active.
        /// \return True if the connection is active, false otherwise.
        bool is_connected() const override final {
            std::lock_guard<std::mutex> locker(m_mdbx_mutex);
            return (m_env != nullptr);
        }

		/// \brief Returns a pointer to the internally managed read-only transaction.
        /// \return Pointer to the MDBX read-only transaction.
		MDBX_txn *rdonly_handle() noexcept {
			return m_rdonly_txn;
		}

		/// \brief Returns the MDBX environment handle.
        /// \return The MDBX environment handle.
        MDBX_env* env_handle() noexcept {
            return m_env;
        }

    private:
		MDBX_env *m_env = nullptr;            ///< Pointer to the MDBX environment handle.
        MDBX_txn *m_rdonly_txn = nullptr;     ///< The MDBX transaction handle.
        mutable std::mutex m_mdbx_mutex;      ///< Mutex for thread-safe access.
        std::unique_ptr<MDBXConfig> m_config; ///< Database configuration object.

        /// \brief Creates necessary directories for the database file.
        /// \throws MDBXException if directories cannot be created.
        void create_directories() {
            namespace fs = std::filesystem;
#           ifdef _WIN32
            // Convert UTF-8 string to wide string for Windows
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::wstring wide_path = converter.from_bytes(m_config->pathname);
            std::filesystem::path file_path(wide_path);
#           else
            std::filesystem::path file_path(m_config->pathname);
#           endif
            fs::path parent_dir = file_path.parent_path();
            if (parent_dir.empty()) parent_dir = fs::current_path();
            if (!std::filesystem::exists(parent_dir)) {
                std::error_code ec;
                if (!std::filesystem::create_directories(parent_dir, ec)) {
                    throw MDBXException("Failed to create directories for path: " + m_config->pathname);
                }
            }
        }

		void db_init() {
			int rc = 0;
			rc = mdbx_env_create(&m_env);
			if (rc != MDBX_SUCCESS) throw MDBXException(
                "mdbx_env_create failed: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)), rc);

			rc = mdbx_env_set_geometry(
				m_env,
				m_config->size_lower,
				m_config->size_now,
				m_config->size_upper,
				m_config->growth_step,
				m_config->shrink_threshold,
				m_config->page_size);
			if (rc != MDBX_SUCCESS) throw MDBXException(
                "mdbx_env_set_geometry failed: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)), rc);

			const int max_dbs = 16;
			rc = mdbx_env_set_maxdbs(m_env, max_dbs);
			if (rc != MDBX_SUCCESS) throw MDBXException(
                "mdbx_env_set_maxdbs failed: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)), rc);

			if (m_config->max_readers) {
				rc = mdbx_env_set_maxreaders(m_env, m_config->max_readers);
			} else {
				rc = mdbx_env_set_maxreaders(m_env, std::thread::hardware_concurrency() * 2);
			}
			if (rc != MDBX_SUCCESS) throw MDBXException(
                "mdbx_env_set_maxreaders failed: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)), rc);

			MDBX_env_flags_t env_flags = MDBX_ACCEDE | MDBX_NOSUBDIR | MDBX_SYNC_DURABLE;
			if (m_config->read_only) env_flags |= MDBX_RDONLY;
			if (m_config->readahead) env_flags |= MDBX_NORDAHEAD;
			if (m_config->use_writemap) env_flags |= MDBX_WRITEMAP;

#           ifdef _WIN32
			// Convert UTF-8 string to wide string for Windows
            std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
            std::wstring wide_pathname = converter.from_bytes(m_config->pathname);
			rc = mdbx_env_openW(m_env, wide_pathname.c_str(), env_flags, 0664);
#           else
			rc = mdbx_env_open(m_env, m_config->pathname, env_flags, 0664);
#           endif
            std::cout << "--3" << std::endl;
			if (rc != MDBX_SUCCESS) throw MDBXException(
                "mdbx_env_open failed: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)), rc);

			rc = mdbx_txn_begin(m_env, nullptr, MDBX_TXN_RDONLY, &m_rdonly_txn);
			if (rc != MDBX_SUCCESS) throw MDBXException(
                "mdbx_txn_begin failed: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)), rc);

            rc = mdbx_txn_reset(m_rdonly_txn);
            if (rc != MDBX_SUCCESS) throw MDBXException(
                "mdbx_txn_reset failed: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)), rc);
		}

    }; // MDBXConnection

} // namespace dfh::storage::mdbx

#endif // _DFH_STORAGE_MDBX_CONNECTION_HPP_INCLUDED
