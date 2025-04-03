#pragma once
#ifndef _DTH_STORAGE_STORAGE_METADATA_HPP_INCLUDED
#define _DTH_STORAGE_STORAGE_METADATA_HPP_INCLUDED

/// \file StorageMetadata.hpp
/// \brief Metadata describing what data a storage backend holds.

namespace dfh::storage {

    /// \struct StorageMetadata
    /// \brief Metadata describing the types of data stored in a storage backend.
    ///
    /// This metadata includes supported market types, exchanges, symbols,
    /// and time range for which data is available. It is used to route read/write
    /// operations to the appropriate storage backend.
    struct StorageMetadata {
    public:
        StorageDataFlags data_flags = StorageDataFlags::NONE; ///< Bitmask of stored data types.

    private:
        uint64_t                m_start_time_ms = 0;          ///< Start time of data range.
        uint64_t                m_end_time_ms = 0;            ///< End time of data range.
        std::vector<dfh::MarketType> m_market_types;          ///< Supported market types (must be sorted for binary search).
        std::vector<uint16_t>   m_exchange_ids;               ///< Supported exchanges (must be sorted for binary search).
        std::vector<uint16_t>   m_symbol_ids;                 ///< Supported symbols (must be sorted for binary search).

    public:

        /// \brief Sets the aligned time range based on the given timeframe.
        /// \param start_ms Start time in milliseconds.
        /// \param end_ms End time in milliseconds.
        /// \param tf Timeframe to determine the alignment granularity.
        /// \throws dfh::storage::StorageException If timeframe is unknown or end < start.
        void set_time_range(uint64_t start_ms, uint64_t end_ms, dfh::TimeFrame tf) {
            if (end_ms < start_ms) throw StorageException("StorageMetadata: end time must be greater than or equal to start time");

            const uint64_t duration_ms = (tf == dfh::TimeFrame::UNKNOWN)
                ? 3600000ULL  // 1 hour
                : get_segment_duration_ms(tf);

            m_start_time_ms = start_ms - (start_ms % duration_ms);
            m_end_time_ms   = end_ms - (end_ms % duration_ms) + duration_ms;
        }

        /// \brief Returns the aligned start time in milliseconds.
        /// \return start time in milliseconds.
        uint64_t start_time_ms() const noexcept { return m_start_time_ms; }

        /// \brief Returns the aligned end time in milliseconds.
        /// \return end time in milliseconds.
        uint64_t end_time_ms() const noexcept { return m_end_time_ms; }

        /// \brief Adds a market type to the metadata, maintaining sorted order and uniqueness.
        /// \param market_type Market type to add.
        void add_market_type(dfh::MarketType market_type) {
            auto it = std::lower_bound(m_market_types.begin(), m_market_types.end(), market_type);
            if (it == m_market_types.end() || *it != market_type)
                m_market_types.insert(it, market_type);
        }

        /// \brief Removes a market type from the metadata if it exists.
        /// \param market_type Market type to remove.
        void remove_market_type(dfh::MarketType market_type) {
            auto it = std::lower_bound(m_market_types.begin(), m_market_types.end(), market_type);
            if (it != m_market_types.end() && *it == market_type)
                m_market_types.erase(it);
        }

        /// \brief Adds an exchange ID to the metadata, maintaining sorted order and uniqueness.
        /// \param exchange_id Exchange identifier to add.
        void add_exchange_id(uint16_t exchange_id) {
            auto it = std::lower_bound(m_exchange_ids.begin(), m_exchange_ids.end(), exchange_id);
            if (it == m_exchange_ids.end() || *it != exchange_id)
                m_exchange_ids.insert(it, exchange_id);
        }

        /// \brief Removes an exchange ID from the metadata if it exists.
        /// \param exchange_id Exchange identifier to remove.
        void remove_exchange_id(uint16_t exchange_id) {
            auto it = std::lower_bound(m_exchange_ids.begin(), m_exchange_ids.end(), exchange_id);
            if (it != m_exchange_ids.end() && *it == exchange_id)
                m_exchange_ids.erase(it);
        }

