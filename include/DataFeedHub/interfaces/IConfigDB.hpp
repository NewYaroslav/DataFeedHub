#pragma once
#ifndef _DFH_ICONFIGDB_HPP_INCLUDED
#define _DFH_ICONFIGDB_HPP_INCLUDED

/// \file IConfigDB.hpp
/// \brief

namespace dfh {

    /// \class IConfigDB
    /// \brief
    class IConfigDB {
    public:

        /// \brief Set a configuration option by key.
        /// \param key Name of the configuration option.
        /// \param value Value to set for the option.
        virtual void set_option(const std::string& key, const std::string& value) = 0;

        /// \brief Get a configuration option by key.
        /// \param key Name of the configuration option.
        /// \return Value of the configuration option.
        virtual std::string get_option(const std::string& key) const = 0;

        /// \brief Validate the configuration.
        /// \return True if the configuration is valid, false otherwise.
        virtual bool validate() const = 0;

        /// \brief Virtual destructor for the interface.
        virtual ~IConfigDB() = default;

    }; // class IConnectionDB

} // namespace dfh

#endif // _DFH_ICONFIGDB_HPP_INCLUDED
