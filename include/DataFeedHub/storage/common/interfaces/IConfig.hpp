#pragma once
#ifndef _DFH_STORAGE_ICONFIG_HPP_INCLUDED
#define _DFH_STORAGE_ICONFIG_HPP_INCLUDED

/// \file IConfig.hpp
/// \brief Declares an interface for configuring database connections.

namespace dfh::storage {

    /// \class IConfig
    /// \brief Interface for database configuration objects.
    ///
    /// Provides a generic mechanism to set, retrieve, and validate configuration
    /// options for various database backends. Implementations are backend-specific
    /// and define how configuration options are applied.
    class IConfig {
    public:

        /// \brief Virtual destructor for safe polymorphic usage.
        virtual ~IConfig() = default;

        /// \brief Sets a configuration option.
        /// \param key Name of the configuration option.
        /// \param value Value to assign to the option.
        virtual void set_option(const std::string& key, const std::string& value) = 0;

        /// \brief Gets the value of a configuration option.
        /// \param key Name of the configuration option.
        /// \return Value currently assigned to the option.
        virtual std::string get_option(const std::string& key) const = 0;

        /// \brief Validates the current configuration.
        /// \return True if the configuration is valid and complete, false otherwise.
        virtual bool validate() const = 0;

    }; // class IConfig

    /// \typedef ConfigPtr
    /// \brief Alias for a unique pointer to an IConfig.
    ///
    /// Used for managing ownership of database configuration instances.
    using ConfigPtr = std::unique_ptr<IConfig>;

} // namespace dfh::storage

#endif // _DFH_STORAGE_ICONFIG_HPP_INCLUDED
