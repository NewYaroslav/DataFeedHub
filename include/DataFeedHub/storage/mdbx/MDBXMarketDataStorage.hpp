#pragma once
#ifndef _DFH_STORAGE_MDBX_STORAGE_HPP_INCLUDED
#define _DFH_STORAGE_MDBX_STORAGE_HPP_INCLUDED

/// \file MDBXMarketDataStorage.hpp
/// \brief MDBX implementation of IMarketDataStorage interface.

#include "MDBXStorage/MetadataBD.hpp"
#include "MDBXStorage/BarBD.hpp"

namespace dfh::storage::mdbx {

    /// \class MDBXMarketDataStorage
    /// \brief Provides MDBX-based implementation of the IMarketDataStorage interface.
    class MDBXMarketDataStorage final : public dfh::storage::IMarketDataStorage {
    public:

        /// \brief Constructs the market data storage.
        /// \param config Unique pointer to the MDBX configuration.
        /// \throws MDBXException if connection or configuration fails.
        explicit MDBXMarketDataStorage(ConfigPtr config)
            : m_connection(std::make_shared<MDBXConnection>(std::move(config))),
              m_metadata_db(m_connection.get()),
              m_bar_db(m_connection.get()) {
        }

        /// \brief Constructs storage using a shared MDBX connection.
        /// \param connection Shared pointer to an active MDBX connection.
        explicit MDBXMarketDataStorage(std::shared_ptr<MDBXConnection> connection)
            : m_connection(std::move(connection)),
              m_metadata_db(m_connection.get()),
              m_bar_db(m_connection.get()) {
        }

        /// \brief Destructor that attempts to stop the backend.
        virtual ~MDBXMarketDataStorage() {
            try {
                if (!m_connection->is_connected()) return;
                m_metadata_db.stop();
                m_bar_db.stop();
            } catch(...) {};
        }

        //--- Connection and lifecycle management ---

        /// \copydoc IMarketDataStorage::configure
		void configure(ConfigPtr config) override final {
            m_connection->configure(std::move(config));
		}

        /// \copydoc IMarketDataStorage::connect
        void connect() override final {
            m_connection->connect();
        }

        /// \copydoc IMarketDataStorage::disconnect
        void disconnect() override final {
            m_connection->disconnect();
        }

        /// \copydoc IMarketDataStorage::is_connected
        bool is_connected() const override final {
            return m_connection->is_connected();
        }

        /// \copydoc IMarketDataStorage::start
        void start(const TransactionPtr& txn) override final {
            if (!m_connection->is_connected()) throw MDBXException("Connection is not established");
            MDBXTransaction* txn_ptr = dynamic_cast<MDBXTransaction*>(txn.get());
            if (!txn_ptr) throw MDBXException("Invalid transaction type");
            m_metadata_db.start(txn_ptr);
            m_bar_db.start(txn_ptr);
        }

        /// \copydoc IMarketDataStorage::stop
        void stop(const TransactionPtr& txn) override final {
            if (!m_connection->is_connected()) throw MDBXException("Connection is not established");
            m_metadata_db.stop();
            m_bar_db.stop();
        }

        /// \copydoc IMarketDataStorage::create_transaction
        TransactionPtr create_transaction(TransactionMode mode) override final {
            return std::make_unique<MDBXTransaction>(m_connection, mode);
		}

        /// \copydoc IMarketDataStorage::before_transaction
        void before_transaction(const TransactionPtr& txn) override final {}

        /// \copydoc IMarketDataStorage::after_transaction
        void after_transaction(const TransactionPtr& txn) override final {
            MDBXTransaction* txn_ptr = dynamic_cast<MDBXTransaction*>(txn.get());
            m_bar_db.after_transaction(txn_ptr);
        }

        //--- Metadata operations ---

        /// \copydoc IMarketDataStorage::extend_metadata
        void extend_metadata(const TransactionPtr& txn, const StorageMetadata& metadata) override final {
            MDBXTransaction* txn_ptr = dynamic_cast<MDBXTransaction*>(txn.get());
            StorageMetadata current_metadata;
            m_metadata_db.fetch(txn_ptr, current_metadata);
            current_metadata.merge_with(metadata);
            m_metadata_db.upsert(txn_ptr, current_metadata);
        }

