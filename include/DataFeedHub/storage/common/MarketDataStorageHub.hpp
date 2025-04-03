#pragma once
#ifndef _DFH_STORAGE_MARKET_DATA_STORAGE_HUB_HPP_INCLUDED
#define _DFH_STORAGE_MARKET_DATA_STORAGE_HUB_HPP_INCLUDED

/// \file MarketDataStorageHub.hpp
/// \brief Declares a composite storage manager for handling multiple market data storage backends.

namespace dfh::storage {

    /// \class MarketDataStorageHub
    /// \brief High-level manager for multiple IMarketDataStorage backends.
    ///
    /// Provides a unified interface for routing read/write operations across
    /// multiple storage backends based on metadata and availability.
    class MarketDataStorageHub final : public TransactionGuardAccess {
    public:

        MarketDataStorageHub() = default;

        MarketDataStorageHub(std::vector<MarketDataStoragePtr> storage_list)
            : m_storage_list(std::move(storage_list)) {
        }

        virtual ~MarketDataStorageHub() {
            if (!m_started) return;
            try {
                stop();
            } catch(...) {};
        }

        /// \brief Adds a new storage backend.
        /// \param storage The backend to add.
        /// \throws StorageException If storage has already been started.
        void add_storage(MarketDataStoragePtr storage) {
            if (m_started) throw StorageException("Cannot add storage after startup.");
            m_storage_list.push_back(std::move(storage));
        }

        /// \brief Creates a transaction guard across all storage backends.
        /// \param mode Transaction mode.
        /// \return Pointer to the created guard.
        /// \throws StorageException If the system is not started.
        TransactionGuardPtr transaction(TransactionMode mode) {
            if (!m_started) throw StorageException("MarketDataStorageHub: not started.");
            return create_guard(mode, m_storage_list);
        }

        /// \brief Starts all configured storage backends.
        /// \throws StorageException If startup fails.
        void start() {
            if (m_started) return;
            for (const auto& storage : m_storage_list) {
                if (storage->is_connected()) continue;
                storage->connect();
            }
            auto guard = transaction(TransactionMode::WRITABLE);

            guard->begin();
            for (size_t db_index = 0; db_index < m_storage_list.size(); ++db_index) {
                m_storage_list[db_index]->start(get_transaction(guard.get(), db_index));
            }
            refresh_metadata(guard);
            guard->commit();

            m_started = true;
        }

        /// \brief Stops all active storage backends.
        /// \throws StorageException If shutdown fails.
        void stop() {
            if (!m_started) return;
            auto guard = transaction(TransactionMode::WRITABLE);

            guard->begin();
            for (size_t db_index = 0; db_index < m_storage_list.size(); ++db_index) {
                m_storage_list[db_index]->stop(get_transaction(guard.get(), db_index));
            }
            guard->commit();

            for (const auto& storage : m_storage_list) {
                if (!storage->is_connected()) continue;
                storage->disconnect();
            }
            m_started = false;
        }

        /// \brief Checks if all backends are connected.
        /// \return True if all are connected.
        bool is_connected() const {
            for (const auto& storage : m_storage_list) {
                if (!storage->is_connected()) return false;
            }
            return true;
        }

        /// \brief Returns the cached storage metadata for each backend.
        /// \return Storage metadata for each backend.
        const std::vector<StorageMetadata>& metadata() const {
            return m_storage_metadata;
        }

        //--- Metadata operations ---

        /// \brief Adds or extends metadata for the specified storage backend.
        /// \param guard Active transaction guard managing backend transactions.
        /// \param db_index Index of the target storage backend.
        /// \param metadata Metadata to merge with the existing metadata.
        void extend_metadata(const TransactionGuardPtr &guard, size_t db_index, const StorageMetadata& metadata) {
            if (db_index >= m_storage_list.size()) throw StorageException("MarketDataStorageHub: invalid storage backend index in extend_metadata");
            m_storage_list[db_index]->extend_metadata(get_transaction(guard.get(), db_index), metadata);
        }

        /// \brief Erases data corresponding to the specified metadata from the storage backend.
        /// \param guard Active transaction guard managing backend transactions.
        /// \param db_index Index of the target storage backend.
        /// \param metadata Metadata describing the data to be removed.
        void erase_data(const TransactionGuardPtr &guard, size_t db_index, const StorageMetadata& metadata) {
            if (db_index >= m_storage_list.size()) throw StorageException("MarketDataStorageHub: invalid storage backend index in erase_data");
            m_storage_list[db_index]->erase_data(get_transaction(guard.get(), db_index), metadata);
        }

        //--- Data fetch ---

