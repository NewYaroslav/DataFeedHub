#pragma once
#ifndef _DTH_IMARKETDATASOURCE_HPP_INCLUDED
#define _DTH_IMARKETDATASOURCE_HPP_INCLUDED

/// \file IMarketDataSource.hpp
/// \brief Interface for accessing market data sources.

namespace dfh::core {

    /// \brief Interface for market data sources.
    class IMarketDataSource {
    public:
        virtual ~IMarketDataSource() = default;

        /// \brief Returns the total number of available symbols across all data feeds.
        /// \details Symbols from different market types (e.g., spot and futures) are treated as the same asset.
        /// This means that the symbol count is independent of the number of supported market types.
        /// \return Total number of symbols.
        virtual size_t get_symbol_count() const = 0;

        /// \brief Returns the total number of market data providers.
        /// \details Different market types (e.g., spot and futures) on the same exchange are treated as separate providers.
        /// For example, Binance Spot and Binance Futures are counted as two different providers.
        /// \return Total number of providers.
        virtual size_t get_provider_count() const = 0;

        /// \brief Retrieves the bid/ask price restoration configuration for a specific symbol and provider.
        /// \param symbol_index Index of the symbol.
        /// \param provider_index Index of the market data provider.
        /// \return Reference to the `BidAskRestoreConfig` for the given symbol and provider.
        virtual const BidAskRestoreConfig& bidask_config(uint32_t symbol_index, uint32_t provider_index) const = 0;

        /// \brief Retrieves the bid/ask price restoration configuration by a unique index.
        /// \details The index is a unique identifier that represents a symbol-provider pair.
        /// \param index Unique identifier of the symbol-provider pair.
        /// \return Reference to the `BidAskRestoreConfig` for the specified index.
        virtual const BidAskRestoreConfig& bidask_config(uint32_t index) const = 0;

        /// \brief Fetches historical market tick data by a unique index.
        /// \details This method retrieves tick data from a database or other storage.
        /// Depending on the implementation, fetching may involve additional processing or decompression.
        /// \param index Unique identifier of the symbol-provider pair.
        /// \param start_time_ms Start time in milliseconds.
        /// \param end_time_ms End time in milliseconds.
        /// \param ticks Output vector to store the retrieved ticks.
        /// \param config Configuration for encoding/decoding tick data.
        /// \return True if ticks were successfully retrieved, false if no data is available.
        virtual bool fetch_ticks(
            uint32_t index,
            uint64_t start_time_ms,
            uint64_t end_time_ms,
            std::vector<MarketTick>& ticks,
            TickCodecConfig& config) = 0;

        /// \brief Fetches historical market tick data for a specific symbol and provider.
        /// \details This method retrieves tick data from a database or other storage.
        /// Depending on the implementation, fetching may involve additional processing or decompression.
        /// \param symbol_index Index of the symbol.
        /// \param provider_index Index of the market data provider.
        /// \param start_time_ms Start time in milliseconds.
        /// \param end_time_ms End time in milliseconds.
        /// \param ticks Output vector to store the retrieved ticks.
        /// \param config Configuration for encoding/decoding tick data.
        /// \return True if ticks were successfully retrieved, false if no data is available.
        virtual bool fetch_ticks(
            uint32_t symbol_index,
            uint32_t provider_index,
            uint64_t start_time_ms,
            uint64_t end_time_ms,
            std::vector<MarketTick>& ticks,
            TickCodecConfig& config) = 0;
    };

}; // namespace dfh::core

#endif // _DTH_IMARKETDATASOURCE_HPP_INCLUDED