        /// \copydoc IMarketDataStorage::erase_data
        void erase_data(const TransactionPtr& txn, const StorageMetadata& metadata) override final {
            MDBXTransaction* txn_ptr = dynamic_cast<MDBXTransaction*>(txn.get());
            StorageMetadata current_metadata;
            m_metadata_db.fetch(txn_ptr, current_metadata);
            current_metadata.subtract(metadata);
            m_metadata_db.upsert(txn_ptr, current_metadata);
            m_bar_db.erase_data(txn_ptr, metadata);
        }

        //--- Data insertion and update ---

        /// \copydoc IMarketDataStorage::prepare_bar_metadata
        void prepare_bar_metadata(const TransactionPtr& txn) override final {
            m_bar_db.prepare_metadata(dynamic_cast<MDBXTransaction*>(txn.get()));
        }

        /// \copydoc IMarketDataStorage::upsert
        void upsert(
                const TransactionPtr& txn,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                const std::vector<dfh::MarketBar>& bars,
                const dfh::BarCodecConfig& config) override final {
            m_bar_db.upsert(dynamic_cast<MDBXTransaction*>(txn.get()), market_type, exchange_id, symbol_id, bars, config);
        }

        //--- Data fetch ---

         /// \copydoc IMarketDataStorage::fetch(const TransactionPtr&, StorageMetadata&)
        bool fetch(
                const TransactionPtr& txn,
                StorageMetadata& metadata) override final {
            return m_metadata_db.fetch(dynamic_cast<MDBXTransaction*>(txn.get()), metadata);
        }

        /// \copydoc IMarketDataStorage::fetch(const TransactionPtr&, MarketType, uint16_t, uint16_t, TimeFrame, BarMetadata&)
        bool fetch(
                const TransactionPtr& txn,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                dfh::TimeFrame time_frame,
                dfh::BarMetadata& metadata) override final {
            return m_bar_db.fetch(dynamic_cast<MDBXTransaction*>(txn.get()),
                market_type, exchange_id, symbol_id, time_frame, metadata);
        }

        /// \copydoc IMarketDataStorage::fetch(const TransactionPtr&, MarketType, uint16_t, uint16_t, TimeFrame, uint64_t, vector<MarketBar>&, BarCodecConfig&)
        bool fetch(
                const TransactionPtr& txn,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                dfh::TimeFrame time_frame,
                uint64_t segment_key,
                std::vector<dfh::MarketBar>& out_bars,
                dfh::BarCodecConfig& out_configs) override final {
            return m_bar_db.fetch(dynamic_cast<MDBXTransaction*>(txn.get()),
                market_type, exchange_id, symbol_id, time_frame,
                segment_key, out_bars, out_configs);
        }

        //--- Data deletion --

        /// \copydoc IMarketDataStorage::erase(const TransactionPtr&, MarketType, uint16_t, uint16_t, TimeFrame, uint64_t)
        void erase(
                const TransactionPtr& txn,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                dfh::TimeFrame time_frame,
                uint64_t segment_key) override final {
            m_bar_db.erase(dynamic_cast<MDBXTransaction*>(txn.get()),
                market_type, exchange_id, symbol_id, time_frame,
                segment_key);
        }

        /// \copydoc IMarketDataStorage::erase(const TransactionPtr&, MarketType, uint16_t, uint16_t, TimeFrame)
        void erase(
                const TransactionPtr& txn,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                dfh::TimeFrame time_frame) override final {
            m_bar_db.erase(dynamic_cast<MDBXTransaction*>(txn.get()),
                market_type, exchange_id, symbol_id, time_frame);
        }

        /// \copydoc IMarketDataStorage::erase(const TransactionPtr&, TimeFrame)
        void erase(
                const TransactionPtr& txn,
                dfh::TimeFrame time_frame) override final {
            m_bar_db.erase(dynamic_cast<MDBXTransaction*>(txn.get()), time_frame);
        }

        /// \copydoc IMarketDataStorage::erase_all_data
        void erase_all_data(const TransactionPtr& txn) override final {
            m_metadata_db.erase_all_data(dynamic_cast<MDBXTransaction*>(txn.get()));
            m_bar_db.erase_all_data(dynamic_cast<MDBXTransaction*>(txn.get()));
        }

	private:
        std::shared_ptr<MDBXConnection> m_connection;   ///< Shared pointer to the MDBX connection.
        MetadataBD m_metadata_db;                       ///< Interface to metadata database.
        BarBD      m_bar_db;                            ///< Interface to bar data database.
    };

}; // namespace dfh::storage::mdbx

#endif // _DFH_STORAGE_MDBX_STORAGE_HPP_INCLUDED