        /// \brief Retrieves metadata for each storage backend.
        /// \param guard Active transaction guard managing backend transactions.
        /// \param metadata_list Output vector to receive metadata for each backend.
        /// \return True if metadata was successfully retrieved for all backends, false otherwise.
        /// \throws StorageException If the operation fails during metadata fetch.
        bool fetch(
                const TransactionGuardPtr &guard,
                std::vector<StorageMetadata>& metadata_list) {
            if (!refresh_metadata(guard)) return false;
            metadata_list = m_storage_metadata;
            return true;
        }

        /// \brief Retrieves bar metadata for the specified market/symbol/timeframe across all backends.
        /// \param guard Active transaction guard.
        /// \param market_type Market type (e.g., SPOT, FUTURES).
        /// \param exchange_id Exchange identifier.
        /// \param symbol_id Symbol identifier.
        /// \param time_frame Timeframe for which metadata is requested.
        /// \param metadata_list Output vector containing metadata per backend.
        /// \return True if metadata was retrieved from all backends, false otherwise.
        bool fetch(
                const TransactionGuardPtr &guard,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                dfh::TimeFrame time_frame,
                std::vector<dfh::BarMetadata>& metadata_list) {
            bool success = true;
            metadata_list.resize(m_storage_list.size());
            for (size_t db_index = 0; db_index < m_storage_list.size(); ++db_index) {
                if (!m_storage_list[db_index]->fetch(get_transaction(guard.get(), db_index), market_type, exchange_id, symbol_id, time_frame, metadata_list[db_index])) {
                    success = false;
                }
            }
            return success;
        }

        /// \brief Retrieves bar metadata using the symbol key across all backends.
        /// \param guard Active transaction guard.
        /// \param symbol_key Unique 32-bit symbol key.
        /// \param time_frame Timeframe for which metadata is requested.
        /// \param metadata_list Output vector containing metadata per backend.
        /// \return True if metadata was retrieved from all backends, false otherwise.
        bool fetch(
                const TransactionGuardPtr &guard,
                uint32_t symbol_key,
                dfh::TimeFrame time_frame,
                std::vector<dfh::BarMetadata>& metadata_list) {
            dfh::MarketType market_type;
            uint16_t exchange_id, symbol_id;
            dfh::extract_symbol_key32(symbol_key, market_type, exchange_id, symbol_id);
            return fetch(guard, market_type, exchange_id, symbol_id, time_frame, metadata_list);
        }

        /// \brief Retrieves a range of bar segments by market identifiers and time range.
        /// \param guard Active transaction guard.
        /// \param market_type Market type (e.g., SPOT, FUTURES).
        /// \param exchange_id Exchange identifier.
        /// \param symbol_id Symbol identifier.
        /// \param time_frame Target time frame (e.g., M1, H1).
        /// \param start_time_ms Start of desired data range (inclusive).
        /// \param end_time_ms End of desired data range (exclusive).
        /// \param bars Output vector to hold the retrieved bar data.
        /// \param config Output structure to receive the codec configuration.
        /// \return True if at least one segment was successfully retrieved, false otherwise.
        bool fetch(
                const TransactionGuardPtr &guard,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                dfh::TimeFrame time_frame,
                uint64_t start_time_ms,
                uint64_t end_time_ms,
                std::vector<dfh::MarketBar>& bars,
                dfh::BarCodecConfig& config) {
            const uint64_t duration_ms = dfh::get_segment_duration_ms(time_frame);
            const uint64_t segment_start = start_time_ms / duration_ms;
            const uint64_t segment_stop  = (end_time_ms - 1) / duration_ms;

            bool success = false;
            for (uint64_t segment = segment_start; segment <= segment_stop; ++segment) {
                const size_t db_index = find_storage_index(
                        StorageDataFlags::BARS,
                        market_type,
                        exchange_id,
                        symbol_id,
                        segment * duration_ms);

                success |= m_storage_list[db_index]->fetch(
                    get_transaction(guard.get(), db_index),
                    market_type,
                    exchange_id,
                    symbol_id,
                    time_frame,
                    segment,
                    bars,
                    config);
            }

            if (start_time_ms > (segment_start * duration_ms)) {
                dfh::transform::crop_bars_before(bars, start_time_ms);
            }
            if (end_time_ms < ((segment_stop * duration_ms) + duration_ms)) {
                dfh::transform::crop_bars_after(bars, end_time_ms);
            }
            return success;
        }

