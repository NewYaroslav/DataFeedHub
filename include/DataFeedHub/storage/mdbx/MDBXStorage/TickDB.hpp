#pragma once
#ifndef _DFH_MDBX_TICK_BD_HPP_INCLUDED
#define _DFH_MDBX_TICK_BD_HPP_INCLUDED

/// \file TickBD.hpp
/// \brief Provides an interface for storing and retrieving tick data in MDBX.

namespace dfh::storage::mdbx {

	/// \class TickBD
    /// \brief Manages the storage and retrieval of tick data in an MDBX database.
    class TickBD {
    public:

		/// \brief Constructs a `TickBD` instance.
        /// \param connection Reference to an active MDBX connection.
		TickBD(MDBXConnection &connection) : m_connection(connection) {}

		/// \brief Destructor.
		~TickBD() {
			if (m_dbi_ticks) {
                mdbx_dbi_close(m_connection.env_handle(), m_dbi_ticks);
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
                // Открываем (или создаем) подбазу данных "ticks"
                rc = mdbx_dbi_open(transaction.handle(), "ticks", MDBX_CREATE | MDBX_INTEGERKEY, &m_dbi_ticks);
                if (rc != MDBX_SUCCESS) {
                    throw MDBXException("Failed to open 'ticks' database: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc), rc));
                }
                // Открываем (или создаем) подбазу данных "tick_metadata"
                rc = mdbx_dbi_open(transaction.handle(), "tick_metadata", MDBX_CREATE | MDBX_INTEGERKEY, &m_dbi_metadata);
                if (rc != MDBX_SUCCESS) {
                    throw MDBXException("Failed to open 'tick_metadata' database: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc), rc));
                }
                // Читаем мета данные
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
			if (m_dbi_ticks) {
				rc |= mdbx_dbi_close(m_connection.env_handle(), m_dbi_ticks);
			}
			if (m_dbi_metadata) {
				rc |= mdbx_dbi_close(m_connection.env_handle(), m_dbi_metadata);
			}
			if (rc != MDBX_SUCCESS) {
				throw MDBXException("Failed to close database: (" + std::to_string(rc) + ") " + std::string(mdbx_strerror(rc), rc));
			}
		}

        /// \brief Inserts or updates tick metadata.
        /// \param metadata The metadata entry to insert or update.
		void upsert(const TickMetadata& metadata) {
            uint32_t metadata_key = generate_metadata_key(metadata.symbol_id, metadata.provider_id);
            auto it_metadata = m_metadata.find(metadata_key);
            if (it_metadata == m_metadata.end()) {
                m_metadata[metadata_key] = metadata;
            } else
            if (std::memcmp(&it_metadata->second, &metadata, sizeof(TickMetadata)) == 0) {
                return;
            }

            MDBXTransaction transaction(m_connection.env_handle(), MDBX_TXN_READWRITE);
            put_fixed_key32(transaction.handle(), m_dbi_metadata, metadata_key, metadata);
            transaction.commit();
        }

        /// \brief Inserts or updates tick metadata.
        /// \details If metadata with the same symbol and provider already exists, it will be updated.
        /// \param metadata_list The metadata entry to insert or update.
        /// \throws MDBXException if the operation fails.
        void upsert(const std::vector<TickMetadata>& metadata_list) {
            MDBXTransaction transaction(m_connection.env_handle(), MDBX_TXN_READWRITE);
            for (const auto& metadata : metadata_list) {
                uint32_t metadata_key = generate_metadata_key(metadata.symbol_id, metadata.provider_id);
                auto it_metadata = m_metadata.find(metadata_key);
                if (it_metadata == m_metadata.end()) {
                    m_metadata[metadata_key] = metadata;
                } else
                if (std::memcmp(&it_metadata->second, &metadata, sizeof(TickMetadata)) == 0) {
                    continue;
                } else {
                    it_metadata->second = metadata;
                }
                put_fixed_key32(transaction.handle(), m_dbi_metadata, metadata_key, metadata);
            }
            transaction.commit();
        }

        /// \brief Retrieves tick metadata by symbol and provider.
        /// \param symbol_id The symbol identifier.
        /// \param provider_id The provider identifier.
        /// \param metadata Reference to store the retrieved metadata.
        /// \return True if metadata was found, false otherwise.
        bool fetch(
                uint16_t symbol_id,
                uint16_t provider_id,
                TickMetadata& metadata) {
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

        /// \brief Retrieves all tick metadata records.
        /// \param metadata_records Reference to a vector where the retrieved metadata will be stored.
        /// \return True if at least one metadata record was found, false otherwise.
        /// \throws MDBXException if retrieval fails.
        bool fetch(std::vector<TickMetadata>& metadata_records) {
            int rc = 0;
            bool res;
            m_connection.renew();
            try {
                res = get_all_fixed_key32(m_connection.rdonly_handle(), m_dbi_metadata, metadata_records);
            } catch(...) {
                m_connection.reset();
                throw;
            }
            m_connection.reset();
            return res;
        }

        /// \brief Retrieves all tick metadata records as a key-value map.
        /// \param metadata_map Reference to a map where the retrieved metadata will be stored.
        ///        The key is a unique metadata identifier (uint32_t).
        /// \return True if at least one metadata record was found, false otherwise.
        /// \throws MDBXException if retrieval fails.
        bool fetch(std::unordered_map<uint32_t, TickMetadata>& metadata_map) {
            bool res;
            m_connection.renew();
            try {
                res = get_all_fixed_map_key32(m_connection.rdonly_handle(), m_dbi_metadata, metadata_map);
            } catch(...) {
                m_connection.reset();
                throw;
            }
            m_connection.reset();
            return res;
        }

        /// \brief Returns a copy of all cached metadata.
        /// \return A map containing all cached tick metadata.
        std::unordered_map<uint32_t, TickMetadata> cached_metadata() const {
            return m_metadata;
        }

        /// \brief Retrieves tick metadata from the in-memory cache by symbol and provider.
        /// \param symbol_id The symbol identifier.
        /// \param provider_id The provider identifier.
        /// \param metadata Reference to store the retrieved metadata.
        /// \return True if metadata was found in cache, false otherwise.
        bool get_cached(
                uint16_t symbol_id,
                uint16_t provider_id,
                TickMetadata& metadata) const {
            uint32_t metadata_key = generate_metadata_key(symbol_id, provider_id);
            auto it = m_metadata.find(metadata_key);
            if (it != m_metadata.end()) {
                metadata = it->second;
                return true;
            }
            return false;
        }

        /// \brief Retrieves all cached tick metadata records.
        /// \param metadata_records Reference to a vector where the cached metadata will be stored.
        /// \return True if at least one metadata record was found in cache, false otherwise.
        bool get_cached(std::vector<TickMetadata>& metadata_records) const {
            if (m_metadata.empty()) return false;
            metadata_records.reserve(metadata_records.size() + m_metadata.size());
            for (const auto& [key, metadata] : m_metadata) {
                metadata_records.push_back(metadata);
            }
            return true;
        }

        /// \brief Retrieves all cached tick metadata records as a key-value map.
        /// \param metadata_map Reference to a map where the cached metadata will be stored.
        /// \return True if at least one metadata record was found in cache, false otherwise.
        bool get_cached(std::unordered_map<uint32_t, TickMetadata>& metadata_map) const {
            if (m_metadata.empty()) return false;
            metadata_map.insert(m_metadata.begin(), m_metadata.end());
            return true;
        }

        /// \brief Inserts or updates tick data in hourly fragments.
        /// \details Tick data is stored in hourly segments, replacing entire segments.
        /// \param symbol_id The symbol identifier.
        /// \param provider_id The provider identifier.
        /// \param ticks The tick data to insert.
        /// \param codec_config The encoding configuration.
        /// \throws MDBXException if storage fails.
		void upsert(
                uint16_t symbol_id,
                uint16_t provider_id,
                const std::vector<MarketTick>& ticks,
                const TickCodecConfig& codec_config) {
            if (ticks.empty()) return;

            std::vector<std::vector<MarketTick>> out_segments;
			if (!split_ticks(ticks, out_segments)) throw MDBXException("Ticks are not in correct order.");

			uint64_t start_ts = ticks.front().time_ms;
            uint64_t end_ts   = ticks.back().time_ms;

            MDBXTransaction transaction(m_connection.env_handle(), MDBX_TXN_READWRITE);

            uint32_t metadata_key = generate_metadata_key(symbol_id, provider_id);
            auto it_metadata = m_metadata.find(metadata_key);
            if (it_metadata == m_metadata.end()) {
                TickMetadata metadata;
                metadata.symbol_id   = symbol_id;
                metadata.provider_id = provider_id;
                metadata.flags = codec_config.flags;
                metadata.price_digits  = codec_config.price_digits;
                metadata.volume_digits = codec_config.volume_digits;
                metadata.price_tick_size  = dfh::utils::precision_tolerance(codec_config.price_digits);
                metadata.volume_step_size = dfh::utils::precision_tolerance(codec_config.volume_digits);
                metadata.start_ts = ticks.front().time_ms;
                metadata.end_ts   = ticks.back().time_ms;
                m_metadata[metadata_key] = metadata;
                it_metadata = m_metadata.find(metadata_key);
                put_fixed_key32(transaction.handle(), m_dbi_metadata, metadata_key, metadata);
            } else {
                bool is_update = false;
                if (time_shield::start_of_hour_ms(end_ts) >=
                    time_shield::start_of_hour_ms(it_metadata->end_ts)) {
                    if (it_metadata->price_digits != codec_config.price_digits) {
                        it_metadata->price_digits = codec_config.price_digits;
                        it_metadata->price_tick_size  = dfh::utils::precision_tolerance(codec_config.price_digits);
                        is_update = true;
                    }
                    if (it_metadata->volume_digits != codec_config.volume_digits) {
                        it_metadata->volume_digits = codec_config.volume_digits;
                        it_metadata->volume_step_size = dfh::utils::precision_tolerance(codec_config.volume_digits);
                        is_update = true;
                    }
                }
                if (start_ts < it_metadata->start_ts ||
                    end_ts > it_metadata->end_ts) {
                    it_metadata->start_ts = start_ts;
                    it_metadata->end_ts = end_ts;
                    is_update = true;
                }
                if (is_update) {
                    put_fixed_key32(transaction.handle(), m_dbi_metadata, metadata_key, *it_metadata);
                }
            }

            std::vector<uint8_t> output;
			for (const auto &segment : out_segments) {
                m_tick_serializer.serialize(segment, codec_config, output);
                const uint32_t unix_hour = time_shield::start_of_hour_ms(segment[0].time_ms);
                put_raw_key64(
                    transaction.handle(),
                    m_dbi_ticks,
                    generate_tick_key(symbol_id, provider_id, unix_hour),
                    output.data(), output.size(),
                    (out_segments.size() >= 2));
			}

			transaction.commit();
		}

        /// \brief Inserts or updates tick data in hourly fragments.
        /// \details Tick data is stored in hourly segments, replacing entire segments.
        /// \param symbol_id The symbol identifier.
        /// \param provider_id The provider identifier.
        /// \param ticks The tick data to insert.
        /// \throws MDBXException if storage fails.
		void upsert(
                uint16_t symbol_id,
                uint16_t provider_id,
                const std::vector<MarketTick>& ticks) {
            if (ticks.empty()) return;

            std::vector<std::vector<MarketTick>> out_segments;
			if (!split_ticks(ticks, out_segments)) throw MDBXException("Ticks are not in correct order.");

            MDBXTransaction transaction(m_connection.env_handle(), MDBX_TXN_READWRITE);

			TickCodecConfig codec_config;
			uint32_t metadata_key = generate_metadata_key(symbol_id, provider_id);
            auto it_metadata = m_metadata.find(metadata_key);
            if (it_metadata == m_metadata.end()) {
                TickMetadata metadata;
                if (!get_fixed_key32(transaction.handle(), m_dbi_metadata, metadata_key, metadata)) throw MDBXException("?");

                codec_config.volume_digits = metadata.volume_digits;
                codec_config.price_digits = metadata.price_digits;
                codec_config.flags = metadata.flags;

                m_metadata[metadata_key] = metadata;
                it_metadata = m_metadata.find(metadata_key);
            } else {
                codec_config.volume_digits = it_metadata->volume_digits;
                codec_config.price_digits = it_metadata->price_digits;
                codec_config.flags = it_metadata->flags;
            }

            const uint64_t start_ts = ticks.front().time_ms;
            const uint64_t end_ts   = ticks.back().time_ms;
            if (start_ts < it_metadata->start_ts ||
                end_ts > it_metadata->end_ts) {
                it_metadata->start_ts = start_ts;
                it_metadata->end_ts   = end_ts;
                put_fixed_key32(transaction.handle(), m_dbi_metadata, metadata_key, *it_metadata);
            }

            std::vector<uint8_t> output;
			for (const auto &segment : out_segments) {
                m_tick_serializer.serialize(segment, codec_config, output);
                const uint32_t unix_hour = time_shield::ms_to_hour(segment[0].time_ms);
                put_raw_key64(
                    transaction.handle(),
                    m_dbi_ticks,
                    generate_tick_key(symbol_id, provider_id, unix_hour),
                    output.data(), output.size(),
                    (out_segments.size() >= 2));
			}

			transaction.commit();
		}

		/// \brief Retrieves tick data.
        /// \param symbol_id The symbol identifier.
        /// \param provider_id The provider identifier.
        /// \param start_ts The start timestamp.
        /// \param end_ts The end timestamp.
        /// \param ticks The vector where the retrieved tick data will be appended.
        /// \param codec_config The encoding configuration.
        /// \return True if any ticks were found, otherwise false.
        /// \throws MDBXException if retrieval fails.
		bool fetch(
                uint16_t symbol_id,
                uint16_t provider_id,
                uint64_t start_ts,
                uint64_t end_ts,
                std::vector<MarketTick>& ticks,
                TickCodecConfig& codec_config) {
            uint64_t start_hour = time_shield::ms_to_hour(start_ts);
            uint64_t end_hour = time_shield::ms_to_hour(end_ts - 1);
            m_connection.renew();
            try {
                for (uint64_t unix_hour = start_hour; unix_hour <= end_hour; ++unix_hour) {
                    uint64_t key = generate_tick_key(symbol_id, provider_id, unix_hour);
                    m_buffer.clear();
                    if (!get_raw_key64(m_connection.rdonly_handle(), m_dbi_ticks, key, m_buffer)) continue;
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
		TickSerializer  m_tick_serializer;
		std::unordered_map<uint32_t, TickMetadata> m_metadata;
		std::vector<uint8_t> m_buffer;
		MDBXConnection& m_connection;
		MDBX_dbi m_dbi_ticks = 0;
		MDBX_dbi m_dbi_metadata = 0;

		/// \brief Generates a 64-bit key for tick storage.
        /// \param symbol_id The symbol identifier.
        /// \param provider_id The provider identifier.
        /// \param unix_hour The Unix timestamp rounded to the hour.
        /// \return A unique 64-bit key constructed as:
		///         [unix_hour (32 bits)] | [provider_id (16 bits)] | [symbol_id (16 bits)].
		uint64_t generate_tick_key(uint16_t symbol_id, uint16_t provider_id, uint32_t unix_hour) {
			return ((uint64_t)provider_id << 16) | (uint64_t)symbol_id | ((uint64_t)unix_hour << 32);
		}

		/// \brief Generates a 32-bit key for metadata storage.
        /// \param symbol_id The symbol identifier.
        /// \param provider_id The provider identifier.
        /// \return A unique 32-bit key constructed as:
		///         [provider_id (upper 16 bits)] | [symbol_id (lower 16 bits)].
		uint32_t generate_metadata_key(uint16_t symbol_id, uint16_t provider_id) {
			return ((uint32_t)provider_id << 16) | (uint32_t)symbol_id;
		}

		/// \brief Splits the tick data into hourly segments and ensures time order.
        /// \param ticks The full vector of tick data.
        /// \param out_segments A vector of vectors, where each sub-vector contains ticks for one hour.
        /// \return True if the ticks are ordered correctly, otherwise false.
        bool split_ticks(const std::vector<MarketTick>& ticks, std::vector<std::vector<MarketTick>>& out_segments) {
            uint64_t current_hour = time_shield::ms_to_hour(ticks[0].time_ms);
            std::vector<MarketTick> current_segment;

            current_segment.reserve(ticks.size());

            for (size_t i = 0; i < ticks.size(); ++i) {
                const MarketTick& tick = ticks[i];

                if (i > 0 && tick.time_ms < ticks[i - 1].time_ms) {
                    return false; // Ticks are not ordered
                }

                uint64_t tick_hour = time_shield::ms_to_hour(tick.time_ms);
                if (tick_hour != current_hour) {
                    // Store completed segment
                    out_segments.push_back(std::move(current_segment));
                    current_segment.clear();
                    current_segment.reserve(ticks.size() - i);
                    current_hour = tick_hour;
                }

                current_segment.push_back(tick);
            }

            if (!current_segment.empty()) {
                out_segments.push_back(std::move(current_segment));
            }

            return true;
        }
    };

};

#endif // _DFH_MDBX_TICK_BD_HPP_INCLUDED
