#pragma once
#ifndef _DFH_STORAGE_MDBX_CONFIG_HPP_INCLUDED
#define _DFH_STORAGE_MDBX_CONFIG_HPP_INCLUDED

/// \file MDBXConfig.hpp
/// \brief Configuration class for MDBX database.

namespace dfh::storage::mdbx {

    /// \class MDBXConfig
    /// \brief Configuration for MDBX databases.
    class MDBXConfig final : public IConfig {
    public:
        std::string pathname;                   ///< Pathname for the database or directory in which the database files reside.
        int64_t size_lower  = -1;               ///< Lower bound for database size.
        int64_t size_now    = -1;               ///< Current size of the database.
        int64_t size_upper  = -1;               ///< Upper bound for database size.
        int64_t growth_step = 16 * 1024 * 1024; ///< Step size for database growth.
        int64_t shrink_threshold = 16 * 1024 * 1024; ///< Threshold for database shrinking.
        int64_t page_size   = 0;                ///< Page size; should be a power of two.
        int64_t max_readers = 0;                ///< Maximum number of reader slots; 0 uses the default (twice the number of CPU cores).
        bool read_only = false;                 ///< Enables or disables read-only mode.
        bool readahead = false;                 ///< Enables or disables readahead for sequential data access.
        bool use_writemap = false;              ///< Enables or disables the `MDBX_WRITEMAP` mode, which maps the database into memory for direct modification.

        /// \brief Validate the MDBX configuration.
        /// \return True if the configuration is valid, false otherwise.
        bool validate() const override {
            const bool page_ok = (page_size == 0) || ((page_size & (page_size - 1)) == 0);
            const bool size_ok = (size_lower <= size_now || size_now == -1) &&
                                 (size_now <= size_upper || size_now == -1);
            return !pathname.empty() && page_ok && size_ok;
        }

        /// \brief Set a configuration option by key.
        /// \param key The name of the configuration option.
        /// \param value The value to set for the option.
        void set_option(const std::string& key, const std::string& value) override {
            if (key == "pathname") {
                pathname = value;
            } else
            if (key == "read_only") {
                read_only = (value == "true");
            } else
            if (key == "readahead") {
                readahead = (value == "true");
            } else
            if (key == "use_writemap") {
                use_writemap = (value == "true");
            } else {
                try {
                    int64_t num_value = std::stoll(value);
                    if (key == "size_lower") size_lower = num_value;
                    else if (key == "size_now") size_now = num_value;
                    else if (key == "size_upper") size_upper = num_value;
                    else if (key == "growth_step") growth_step = num_value;
                    else if (key == "shrink_threshold") shrink_threshold = num_value;
                    else if (key == "page_size") page_size = num_value;
                    else throw std::invalid_argument("Unknown key: " + key);
                } catch (const std::exception& e) {
                    throw std::invalid_argument("Invalid value for key: " + key);
                }
            }
        }

        /// \brief Get a configuration option by key.
        /// \param key The name of the configuration option.
        /// \return The value of the configuration option.
        std::string get_option(const std::string& key) const override {
            if (key == "pathname") return pathname;
            if (key == "read_only") return read_only ? "true" : "false";
            if (key == "readahead") return readahead ? "true" : "false";
            if (key == "use_writemap") return use_writemap ? "true" : "false";
            if (key == "size_lower") return std::to_string(size_lower);
            if (key == "size_now") return std::to_string(size_now);
            if (key == "size_upper") return std::to_string(size_upper);
            if (key == "growth_step") return std::to_string(growth_step);
            if (key == "shrink_threshold") return std::to_string(shrink_threshold);
            if (key == "page_size") return std::to_string(page_size);
            return std::string();
        }
    };

}; // namespace dfh::storage::mdbx

#endif // _DFH_STORAGE_MDBX_CONFIG_HPP_INCLUDED