        /// \brief Retrieves a range of bar segments by symbol key and time range.
        /// \param guard Active transaction guard.
        /// \param symbol_key Unique 32-bit key representing market/exchange/symbol.
        /// \param time_frame Target time frame.
        /// \param start_time_ms Start of desired data range (inclusive).
        /// \param end_time_ms End of desired data range (exclusive).
        /// \param bars Output vector to hold the retrieved bar data.
        /// \param config Output structure to receive the codec configuration.
        /// \return True if at least one segment was successfully retrieved, false otherwise.
        bool fetch(
                const TransactionGuardPtr &guard,
                uint32_t symbol_key,
                dfh::TimeFrame time_frame,
                uint64_t start_time_ms,
                uint64_t end_time_ms,
                std::vector<dfh::MarketBar>& bars,
                dfh::BarCodecConfig& config) {
            dfh::MarketType market_type;
            uint16_t exchange_id, symbol_id;
            dfh::extract_symbol_key32(symbol_key, market_type, exchange_id, symbol_id);
            return fetch(guard, market_type, exchange_id, symbol_id, time_frame, start_time_ms, end_time_ms, bars, config);
        }

        //--- Data insertion and update ---

        /// \brief Prepares internal metadata structures for bar data processing.
        /// \param guard Transaction guard managing active transactions for all backends.
        void prepare_bar_metadata(const TransactionGuardPtr &guard) {
            for (size_t db_index = 0; db_index < m_storage_list.size(); ++db_index) {
                m_storage_list[db_index]->prepare_bar_metadata(get_transaction(guard.get(), db_index));
            }
        }

        /// \brief Inserts or updates bar data in the appropriate backend using symbol components.
        /// \param guard Transaction guard managing active transactions for all backends.
        /// \param market_type Market type (e.g., SPOT, FUTURES).
        /// \param exchange_id Exchange identifier.
        /// \param symbol_id Symbol identifier.
        /// \param bars List of bars to insert or update.
        /// \param config Codec configuration that defines timeframe and encoding rules.
        /// \throws StorageException If bar data is unordered or no suitable backend is found.
        void upsert(
                const TransactionGuardPtr &guard,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                const std::vector<dfh::MarketBar>& bars,
                const dfh::BarCodecConfig& config) {
            std::vector<std::vector<dfh::MarketBar>> out_segments;
            if (!dfh::transform::split_bars(config.time_frame, bars, out_segments)) throw StorageException("Bars are not in correct order.");

            const uint64_t duration_ms = dfh::get_segment_duration_ms(config.time_frame);
            for (size_t i = 0; i < out_segments.size(); ++i) {
                const uint64_t segment_time_ms = time_shield::start_of_period(duration_ms, out_segments[i][0].time_ms);

                const size_t db_index = find_storage_index(
                    StorageDataFlags::BARS,
                    market_type,
                    exchange_id,
                    symbol_id,
                    segment_time_ms);

                m_storage_list[db_index]->upsert(
                    get_transaction(guard.get(), db_index),
                    market_type, exchange_id, symbol_id,
                    out_segments[i],
                    config);
            }
        }

        /// \brief Inserts or updates bar data using a 32-bit symbol key.
        /// \param guard Transaction guard managing active transactions for all backends.
        /// \param symbol_key Encoded 32-bit key combining market type, exchange ID, and symbol ID.
        /// \param bars List of bars to insert or update.
        /// \param config Codec configuration for encoding and segmentation.
        /// \throws StorageException If bar data is unordered or no suitable backend is found.
        void upsert(
                const TransactionGuardPtr &guard,
                uint32_t symbol_key,
                const std::vector<dfh::MarketBar>& bars,
                const dfh::BarCodecConfig& config) {
            dfh::MarketType market_type;
            uint16_t exchange_id, symbol_id;
            dfh::extract_symbol_key32(symbol_key, market_type, exchange_id, symbol_id);
            upsert(guard, market_type, exchange_id, symbol_id, bars, config);
        }

        /// \brief Refreshes metadata from all registered backends.
        /// \param guard Active transaction guard.
        /// \return True if all metadata fetches succeeded; false otherwise.
        /// \throws StorageException On backend failure.
        bool refresh_metadata(const TransactionGuardPtr &guard) {
            bool success = true;
            m_storage_metadata.resize(m_storage_list.size());
            for (size_t db_index = 0; db_index < m_storage_list.size(); ++db_index) {
                if (!m_storage_list[db_index]->fetch(get_transaction(guard.get(), db_index), m_storage_metadata[db_index])) {
                    success = false;
                }
            }
            return success;
        }

        //--- Data deletion --