        /// \brief Adds a symbol ID to the metadata, maintaining sorted order and uniqueness.
        /// \param symbol_id Symbol identifier to add.
        void add_symbol_id(uint16_t symbol_id) {
            auto it = std::lower_bound(m_symbol_ids.begin(), m_symbol_ids.end(), symbol_id);
            if (it == m_symbol_ids.end() || *it != symbol_id)
                m_symbol_ids.insert(it, symbol_id);
        }

        /// \brief Removes a symbol ID from the metadata if it exists.
        /// \param symbol_id Symbol identifier to remove.
        void remove_symbol_id(uint16_t symbol_id) {
            auto it = std::lower_bound(m_symbol_ids.begin(), m_symbol_ids.end(), symbol_id);
            if (it != m_symbol_ids.end() && *it == symbol_id)
                m_symbol_ids.erase(it);
        }

        /// \brief Returns the list of market types.
        /// \return Sorted vector of supported market types.
        const std::vector<dfh::MarketType>& market_types() const noexcept { return m_market_types; }

        /// \brief Returns the list of exchange IDs.
        /// \return Sorted vector of supported exchange identifiers.
        const std::vector<uint16_t>& exchange_ids() const noexcept { return m_exchange_ids; }

        /// \brief Returns the list of symbol IDs.
        /// \return Sorted vector of supported symbol identifiers.
        const std::vector<uint16_t>& symbol_ids() const noexcept { return m_symbol_ids; }

        /// \brief Checks if the metadata includes the specified symbol.
        /// \param symbol_id Symbol identifier to check.
        /// \return True if symbol is present, false otherwise.
        bool has_symbol(uint16_t symbol_id) const {
            return std::binary_search(m_symbol_ids.begin(), m_symbol_ids.end(), symbol_id);
        }

        /// \brief Checks if the metadata includes the specified exchange.
        /// \param exchange_id Exchange identifier to check.
        /// \return True if exchange is present, false otherwise.
        bool has_exchange(uint16_t exchange_id) const {
            return std::binary_search(m_exchange_ids.begin(), m_exchange_ids.end(), exchange_id);
        }

        /// \brief Checks if the metadata includes the specified market type.
        /// \param _market_type Market type to check.
        /// \return True if type is present, false otherwise.
        bool has_market_type(dfh::MarketType _market_type) const {
            return std::binary_search(m_market_types.begin(), m_market_types.end(), _market_type);
        }

        /// \brief Checks if a specific data flag is present.
        /// \param flag Flag to check.
        /// \return True if flag is set, false otherwise.
        bool has_flag(StorageDataFlags flag) const {
            return (data_flags & flag) != StorageDataFlags::NONE;
        }

        /// \brief Checks if the given timestamp falls within the metadata time range.
        /// \param timestamp_ms Timestamp in milliseconds.
        /// \return True if within range, false otherwise.
        bool contains_time(uint64_t timestamp_ms) const {
            return (timestamp_ms >= m_start_time_ms || m_start_time_ms == 0) &&
                (timestamp_ms < m_end_time_ms || m_end_time_ms == 0);
        }

        /// \brief Merges the contents of another StorageMetadata into this one.
        /// \param other Metadata to merge from.
        void merge_with(const StorageMetadata& other) {
            data_flags |= other.data_flags;

            if (m_start_time_ms == 0 ||
                (other.m_start_time_ms != 0 && other.m_start_time_ms < m_start_time_ms)) {
                m_start_time_ms = other.m_start_time_ms;
            }

            if (m_end_time_ms == 0 ||
                (other.m_end_time_ms != 0 && other.m_end_time_ms > m_end_time_ms)) {
                m_end_time_ms = other.m_end_time_ms;
            }

            for (auto market_type : other.m_market_types) add_market_type(market_type);
            for (auto exchange_id : other.m_exchange_ids) add_exchange_id(exchange_id);
            for (auto symbol_id : other.m_symbol_ids) add_symbol_id(symbol_id);
        }

