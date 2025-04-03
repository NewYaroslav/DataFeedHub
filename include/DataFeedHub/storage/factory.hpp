#pragma once
#ifndef _DFH_STORAGE_FACTORY_HPP_INCLUDED
#define _DFH_STORAGE_FACTORY_HPP_INCLUDED

/// \file factory.hpp
/// \brief Factory methods for creating storage-related objects.

namespace dfh::storage {

    /// \brief Creates configuration object for specified storage backend.
    /// \param backend Storage backend type.
    /// \return Unique pointer to IConfigDB implementation.
    inline ConfigPtr create_config(StorageBackend backend) {
        switch (backend) {
        case StorageBackend::MDBX:
            return std::make_unique<dfh::storage::mdbx::MDBXConfig>();
        default:
            throw std::invalid_argument("create_config: unsupported storage backend");
        }
    }

    inline ConfigPtr create_config(const dfh::storage::mdbx::MDBXConfig& source) {
        return std::make_unique<dfh::storage::mdbx::MDBXConfig>(source);
    }

    inline ConfigPtr create_config(dfh::storage::mdbx::MDBXConfig&& source) {
        return std::make_unique<dfh::storage::mdbx::MDBXConfig>(std::move(source));
    }

//------------------------------------------------------------------------------

    /// \brief Creates connection object for specified storage backend.
    /// \param backend Storage backend type.
    /// \return Shared pointer to IConnectionDB implementation.
    ConnectionPtr create_connection(StorageBackend backend) {
        switch (backend) {
        case StorageBackend::MDBX:
            return std::make_shared<dfh::storage::mdbx::MDBXConnection>();
        default:
            throw std::invalid_argument("create_connection: unsupported storage backend");
        }
    }

    /// \brief Creates connection object from config.
    /// \param config Unique pointer to config.
    /// \return Shared pointer to IConnectionDB.
    /// \throws std::invalid_argument if config type is unsupported.
    ConnectionPtr create_connection(ConfigPtr config) {
        auto* raw = dynamic_cast<mdbx::MDBXConfig*>(config.get());
        if (!raw) {
            config.reset();
            throw std::invalid_argument("create_connection: unsupported config type");
        }
        return std::make_shared<dfh::storage::mdbx::MDBXConnection>(std::move(config));
    }

    inline ConnectionPtr create_connection(const dfh::storage::mdbx::MDBXConfig& config) {
        return std::make_shared<dfh::storage::mdbx::MDBXConnection>(std::make_unique<dfh::storage::mdbx::MDBXConfig>(config));
    }

    inline ConnectionPtr create_connection(dfh::storage::mdbx::MDBXConfig&& config) {
        return std::make_shared<dfh::storage::mdbx::MDBXConnection>(std::make_unique<dfh::storage::mdbx::MDBXConfig>(std::move(config)));
    }

//------------------------------------------------------------------------------

    /// \brief Creates storage from config.
    /// \param config Unique pointer to config.
    /// \return Shared pointer to IMarketDataStorage.
    /// \throws std::invalid_argument if config type is unsupported.
    MarketDataStoragePtr create_storage(ConfigPtr config) {
        auto* raw = dynamic_cast<dfh::storage::mdbx::MDBXConfig*>(config.get());
        if (!raw) {
            config.reset();
            throw std::invalid_argument("create_storage(config): unsupported config type");
        }
        return std::make_unique<dfh::storage::mdbx::MDBXMarketDataStorage>(std::move(config));
    }

    /// \brief Creates storage from connection.
    /// \param connection Shared pointer to connection.
    /// \return Shared pointer to IMarketDataStorage.
    /// \throws std::invalid_argument if connection type is unsupported.
    MarketDataStoragePtr create_storage(ConnectionPtr connection) {
        auto derived_ptr = std::dynamic_pointer_cast<dfh::storage::mdbx::MDBXConnection>(connection);
        if (!derived_ptr) {
            throw std::invalid_argument("create_storage(connection): unsupported connection type");
        }
        return std::make_unique<dfh::storage::mdbx::MDBXMarketDataStorage>(derived_ptr);
    }

    inline MarketDataStoragePtr create_storage(const dfh::storage::mdbx::MDBXConfig& config) {
        return std::make_unique<dfh::storage::mdbx::MDBXMarketDataStorage>(std::make_unique<dfh::storage::mdbx::MDBXConfig>(config));
    }

    inline MarketDataStoragePtr create_storage(dfh::storage::mdbx::MDBXConfig&& config) {
        return std::make_unique<dfh::storage::mdbx::MDBXMarketDataStorage>(std::make_unique<dfh::storage::mdbx::MDBXConfig>(std::move(config)));
    }
}

#endif // _DFH_STORAGE_FACTORY_HPP_INCLUDED
