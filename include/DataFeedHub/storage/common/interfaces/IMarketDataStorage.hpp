#pragma once
#ifndef _DFH_STORAGE_IMARKET_DATA_STORAGE_HPP_INCLUDED
#define _DFH_STORAGE_IMARKET_DATA_STORAGE_HPP_INCLUDED

/// \file IMarketDataStorage.hpp
/// \brief Declares the interface for unified market data storage backends.

namespace dfh::storage {

    /// \class IMarketDataStorage
    /// \brief Interface for unified storage of market-related data including bars, ticks, and metrics.
    ///
    /// This interface provides access to core operations for reading, writing, and managing
    /// various types of market data, such as time series bars, tick data, and market-related metrics.
    /// It is designed to work across different storage backends in a unified way.
    class IMarketDataStorage {
    public:

        /// \brief Virtual destructor for safe polymorphic usage.
        virtual ~IMarketDataStorage() = default;

        //--- Connection and lifecycle management ---

        /// \brief Configures the backend connection using the given configuration.
        /// \param config Unique pointer to the backend configuration object.
        virtual void configure(ConfigPtr config) = 0;

        /// \brief Establishes a connection to the underlying storage backend.
        /// \throws StorageException If the connection fails.
        virtual void connect() = 0;

        /// \brief Closes the connection to the storage backend and releases associated resources.
        /// \throws StorageException If the disconnection fails.
        virtual void disconnect() = 0;

        /// \brief Checks whether the storage backend is currently connected.
        /// \return True if connected, false otherwise.
        virtual bool is_connected() const = 0;

        /// \brief Starts the backend (e.g. initializes schema or opens tables).
        /// \param txn Active transaction used for startup operations.
        virtual void start(const TransactionPtr& txn) = 0;

        /// \brief Stops the backend and finalizes any ongoing work.
        /// \param txn Active transaction used for shutdown operations.
        virtual void stop(const TransactionPtr& txn) = 0;

        /// \brief Creates a new transaction for this storage backend.
        /// \param mode Transaction mode (read-only or writable).
        /// \return A unique pointer to the created transaction.
        virtual TransactionPtr create_transaction(TransactionMode mode) = 0;

        /// \brief Prepares the backend for a new transaction (e.g., clearing state, resetting caches).
        /// \param txn Active transaction.
        virtual void before_transaction(const TransactionPtr& txn) = 0;

        /// \brief Finalizes the backend after a successful transaction commit (e.g., flushing state).
        /// \param txn Active transaction.
        virtual void after_transaction(const TransactionPtr& txn) = 0;

        //--- Metadata operations ---

        /// \brief Adds or extends the backend’s metadata.
        /// \param txn Active transaction.
        /// \param metadata Metadata to merge with existing metadata.
        virtual void extend_metadata(const TransactionPtr& txn, const StorageMetadata& metadata) = 0;

        /// \brief Erases data defined by the specified metadata from the backend.
        /// \param txn Active transaction.
        /// \param metadata Metadata describing the data to be removed.
        virtual void erase_data(const TransactionPtr& txn, const StorageMetadata& metadata) = 0;

        //--- Data insertion and update ---

        /// \brief Prepares internal structures for updating bar metadata.
        /// \param txn Active transaction.
        virtual void prepare_bar_metadata(const TransactionPtr& txn) = 0;

        /// \brief Inserts or updates bar data using individual market identifiers.
        /// \param txn Active transaction.
        /// \param market_type Market type of the data.
        /// \param exchange_id Exchange identifier.
        /// \param symbol_id Symbol identifier.
        /// \param bars List of bars to insert or update.
        /// \param config Encoding configuration for bar storage.
        virtual void upsert(
                const TransactionPtr& txn,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                const std::vector<dfh::MarketBar>& bars,
                const dfh::BarCodecConfig& config) = 0;

        //--- Data fetch ---

        /// \brief Retrieves the metadata describing stored data in the backend.
        /// \param txn Active transaction used for the metadata query.
        /// \param out_metadata Output metadata object to be populated.
        /// \return True if metadata is available and successfully retrieved, false otherwise.
        /// \throws StorageException If the operation fails due to backend issues.
        virtual bool fetch(
                const TransactionPtr& txn,
                StorageMetadata& out_metadata) = 0;

        /// \brief Retrieves bar metadata by symbol components.
        /// \param txn Active transaction.
        /// \param market_type Market type.
        /// \param exchange_id Exchange ID.
        /// \param symbol_id Symbol ID.
        /// \param time_frame Target time frame.
        /// \param out_metadata Output metadata object.
        /// \return True if metadata is found, false otherwise.
        virtual bool fetch(
                const TransactionPtr& txn,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                dfh::TimeFrame time_frame,
                dfh::BarMetadata& out_metadata) = 0;

        /// \brief Retrieves a segment of bar data by market identifiers.
        /// \param txn Active transaction.
        /// \param market_type Market type (e.g., SPOT, FUTURES).
        /// \param exchange_id Exchange identifier.
        /// \param symbol_id Symbol identifier.
        /// \param time_frame Target time frame (e.g., M1, M5).
        /// \param segment_key Starting timestamp of the segment (aligned to time_frame).
        /// \param out_bars Output vector to store retrieved bars.
        /// \param out_configs Output structure to receive codec configuration used for the data.
        /// \return True if the segment exists and data is retrieved, false otherwise.
        virtual bool fetch(
                const TransactionPtr& txn,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                dfh::TimeFrame time_frame,
                uint64_t segment_key,
                std::vector<dfh::MarketBar>& out_bars,
                dfh::BarCodecConfig& out_configs) = 0;

        //--- Data deletion --

        /// \brief Erases a specific segment of bar data by market identifiers.
        /// \param txn Active transaction.
        /// \param market_type Market type.
        /// \param exchange_id Exchange identifier.
        /// \param symbol_id Symbol identifier.
        /// \param time_frame Target time frame.
        /// \param segment_key Starting timestamp of the segment to erase.
        virtual void erase(
                const TransactionPtr& txn,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                dfh::TimeFrame time_frame,
                uint64_t segment_key) = 0;

        /// \brief Erases all segments of bar data for the given market identifiers and time frame.
        /// \param txn Active transaction.
        /// \param market_type Market type.
        /// \param exchange_id Exchange identifier.
        /// \param symbol_id Symbol identifier.
        /// \param time_frame Target time frame.
        virtual void erase(
                const TransactionPtr& txn,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                dfh::TimeFrame time_frame) = 0;

        /// \brief Erases all segments of bar data for the time frame.
        /// \param txn Active transaction.
        /// \param time_frame Target time frame.
        virtual void erase(
                const TransactionPtr& txn,
                dfh::TimeFrame time_frame) = 0;

        /// \brief Erases all stored data in this backend.
        /// \param txn Active transaction.
        /// \warning This operation is destructive and cannot be undone.
        virtual void erase_all_data(const TransactionPtr& txn) = 0;

    }; // class IMarketDataStorage

    /// \typedef MarketDataStoragePtr
    /// \brief Alias for a unique pointer to IMarketDataStorage.
    ///
    /// Used for exclusive ownership of a storage backend instance,
    /// which may maintain internal buffers or mutable state.
    using MarketDataStoragePtr = std::unique_ptr<IMarketDataStorage>;
} // namespace dfh::storage

#endif // _DFH_STORAGE_IMARKET_DATA_STORAGE_HPP_INCLUDED