        /// \brief Subtracts the contents of another StorageMetadata from this one.
        /// \param other Metadata to subtract.
        void subtract(const StorageMetadata& other) {
            data_flags &= ~other.data_flags;

            if (other.m_start_time_ms > m_start_time_ms) m_start_time_ms = other.m_start_time_ms;
            if (other.m_end_time_ms < m_end_time_ms) m_end_time_ms = other.m_end_time_ms;

            remove_items(m_market_types, other.m_market_types);
            remove_items(m_exchange_ids, other.m_exchange_ids);
            remove_items(m_symbol_ids, other.m_symbol_ids);
        }

        /// \brief Serializes this metadata into a binary vector.
        /// \return Serialized binary data.
        std::vector<uint8_t> serialize() const {
            std::vector<uint8_t> data;
            append_to_vector(data, static_cast<uint32_t>(data_flags));
            append_to_vector(data, m_start_time_ms);
            append_to_vector(data, m_end_time_ms);

            write_vector(data, m_symbol_ids);
            write_vector(data, m_exchange_ids);
            write_vector(data, m_market_types);
            return data;
        }

        /// \brief Deserializes metadata from binary data.
        /// \param data Pointer to binary data.
        /// \param size Size of binary data in bytes.
        /// \throws dfh::storage::StorageException If data is invalid.
        void deserialize(const uint8_t* data, size_t size) {
            size_t offset = 0;
            uint32_t flags = 0;
            read_from_data(data, size, offset, flags);
            data_flags = static_cast<StorageDataFlags>(flags);

            read_from_data(data, size, offset, m_start_time_ms);
            read_from_data(data, size, offset, m_end_time_ms);

            read_vector(data, size, offset, m_symbol_ids);
            read_vector(data, size, offset, m_exchange_ids);
            read_vector(data, size, offset, m_market_types);
        }

    private:

        template <typename T>
        void append_to_vector(std::vector<uint8_t>& out, const T& value) const {
            const uint8_t* src = reinterpret_cast<const uint8_t*>(&value);
            out.insert(out.end(), src, src + sizeof(T));
        }

        template <typename T>
        void read_from_data(const uint8_t* data, size_t size, size_t& offset, T& value) const {
            if (offset + sizeof(T) > size) {
                throw StorageException("StorageMetadata: buffer overflow while reading");
            }
            std::memcpy(&value, data + offset, sizeof(T));
            offset += sizeof(T);
        }

        template <typename T>
        static void write_vector(std::vector<uint8_t>& out, const std::vector<T>& vec) {
            uint32_t count = static_cast<uint32_t>(vec.size());
            out.insert(out.end(), reinterpret_cast<uint8_t*>(&count), reinterpret_cast<uint8_t*>(&count) + sizeof(count));
            if (!vec.empty()) {
                const uint8_t* raw = reinterpret_cast<const uint8_t*>(vec.data());
                out.insert(out.end(), raw, raw + sizeof(T) * vec.size());
            }
        }

        template <typename T>
        static void read_vector(const uint8_t* data, size_t size, size_t& offset, std::vector<T>& out_vec) {
            uint32_t count = 0;
            if (offset + sizeof(count) > size)
                throw StorageException("StorageMetadata: buffer overflow while reading vector size");
            std::memcpy(&count, data + offset, sizeof(count));
            offset += sizeof(count);

            if (offset + sizeof(T) * count > size)
                throw StorageException("StorageMetadata: buffer overflow while reading vector data");

            out_vec.resize(count);
            std::memcpy(out_vec.data(), data + offset, sizeof(T) * count);
            offset += sizeof(T) * count;
        }

        /// \brief Removes elements from `target` that are present in `to_remove`.
        /// \tparam T Element type (e.g., uint16_t, MarketType).
        /// \param target Sorted vector from which elements will be removed.
        /// \param to_remove Sorted vector of elements to remove.
        template <typename T>
        void remove_items(std::vector<T>& target, const std::vector<T>& to_remove) {
            std::vector<T> result;
            std::set_difference(
                target.begin(), target.end(),
                to_remove.begin(), to_remove.end(),
                std::back_inserter(result)
            );
            target = std::move(result);
        }
    };

} // namespace dfh::storage

#endif // _DTH_STORAGE_STORAGE_METADATA_HPP_INCLUDED
