#pragma once
#ifndef _DFH_MDBX_FUNDING_DB_HPP_INCLUDED
#define _DFH_MDBX_FUNDING_DB_HPP_INCLUDED

/// \file FundingDB.hpp
/// \brief Provides an interface for storing and retrieving funding rate data in an MDBX database.

namespace dfh::storage::mdbx {

	/// \class FundingDB
    /// \brief Manages the storage and retrieval of funding rate data in an MDBX database.
    class FundingDB {
    public:

		/// \brief Constructs a FundingDB instance.
        /// \param connection Reference to an active MDBX connection.
		FundingDB(MDBXConnection &connection) : m_connection(connection) {}

		/// \brief Destructor.
		~FundingDB() {
			if (m_dbi_ticks) {
                mdbx_dbi_close(m_connection.env_handle(), m_dbi_funding_rates);
            }
			if (m_dbi_metadata) {
                mdbx_dbi_close(m_connection.env_handle(), m_dbi_metadata);
            }
		}

		/// \brief Initializes the database and loads metadata.
        /// \throws MDBXException if database initialization fails.
		void start() {
			MDBXTransaction transaction(m_connection.env_handle(), MDBX_TXN_READWRITE);
			int rc = 0;
			try {
                // Открываем (или создаем) подбазу данных "funding_rates"
                rc = mdbx_dbi_open(transaction.handle(), "funding_rates", MDBX_CREATE | MDBX_INTEGERKEY, &m_dbi_funding_rates);
                if (rc != MDBX_SUCCESS) {
                    throw MDBXException("Failed to open 'funding_rates' database: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc), rc));
                }
                // Открываем (или создаем) подбазу данных "tick_metadata"
                rc = mdbx_dbi_open(transaction.handle(), "funding_metadata", MDBX_CREATE | MDBX_INTEGERKEY, &m_dbi_metadata);
                if (rc != MDBX_SUCCESS) {
                    throw MDBXException("Failed to open 'funding_metadata' database: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc), rc));
                }

                get_all_fixed_map_key32(transaction.handle(), m_dbi_metadata, m_metadata);
            } catch(...) {
                transaction.abort();
                throw;
            }
			transaction.commit();
		}

		/// \brief Closes the database connections.
        /// \throws MDBXException if closing fails.
        void stop() {
			int rc = 0;
			if (m_dbi_funding_rates) {
				rc |= mdbx_dbi_close(m_connection.env_handle(), m_dbi_funding_rates);
			}
			if (m_dbi_metadata) {
				rc |= mdbx_dbi_close(m_connection.env_handle(), m_dbi_metadata);
			}
			if (rc != MDBX_SUCCESS) {
				throw MDBXException("Failed to close database: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc), rc);
			}
		}

        /// \brief Inserts or updates funding metadata in the database.
        /// \param metadata The funding metadata entry to insert or update.
		void upsert(const FundingMetadata& metadata) {
            uint32_t metadata_key = generate_metadata_key(metadata.symbol_id, metadata.provider_id);
            auto it_metadata = m_metadata.find(metadata_key);
            if (it_metadata == m_metadata.end()) {
                m_metadata[metadata_key] = metadata;
            } else
            if (std::memcmp(&it_metadata->second, &metadata, sizeof(FundingMetadata)) == 0) {
                return;
            }

            MDBXTransaction transaction(m_connection.env_handle(), MDBX_TXN_READWRITE);
            put_fixed_key32(transaction.handle(), m_dbi_metadata, metadata_key, metadata);
            transaction.commit();
        }

        /// \brief Inserts or updates multiple funding metadata records.
        /// \details If metadata with the same symbol and provider already exists, it will be updated.
        /// \param metadata_list A vector of funding metadata entries to insert or update.
        /// \throws MDBXException if the operation fails.
        void upsert(const std::vector<FundingMetadata>& metadata_list) {
            MDBXTransaction transaction(m_connection.env_handle(), MDBX_TXN_READWRITE);
            for (const auto& metadata : metadata_list) {
                uint32_t metadata_key = generate_metadata_key(metadata.symbol_id, metadata.provider_id);
                auto it_metadata = m_metadata.find(metadata_key);
                if (it_metadata == m_metadata.end()) {
                    m_metadata[metadata_key] = metadata;
                } else
                if (std::memcmp(&it_metadata->second, &metadata, sizeof(FundingMetadata)) == 0) {
                    continue;
                } else {
                    it_metadata->second = metadata;
                }
                put_fixed_key32(transaction.handle(), m_dbi_metadata, metadata_key, metadata);
            }
            transaction.commit();
        }

        /// \brief Retrieves funding metadata by symbol and provider.
        /// \param symbol_id The symbol identifier.
        /// \param provider_id The provider identifier.
        /// \param metadata Reference to store the retrieved funding metadata.
        /// \return True if metadata was found, false otherwise.
        bool fetch(
                uint16_t symbol_id,
                uint16_t provider_id,
                FundingMetadata& metadata) {
            uint32_t metadata_key = generate_metadata_key(symbol_id, provider_id);
            bool res;
            m_connection.renew();
            try {
                res = get_fixed_key32(m_connection.rdonly_handle(), m_dbi_metadata, metadata_key, metadata);
            } catch(...) {
                m_connection.reset();
                throw;
            }
            m_connection.reset();
            return res;
        }

        /// \brief Retrieves all funding metadata records.
        /// \param metadata_records Reference to a vector where the retrieved funding metadata will be stored.
        /// \return True if at least one metadata record was found, false otherwise.
        /// \throws MDBXException if retrieval fails.
        bool fetch_metadata(std::vector<FundingMetadata>& metadata_records) {
            int rc = 0;
            bool res;
            MDBX_cursor *cursor = nullptr;
            m_connection.renew();
            try {
                res = get_all_metadata(m_connection.rdonly_handle(), metadata_records);
            } catch(...) {
                m_connection.reset();
                throw;
            }
            m_connection.reset();
            return res;
        }

        /// \brief Retrieves all funding metadata records as a key-value map.
        /// \param metadata_map Reference to a map where the retrieved funding metadata will be stored.
        ///        The key is a unique metadata identifier (uint32_t).
        /// \return True if at least one metadata record was found, false otherwise.
        /// \throws MDBXException if retrieval fails.
        bool fetch_metadata(std::unordered_map<uint32_t, FundingMetadata>& metadata_map) {
            bool res;
            MDBX_cursor *cursor = nullptr;
            m_connection.renew();
            try {
                res = get_all_metadata(m_connection.rdonly_handle(), metadata_map);
            } catch(...) {
                m_connection.reset();
                throw;
            }
            m_connection.reset();
            return res;
        }

        /// \brief Returns a copy of all cached funding metadata.
        /// \return A map containing all cached funding metadata.
        std::unordered_map<uint32_t, FundingMetadata> cached_metadata() const {
            return m_metadata;
        }

        /// \brief Retrieves funding metadata from the in-memory cache by symbol and provider.
        /// \param symbol_id The symbol identifier.
        /// \param provider_id The provider identifier.
        /// \param metadata Reference to store the retrieved funding metadata.
        /// \return True if metadata was found in cache, false otherwise.
        bool get_cached_metadata(
                uint16_t symbol_id,
                uint16_t provider_id,
                FundingMetadata& metadata) const {
            uint32_t metadata_key = generate_metadata_key(symbol_id, provider_id);
            auto it = m_metadata.find(metadata_key);
            if (it != m_metadata.end()) {
                metadata = it->second;
                return true;
            }
            return false;
        }

        /// \brief Retrieves all cached funding metadata records.
        /// \param metadata_records Reference to a vector where the cached funding metadata will be stored.
        /// \return True if at least one metadata record was found in cache, false otherwise
        bool get_cached_metadata(std::vector<FundingMetadata>& metadata_records) const {
            if (m_metadata.empty()) return false;
            metadata_records.reserve(metadata_records.size() + m_metadata.size());
            for (const auto& [key, metadata] : m_metadata) {
                metadata_records.push_back(metadata);
            }
            return true;
        }

        /// \brief Retrieves all cached funding metadata records as a key-value map.
        /// \param metadata_map Reference to a map where the cached funding metadata will be stored.
        /// \return True if at least one metadata record was found in cache, false otherwise.
        bool get_cached_metadata(std::unordered_map<uint32_t, FundingMetadata>& metadata_map) const {
            if (m_metadata.empty()) return false;
            metadata_map.insert(m_metadata.begin(), m_metadata.end());
            return true;
        }

        /// \brief Inserts or updates tick data in hourly fragments.
        /// \details Tick data is stored in hourly segments, replacing entire segments.
        /// \param symbol_id The symbol identifier.
        /// \param provider_id The provider identifier.
        /// \param fundings The tick data to insert.
        /// \throws MDBXException if storage fails.
		void upsert_funding(
                uint16_t symbol_id,
                uint16_t provider_id,
                const std::vector<FundingRate>& fundings) {
            if (fundings.empty()) return;

			uint64_t start_ts = fundings.front().time_ms;
            uint64_t end_ts   = fundings.back().time_ms;

            MDBXTransaction transaction(m_connection.env_handle(), MDBX_TXN_READWRITE);

            uint32_t metadata_key = generate_metadata_key(symbol_id, provider_id);
            auto it_metadata = m_metadata.find(metadata_key);
            if (it_metadata == m_metadata.end()) {
                FundingMetadata metadata;
                metadata.symbol_id   = symbol_id;
                metadata.provider_id = provider_id;
                metadata.start_ts = fundings.front().time_ms;
                metadata.end_ts   = fundings.back().time_ms;
                m_metadata[metadata_key] = metadata;
                it_metadata = m_metadata.find(metadata_key);
                put_metadata(transaction.handle(), metadata_key, metadata);
            } else {
                if (start_ts < it_metadata->start_ts ||
                    end_ts > it_metadata->end_ts) {
                    it_metadata->start_ts = start_ts;
                    it_metadata->end_ts = end_ts;
                    put_metadata(transaction.handle(), metadata_key, *it_metadata);
                }
            }

			for (const auto &funding : fundings) {
				const uint32_t unix_hour = round_to_nearest_hour(funding.time_ms);
				put_funding(
                    transaction.handle(),
                    generate_funding_key(symbol_id, provider_id, unix_hour),
                    funding, (fundings.size() >= 2));
			}

			transaction.commit();
		}

		/// \brief Retrieves tick data.
        /// \param symbol_id The symbol identifier.
        /// \param provider_id The provider identifier.
        /// \param start_ts The start timestamp.
        /// \param end_ts The end timestamp.
        /// \param ticks The vector where the retrieved tick data will be appended.
        /// \return True if any ticks were found, otherwise false.
        /// \throws MDBXException if retrieval fails.
		bool fetch_funding(
                uint16_t symbol_id,
                uint16_t provider_id,
                uint64_t start_ts,
                uint64_t end_ts,
                std::vector<FundingRate>& fundings) {
            uint64_t start_hour = time_shield::ms_to_hour(start_ts);
            uint64_t end_hour = time_shield::ms_to_hour(end_ts - 1);
            m_connection.renew();
            try {
                for (uint64_t unix_hour = start_hour; unix_hour <= end_hour; ++unix_hour) {
                    uint64_t key = generate_tick_key(symbol_id, provider_id, unix_hour);
                    m_buffer.clear();
                    if (!get_tick(m_connection.rdonly_handle(), key, m_buffer)) continue;
                    m_tick_serializer.deserialize(m_buffer, ticks, codec_config);
                }

                if (start_ts > time_shield::hour_to_ms(start_hour)) {
                    auto it_start = std::lower_bound(ticks.begin(), ticks.end(), start_ts,
                        [](const MarketTick& tick, uint64_t timestamp) {
                            return tick.time_ms < timestamp;
                        });
                    if (it_start != ticks.begin()) {
                        ticks.erase(ticks.begin(), it_start); // Remove elements before start_ts
                    }
                }

                if (end_ts < (time_shield::hour_to_ms(end_hour) + time_shield::MS_PER_HOUR)) {
                    auto it_end = std::lower_bound(ticks.begin(), ticks.end(), end_ts,
                        [](const MarketTick& tick, uint64_t timestamp) {
                            return tick.time_ms < timestamp;
                        });
                    if (it_end != ticks.end()) {
                        ticks.erase(it_end, ticks.end());    // Remove elements >= end_ts
                    }
                }
            } catch(...) {
                m_connection.reset();
                throw;
            }
            m_connection.reset();
            return !ticks.empty();
        }

	private:
		std::unordered_map<uint32_t, FundingMetadata> m_metadata;
		std::vector<uint8_t> m_buffer;
		MDBXConnection& m_connection;
		MDBX_dbi m_dbi_funding_rates = 0;
		MDBX_dbi m_dbi_metadata      = 0;
		MDBX_cursor *m_ticks_cursor  = nullptr;

		/// \brief Inserts or updates funding data in the database.
		/// \param txn A pointer to the MDBX transaction handle.
		/// \param key The funding identifier (64-bit key, e.g. timestamp-based).
		/// \param funding The funding rate data to insert or update.
		/// \param batch_write Indicates if batch writing flags should be used.
		/// \throws MDBXException if the operation fails.
        void put_funding(MDBX_txn *txn, uint64_t key, const FundingRate& funding, bool batch_write) {
			MDBX_val db_key{ sizeof(key), &key };
            MDBX_val db_data{ sizeof(FundingRate), &funding };

            int rc = mdbx_put(txn, m_dbi_funding_rates, &db_key, &db_data, batch_write ? (MDBX_APPEND | MDBX_UPSERT) : MDBX_UPSERT);
            if (rc != MDBX_SUCCESS) {
                throw MDBXException("Failed to put funding rates data: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
            }
        }

		/// \brief Retrieves funding data associated with the given key.
        /// \param txn A pointer to the MDBX transaction handle.
        /// \param key The identifier for the funding.
        /// \param funding
        /// \return True if the tick data was found, otherwise false.
        /// \throws MDBXException if retrieval fails.
		bool get_funding(MDBX_txn *txn, uint64_t key, FundingRate& funding) {
			MDBX_val db_key{ sizeof(key), &key };
			MDBX_val db_data;

			int rc = mdbx_get(txn, m_dbi_funding_rates, &db_key, &db_data);
			if (rc == MDBX_NOTFOUND) {
				return false; // Data not found
			}
			if (rc != MDBX_SUCCESS) {
				throw MDBXException("Failed to get tick data: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
			}

            out_data.assign(static_cast<uint8_t*>(db_data.iov_base), static_cast<uint8_t*>(db_data.iov_base) + db_data.iov_len);
			return true;
		}

		/// \brief Inserts or updates tick metadata in the database.
        /// \param txn A pointer to the MDBX transaction handle.
        /// \param key The metadata identifier (32-bit key based on symbol and provider).
        /// \param metadata A reference to the metadata structure to insert or update.
        /// \param batch_write
        /// \throws MDBXException if the operation fails.
        void put_metadata(MDBX_txn *txn, uint32_t key, const FundingMetadata& metadata, bool batch_write) {
			MDBX_val db_key{ sizeof(key), &key };
            MDBX_val db_data{ sizeof(FundingMetadata), const_cast<void*>(&metadata) };

            int rc = mdbx_put(txn, m_dbi_metadata, &db_key, &db_data, batch_write ? (MDBX_APPEND | MDBX_UPSERT) : MDBX_UPSERT);
            if (rc != MDBX_SUCCESS) {
                throw MDBXException("Failed to put metadata: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
            }
        }

		/// \brief Retrieves tick metadata associated with the given key.
        /// \param txn A pointer to the MDBX transaction handle.
        /// \param key The metadata identifier (32-bit key based on symbol and provider).
        /// \param out_metadata A reference to the metadata object where the retrieved metadata will be stored.
        /// \return True if the metadata was found, otherwise false.
        /// \throws MDBXException if retrieval fails.
		bool get_metadata(MDBX_txn *txn, uint32_t key, FundingMetadata& out_metadata) {
			MDBX_val db_key{ sizeof(key), &key };
			MDBX_val db_data;

			int rc = mdbx_get(txn, m_dbi_metadata, &db_key, &db_data);
			if (rc == MDBX_NOTFOUND) {
				return false; // Metadata not found
			}
			if (rc != MDBX_SUCCESS) {
				throw MDBXException("Failed to get metadata: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
			}

			std::memcpy(&out_metadata, db_data.iov_base, db_data.iov_len);
			return true;
		}

		/// \brief Retrieves all tick metadata from the database.
        /// \param txn A pointer to the MDBX transaction handle.
        /// \param out_metadata A reference to a vector where the retrieved metadata will be stored.
        /// \return True if metadata was found, otherwise false.
        /// \throws MDBXException if retrieval fails.
		bool get_all_metadata(MDBX_txn *txn, std::vector<FundingMetadata>& out_metadata) {
			MDBX_cursor* cursor = nullptr;
			int rc = mdbx_cursor_open(txn, m_dbi_metadata, &cursor);
			if (rc != MDBX_SUCCESS) {
				throw MDBXException("Failed to open cursor for metadata: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
			}

			MDBX_val db_key, db_data;
			while ((rc = mdbx_cursor_get(cursor, &db_key, &db_data, MDBX_NEXT)) == MDBX_SUCCESS) {
				if (db_key.iov_len == sizeof(uint32_t)) {
					FundingMetadata metadata;
					std::memcpy(&metadata, db_data.iov_base, db_data.iov_len);
					out_metadata.push_back(std::move(metadata));
				}
			}

			bool res = (rc != MDBX_NOTFOUND);
			rc = mdbx_cursor_close(cursor);
			if (rc != MDBX_SUCCESS) {
				throw MDBXException("Failed to close metadata cursor: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
			}
			return res;
		}

		/// \brief Retrieves all tick metadata as a map from the database.
        /// \param txn A pointer to the MDBX transaction handle.
        /// \param out_metadata_map A reference to a map where the retrieved metadata will be stored.
        /// \return True if metadata was found, otherwise false.
        /// \throws MDBXException if retrieval fails.
		bool get_all_metadata(MDBX_txn *txn, std::unordered_map<uint32_t, FundingMetadata>& out_metadata_map) {
			MDBX_cursor* cursor = nullptr;
			int rc = mdbx_cursor_open(txn, m_dbi_metadata, &cursor);
			if (rc != MDBX_SUCCESS) {
				throw MDBXException("Failed to open cursor for metadata: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
			}

			uint32_t key;
			MDBX_val db_key, db_data;
			while ((rc = mdbx_cursor_get(cursor, &db_key, &db_data, MDBX_NEXT)) == MDBX_SUCCESS) {
				if (db_key.iov_len == sizeof(uint32_t)) {
					FundingMetadata metadata;
					std::memcpy(&key, db_key.iov_base, sizeof(uint32_t));
					std::memcpy(&metadata, db_data.iov_base, db_data.iov_len);
					out_metadata_map[key] = std::move(metadata);
				}
			}

			bool res = (rc != MDBX_NOTFOUND);
			rc = mdbx_cursor_close(cursor);
			if (rc != MDBX_SUCCESS) {
				throw MDBXException("Failed to close metadata cursor: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc)));
			}
			return res;
		}

		/// \brief Generates a 64-bit key for funding rate storage.
		/// \param symbol_id The symbol identifier (should fit within 16 bits).
		/// \param provider_id The provider identifier (should fit within 16 bits).
		/// \param unix_hour The Unix timestamp rounded to the hour (in hours since Unix epoch).
		/// \return A unique 64-bit key constructed as:
		///         [unix_hour (32 bits)] | [provider_id (16 bits)] | [symbol_id (16 bits)].
		uint64_t generate_funding_key(uint32_t symbol_id, uint32_t provider_id, uint32_t unix_hour) {
			return ((uint64_t)provider_id << 16) | (uint64_t)symbol_id | ((uint64_t)unix_hour << 32);
		}

		/// \brief Generates a 32-bit key for metadata storage.
		/// \param symbol_id The symbol identifier (should be a 16-bit value).
		/// \param provider_id The provider identifier (should be a 16-bit value).
		/// \return A unique 32-bit key constructed as:
		///         [provider_id (upper 16 bits)] | [symbol_id (lower 16 bits)].
		uint32_t generate_metadata_key(uint32_t symbol_id, uint32_t provider_id) {
			return (provider_id << 16) | symbol_id;
		}

		/// \brief Rounds a timestamp (in milliseconds) to the nearest hour.
        /// \param time_ms Timestamp in milliseconds since Unix epoch.
        /// \return The rounded value expressed as the number of hours since Unix epoch.
        uint32_t round_to_nearest_hour(uint64_t time_ms) const {
            return static_cast<uint32_t>((time_ms + time_shield::MS_PER_HALF_HOUR) / ms_per_hour);
        }
    };

};

#endif // _DFH_MDBX_FUNDING_DB_HPP_INCLUDED
