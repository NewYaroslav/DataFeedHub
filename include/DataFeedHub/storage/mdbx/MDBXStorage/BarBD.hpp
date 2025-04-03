#pragma once
#ifndef _DFH_STORAGE_MDBX_BAR_BD_HPP_INCLUDED
#define _DFH_STORAGE_MDBX_BAR_BD_HPP_INCLUDED

/// \file BarBD.hpp
/// \brief Manages the storage and retrieval of bar (OHLCV) data in an MDBX database.

namespace dfh::storage::mdbx {

    /// \class BarBD
    /// \brief Handles saving and loading of bar data in MDBX.
    class BarBD {
    public:

        /// \brief Initializes the BarBD instance with the given MDBX connection.
        /// \param connection Pointer to an active MDBX connection.
        BarBD(MDBXConnection* connection)
            : m_connection(connection) {}

        /// \brief Cleans up database handles on destruction.
        ~BarBD() {
            for (auto& dbi : m_dbi_bars) {
                if (dbi) mdbx_dbi_close(m_connection->env_handle(), dbi);
            }
            if (m_dbi_metadata) {
                mdbx_dbi_close(m_connection->env_handle(), m_dbi_metadata);
            }
        }

        /// \brief Opens all required bar data tables and metadata table.
        /// \param txn MDBX transaction used to open the tables.
        /// \throws MDBXException if any table fails to open.
        void start(MDBXTransaction *txn) {
            for (size_t i = 0; i < m_dbi_bars.size(); ++i) {
                std::string name = make_table_name(timeframe_values[i]);
                int rc = mdbx_dbi_open(txn->handle(), name.c_str(), MDBX_CREATE | MDBX_INTEGERKEY, &m_dbi_bars[i]);
                if (rc != MDBX_SUCCESS) {
                    throw MDBXException("Failed to open '" + name + "' database: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc), rc));
                }
            }

            int rc = mdbx_dbi_open(txn->handle(), "bar_metadata", MDBX_CREATE | MDBX_INTEGERKEY, &m_dbi_metadata);
            if (rc != MDBX_SUCCESS) {
                throw MDBXException("Failed to open 'bar_metadata' database: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc), rc));
            }
        }

        /// \brief Closes all opened database handles.
        /// \throws MDBXException if any close operation fails.
        void stop() {
            int rc = 0;
            for (auto& dbi : m_dbi_bars) {
                if (dbi) rc |= mdbx_dbi_close(m_connection->env_handle(), dbi);
            }
            if (m_dbi_metadata) {
                rc |= mdbx_dbi_close(m_connection->env_handle(), m_dbi_metadata);
            }
            if (rc != MDBX_SUCCESS) {
                throw MDBXException("Failed to close database: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc), rc));
            }
        }

        /// \brief Writes all pending metadata updates to the database.
        /// \param txn Active transaction used for writing.
        /// Should only be called after prepare_metadata().
        void after_transaction(MDBXTransaction *txn) {
            if (!m_prepare_metadata) return;
            for (const auto& [meta_key, metadata] : m_metadata) {
                put_fixed_key<uint64_t>(txn->handle(), m_dbi_metadata, meta_key, metadata);
            }
            m_prepare_metadata = false;
        }

        /// \brief Loads all bar metadata from the database into memory.
        /// \param txn Active transaction.
        /// Required before using metadata during the transaction.
        void prepare_metadata(MDBXTransaction *txn) {
            m_metadata.clear();
            get_all_fixed_map<uint64_t>(txn->handle(), m_dbi_metadata, m_metadata);
            m_prepare_metadata = true;
        }

        /// \brief Inserts or updates a segment of bar data.
        /// \param txn Active transaction.
        /// \param market_type Market type.
        /// \param exchange_id Exchange identifier.
        /// \param symbol_id Symbol identifier.
        /// \param bars Vector of bars to store.
        /// \param config Codec config describing compression and metadata.
        /// \throws MDBXException if serialization or insertion fails.
        void upsert(
                MDBXTransaction *txn,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                const std::vector<MarketBar>& bars,
                const BarCodecConfig& config) {
            if (bars.empty()) return;
            const uint64_t duration_ms = dfh::get_segment_duration_ms(config.time_frame);
            const uint64_t segment_key = bars.front().time_ms / duration_ms;
            if (bars.back().time_ms >= ((segment_key * duration_ms) + duration_ms)) {
                throw MDBXException("BarBD::upsert(): Data range crosses segment boundary. Ensure all bars fit within a single segment.");
            }

            const uint32_t symbol_key = dfh::make_symbol_key32(market_type, exchange_id, symbol_id);
            const uint64_t data_key = dfh::make_symbol_key64(symbol_key, segment_key);

            if (m_prepare_metadata) {
                const uint64_t meta_key = dfh::make_symbol_key64(symbol_key, static_cast<uint64_t>(config.time_frame));
                auto it_metadata = m_metadata.find(meta_key);
                if (it_metadata != m_metadata.end()) {
                    BarMetadata& meta = it_metadata->second;

                    m_buffer.clear();
                    if (get_raw_key<uint64_t>(txn->handle(), m_dbi_bars[tf_index(config.time_frame)], data_key, m_buffer)) {
                        uint32_t count = dfh::compression::extract_num_samples(m_buffer.data(), m_buffer.size());
                        if (meta.count >= count) {
                            meta.count -= count;
                            meta.count += static_cast<uint32_t>(bars.size());
                        } else {
                            meta.count = static_cast<uint32_t>(bars.size());
                        }
                    } else {
                        meta.count = static_cast<uint32_t>(bars.size());
                    }

                    if (bars.front().time_ms < meta.start_time_ms) meta.start_time_ms = bars.front().time_ms;
                    if (bars.back().time_ms > meta.end_time_ms) meta.end_time_ms = bars.back().time_ms;
                    if (config.expiration_time_ms != meta.expiration_time_ms ||
                        config.next_expiration_time_ms != meta.next_expiration_time_ms) {
                        meta.expiration_time_ms = config.expiration_time_ms;
                        meta.next_expiration_time_ms = config.next_expiration_time_ms;
                    }
                    if (config.tick_size != meta.tick_size ||
                        config.price_digits != meta.price_digits ||
                        config.volume_digits != meta.volume_digits ||
                        config.quote_volume_digits != meta.quote_volume_digits) {
                        meta.tick_size     = config.tick_size;
                        meta.price_digits  = config.price_digits;
                        meta.volume_digits = config.volume_digits;
                        meta.quote_volume_digits = config.quote_volume_digits;
                    }
                } else {
                    BarMetadata meta;
                    meta.start_time_ms = bars.front().time_ms;
                    meta.end_time_ms   = bars.back().time_ms;
                    meta.expiration_time_ms      = config.expiration_time_ms;
                    meta.next_expiration_time_ms = config.next_expiration_time_ms;
                    meta.tick_size     = config.tick_size;
                    meta.time_frame    = config.time_frame;
                    meta.flags         = config.flags;
                    meta.count         = static_cast<uint32_t>(bars.size());
                    meta.market_type   = market_type;
                    meta.exchange_id   = exchange_id;
                    meta.symbol_id     = symbol_id;
                    meta.price_digits  = config.price_digits;
                    meta.volume_digits = config.volume_digits;
                    meta.quote_volume_digits = config.quote_volume_digits;
                    m_metadata.emplace(meta_key, std::move(meta));
                }
            }

            m_buffer.clear();
            m_serializer.serialize(bars, config, m_buffer);
            put_raw_key<uint64_t>(
                txn->handle(),
                m_dbi_bars[tf_index(config.time_frame)],
                data_key,
                m_buffer.data(), m_buffer.size());
        }

        /// \brief Fetches a bar metadata by symbol components.
        /// \param txn Active transaction.
        /// \param market_type Market type.
        /// \param exchange_id Exchange ID.
        /// \param symbol_id Symbol ID.
        /// \param time_frame Target time frame.
        /// \param metadata Output metadata object.
        /// \return True if metadata is found, false otherwise.
        bool fetch(
                MDBXTransaction *txn,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                dfh::TimeFrame time_frame,
                dfh::BarMetadata& metadata) {
            return get_fixed_key<uint64_t>(txn->handle(), m_dbi_metadata,
                    dfh::make_symbol_key64(
                    dfh::make_symbol_key32(market_type, exchange_id, symbol_id),
                    static_cast<uint64_t>(time_frame)), metadata);
        }

        /// \brief Fetches a segment of bar data from the storage.
        /// \param txn Active transaction.
        /// \param market_type Market type.
        /// \param exchange_id Exchange identifier.
        /// \param symbol_id Symbol identifier.
        /// \param time_frame Target time frame.
        /// \param segment_key Aligned segment timestamp.
        /// \param out_bars Output vector for bars.
        /// \param out_configs Output for codec config.
        /// \return True if data is found, false otherwise.
        bool fetch(
                MDBXTransaction *txn,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                dfh::TimeFrame time_frame,
                uint64_t segment_key,
                std::vector<dfh::MarketBar>& out_bars,
                dfh::BarCodecConfig& out_configs) {
            m_buffer.clear();
            if (!get_raw_key<uint64_t>(txn->handle(), m_dbi_bars[tf_index(time_frame)],
                    dfh::make_symbol_key64(
                    dfh::make_symbol_key32(market_type, exchange_id, symbol_id),
                    segment_key), m_buffer)) {
                return false;
            }
            m_serializer.deserialize(m_buffer, out_bars, out_configs);
            return true;
        }

        /// \brief Erases data defined by the specified metadata from the backend.
        /// \param txn Active transaction.
        /// \param metadata Metadata describing the data to be removed.
        void erase_data(MDBXTransaction *txn, const StorageMetadata& metadata) {
            for (auto market_type : metadata.market_types())
            for (auto exchange_id : metadata.exchange_ids())
            for (auto symbol_id : metadata.symbol_ids()) {
                if (metadata.start_time_ms() == 0 || metadata.end_time_ms() == 0) {
                    for (size_t i = 0; i < m_dbi_bars.size(); ++i) {
                        erase_key_masked<uint64_t>(txn->handle(), m_dbi_bars[i],
                            dfh::KEY64_SYMBOL_PART_MASK,
                            dfh::make_symbol_key64(dfh::make_symbol_key32(market_type, exchange_id, symbol_id), 0));
                    }
                    continue;
                }
                for (size_t i = 0; i < timeframe_values.size(); ++i) {
                    const uint64_t duration_ms = dfh::get_segment_duration_ms(time_shield::sec_to_ms(timeframe_values[i]));
                    const uint64_t segment_start = metadata.start_time_ms() / duration_ms;
                    const uint64_t segment_stop  = (metadata.end_time_ms() - 1) / duration_ms;

                    for (uint64_t segment = segment_start; segment <= segment_stop; ++segment) {
                        erase(txn, market_type, exchange_id, symbol_id, to_timeframe(timeframe_values[i]), segment);
                    }
                }
            } // for symbol_id
        }

        /// \brief Erases a specific segment of bar data.
        /// \param txn Active transaction.
        /// \param market_type Market type.
        /// \param exchange_id Exchange identifier.
        /// \param symbol_id Symbol identifier.
        /// \param time_frame Target time frame.
        /// \param segment_key Segment timestamp to erase.
        void erase(
                MDBXTransaction *txn,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                dfh::TimeFrame time_frame,
                uint64_t segment_key) {

            const uint32_t symbol_key = dfh::make_symbol_key32(market_type, exchange_id, symbol_id);
            const uint64_t meta_key = dfh::make_symbol_key64(symbol_key, static_cast<uint64_t>(time_frame));
            const uint64_t data_key = dfh::make_symbol_key64(symbol_key, segment_key);

            bool has_meta = false;
            dfh::BarMetadata meta;
            if (m_prepare_metadata) {
                auto it_metadata = m_metadata.find(meta_key);
                if (it_metadata != m_metadata.end()) {
                    meta = it_metadata->second;
                    has_meta = true;
                }
            } else
            if (get_fixed_key<uint64_t>(txn->handle(), m_dbi_metadata, meta_key, meta)) {
                has_meta = true;
            }

            if (has_meta) {
                m_buffer.clear();
                if (get_raw_key<uint64_t>(txn->handle(), m_dbi_bars[tf_index(time_frame)], data_key, m_buffer)) {
                    uint32_t count = dfh::compression::extract_num_samples(m_buffer.data(), m_buffer.size());
                    if (meta.count >= count) {
                        meta.count -= count;
                    } else {
                        meta.count = 0;
                    }
                } else {
                    meta.count = 0;
                }
                if (m_prepare_metadata) {
                    m_metadata[meta_key] = meta;
                } else {
                    put_fixed_key<uint64_t>(txn->handle(), m_dbi_metadata, meta_key, meta);
                }
            }

            erase_key<uint64_t>(txn->handle(), m_dbi_bars[tf_index(time_frame)],
                dfh::make_symbol_key64(
                dfh::make_symbol_key32(market_type, exchange_id, symbol_id), segment_key));
        }

        /// \brief Erases all segments for a specific symbol and timeframe.
        /// \param txn Active transaction.
        /// \param market_type Market type.
        /// \param exchange_id Exchange identifier.
        /// \param symbol_id Symbol identifier.
        /// \param time_frame Target time frame.
        void erase(
                MDBXTransaction *txn,
                dfh::MarketType market_type,
                uint16_t exchange_id,
                uint16_t symbol_id,
                dfh::TimeFrame time_frame) {
            erase_key_masked<uint64_t>(txn->handle(), m_dbi_bars[tf_index(time_frame)],
                dfh::KEY64_SYMBOL_PART_MASK,
                dfh::make_symbol_key64(dfh::make_symbol_key32(market_type, exchange_id, symbol_id), 0));
        }

        /// \brief Erases all bar data for the given time frame.
        /// \param txn Active transaction.
        /// \param time_frame Time frame to clear.
        void erase(
                MDBXTransaction *txn,
                dfh::TimeFrame time_frame) {
            erase_all_entries(txn->handle(), m_dbi_bars[tf_index(time_frame)]);
        }

        /// \brief Erases all bar and metadata records from the backend.
        /// \param txn Active transaction.
        /// \warning This action deletes all data irreversibly.
        void erase_all_data(MDBXTransaction *txn) {
            for (size_t i = 0; i < m_dbi_bars.size(); ++i) {
                erase_all_entries(txn->handle(), m_dbi_bars[i]);
            }
            erase_all_entries(txn->handle(), m_dbi_metadata);
        }

    private:
        MDBXConnection* m_connection;
        MDBX_dbi m_dbi_metadata = 0;
        std::array<MDBX_dbi, 11> m_dbi_bars{};
        dfh::compression::BarSerializer m_serializer;
        std::unordered_map<uint64_t, dfh::BarMetadata> m_metadata;
        std::vector<uint8_t> m_buffer;
        bool m_prepare_metadata = false;

        static constexpr std::array<uint32_t, 11> timeframe_values = {
            1,      // 0
            3,      // 1
            5,      // 2
            15,     // 3
            60,     // 4
            300,    // 5
            900,    // 6
            1800,   // 7
            3600,   // 8
            1440,   // 9
            86400,  // 10
        };

        /// \brief Converts TimeFrame to index used in internal arrays.
        /// \param time_frame Time frame enum.
        /// \return Index of the time frame.
        /// \throws MDBXException if the time frame is unknown.
        inline size_t tf_index(dfh::TimeFrame time_frame) {
            switch (time_frame) {
                case TimeFrame::M1:     return 4;
                case TimeFrame::M5:     return 5;
                case TimeFrame::M15:    return 6;
                case TimeFrame::S1:     return 0;
                case TimeFrame::S3:     return 1;
                case TimeFrame::S5:     return 2;
                case TimeFrame::S15:    return 3;
                case TimeFrame::M30:    return 7;
                case TimeFrame::H1:     return 8;
                case TimeFrame::H4:     return 9;
                case TimeFrame::D1:     return 10;
                default: throw MDBXException("tf_index: Unknown TimeFrame value: " + std::to_string(static_cast<uint32_t>(time_frame)));
            }
        }

        /// \brief Creates a table name string for the given time frame.
        /// \param time_frame Time frame.
        /// \return Table name in the format "bars_<seconds>".
        std::string make_table_name(uint32_t time_frame) {
            return "bars_" + std::to_string(time_frame);
        }
    };

} // namespace dfh::storage::mdbx

#endif // _DFH_STORAGE_MDBX_BAR_BD_HPP_INCLUDED