        /// \brief Erases bar segments within the given time range by market identifiers.
        /// \param guard Active transaction guard.
        /// \param market_type Market type (e.g., SPOT, FUTURES).
        /// \param exchange_id Exchange ID.
        /// \param symbol_id Symbol ID.
        /// \param time_frame Time frame (e.g., M1, H1).
        /// \param start_time_ms Start timestamp (inclusive).
        /// \param end_time_ms End timestamp (inclusive).
        void erase(
                const TransactionGuardPtr &guard,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                dfh::TimeFrame time_frame,
                uint64_t start_time_ms,
                uint64_t end_time_ms) {
            const uint64_t duration_ms = dfh::get_segment_duration_ms(time_frame);
            const uint64_t segment_start = start_time_ms / duration_ms;
            const uint64_t segment_stop  = (end_time_ms - 1) / duration_ms;

            for (uint64_t segment = segment_start; segment <= segment_stop; ++segment) {
                const size_t db_index = find_storage_index(
                        StorageDataFlags::BARS,
                        market_type,
                        exchange_id,
                        symbol_id,
                        segment * duration_ms);

                m_storage_list[db_index]->erase(
                    get_transaction(guard.get(), db_index),
                    market_type,
                    exchange_id,
                    symbol_id,
                    time_frame,
                    segment);
            }
        }

        /// \brief Erases bar segments within the given time range by symbol key.
        /// \param guard Active transaction guard.
        /// \param symbol_key 32-bit symbol key.
        /// \param time_frame Time frame.
        /// \param start_time_ms Start timestamp (inclusive).
        /// \param end_time_ms End timestamp (inclusive).
        void erase(
                const TransactionGuardPtr &guard,
                uint32_t symbol_key,
                dfh::TimeFrame time_frame,
                uint64_t start_time_ms,
                uint64_t end_time_ms) {
            dfh::MarketType market_type;
            uint16_t exchange_id, symbol_id;
            dfh::extract_symbol_key32(symbol_key, market_type, exchange_id, symbol_id);
            erase(guard, market_type, exchange_id, symbol_id, time_frame, start_time_ms, end_time_ms);
        }

        /// \brief Erases all bar data for the given market identifiers and time frame.
        /// \param guard Active transaction guard.
        /// \param market_type Market type.
        /// \param exchange_id Exchange ID.
        /// \param symbol_id Symbol ID.
        /// \param time_frame Time frame.
        void erase(
                const TransactionGuardPtr &guard,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                dfh::TimeFrame time_frame) {
            for (size_t db_index = 0; db_index < m_storage_list.size(); ++db_index) {
                m_storage_list[db_index]->erase(get_transaction(guard.get(), db_index), market_type, exchange_id, symbol_id, time_frame);
            }
        }

        /// \brief Erases all bar data for the given symbol key and time frame.
        /// \param guard Active transaction guard.
        /// \param symbol_key 32-bit symbol key.
        /// \param time_frame Time frame.
        void erase(
                const TransactionGuardPtr &guard,
                uint32_t symbol_key,
                dfh::TimeFrame time_frame) {
            dfh::MarketType market_type;
            uint16_t exchange_id, symbol_id;
            dfh::extract_symbol_key32(symbol_key, market_type, exchange_id, symbol_id);
            erase(guard, market_type, exchange_id, symbol_id, time_frame);
        }

        /// \brief Erases all stored data from all backends.
        /// \param guard Active transaction guard.
        /// \warning This operation is irreversible and will delete all data.
        void erase_all_data(const TransactionGuardPtr &guard) {
            for (size_t db_index = 0; db_index < m_storage_list.size(); ++db_index) {
                m_storage_list[db_index]->erase_all_data(get_transaction(guard.get(), db_index));
            }
        }

    private:
        std::vector<MarketDataStoragePtr> m_storage_list;     ///< List of storage backend instances.
        std::vector<StorageMetadata>      m_storage_metadata; ///< Cached metadata for routing decisions.
        bool m_started = false; ///< Indicates whether the hub has been started.

        /// \brief Finds the index of the first storage backend that matches the given metadata conditions.
        /// \param flags Type of data to store (e.g., BARS, TICKS).
        /// \param market_type Market type (e.g., SPOT, FUTURES).
        /// \param exchange_id Exchange identifier.
        /// \param symbol_id Symbol identifier.
        /// \param timestamp_ms Timestamp to check range coverage.
        /// \return Index of the first suitable storage.
        /// \throws dfh::storage::StorageException If no matching backend is found.
        inline size_t find_storage_index(
                StorageDataFlags flags,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                uint64_t timestamp_ms) {
            for (size_t i = 0; i < m_storage_metadata.size(); ++i) {
                const auto& s = m_storage_metadata[i];
                if (s.has_flag(flags) &&
                    s.has_symbol(symbol_id) &&
                    s.has_exchange(exchange_id) &&
                    s.has_market_type(market_type) &&
                    s.contains_time(timestamp_ms)) return i;
            }
            throw StorageException("No suitable storage found for the given metadata conditions.");
        }

    }; // MarketDataStorageHub

}; // namespace dfh::storage

#endif // _DFH_STORAGE_MARKET_DATA_STORAGE_HUB_HPP_INCLUDED
