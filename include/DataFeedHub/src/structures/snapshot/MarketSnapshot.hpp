#pragma once
#ifndef _DTH_SNAPSHOT_MARKET_SNAPSHOT_HPP_INCLUDED
#define _DTH_SNAPSHOT_MARKET_SNAPSHOT_HPP_INCLUDED

namespace dfh {

    /// \brief Класс для хранения среза состояния рынка на определённый момент времени.
    /// Структура отражает актуальное состояние данных на конкретный момент времени.
    /// MarketSnapshot выполняет роль обёртки, предоставляющей только чтение данных из буфера, запрещая прямой доступ к нему.
    class MarketSnapshot {
    public:

        MarketSnapshot(DataBuffer &buffer) : buffer(buffer) {};

        const FundingRateInfo& get_funding_rate_info(uint32_t symbol_index, uint32_t provider_index) const {
            return buffer.get_funding_info(symbol_index, provider_index);
        }

        const FundingRateInfo& get_funding_rate_info(const std::string& symbol, const std::string& provider) const {
            return buffer.get_funding_info(symbol_index, provider_index);
        }

        const std::vector<FundingRateInfo>& get_funding_rate_info() const {
            return buffer.get_funding_rate_info();
        }

        double get_funding_rate(uint32_t symbol_index, uint32_t provider_index) const {
            return buffer.get_funding_rate(symbol_index, provider_index, offset);
        }

        double get_mark_price(uint32_t symbol_index, uint32_t provider_index) const {
            return buffer.get_mark_price(symbol_index, provider_index, offset);
        }

        double get_funding_rate(const std::string& symbol, const std::string& provider) const {
            return buffer.get_funding_rate(symbol_index, provider_index, offset);
        }

        double get_mark_price(const std::string& symbol, const std::string& provider) const {
            return buffer.get_mark_price(symbol_index, provider_index, offset);
        }

        /// \brief ?
        /// \param symbol_index Index of the symbol
        /// \param provider_index Index of the data provider
        /// \return MarketTick
        const MarketTick& get_tick(uint32_t symbol_index, uint32_t provider_index, uint32_t offset = 0) const {
            return buffer.get_tick(symbol_index, provider_index, offset);
        }

        const size_t get_tick_count(uint32_t symbol_index, uint32_t provider_index) const {
            return buffer.get_tick_count(symbol_index, provider_index);
        }

        const std::vector<MarketTick> get_ticks(uint32_t symbol_index, uint32_t provider_index) const {
            return buffer.get_ticks(symbol_index, provider_index);
        }

        const MarketTick& get_tick(const std::string& symbol, const std::string& provider, uint32_t offset = 0) const {
            return buffer.get_tick(symbol, provider, offset);
        }

        const size_t get_tick_count(const std::string& symbol, const std::string& provider) const {
            return buffer.get_tick_count(symbol, provider);
        }

        const std::vector<MarketTick> get_ticks(const std::string& symbol, const std::string& provider) const {
            return buffer.get_ticks(symbol, provider);
        }

        uint32_t get_symbol_index(const std::string& name) const {
            return buffer.get_symbol_index(name);
        }

        uint32_t get_provider_index(const std::string& name) const {
            return buffer.get_provider_index(name);
        }

        const std::string& get_symbol_name(uint32_t symbol_index) const {
            return buffer.get_symbol(symbol_index);
        }

        const std::string& get_provider_name(uint32_t provider_index) const {
            return buffer.get_provider_name(provider_index);
        }

        const uint64_t get_time_ms() const {
            return buffer.get_time_ms();
        }

        bool has_flag(SnapshotUpdateFlags flag) const {
            return buffer.has_flag(flag);
        }

    private:
        DataBuffer &buffer;
    };

};

#endif // _DTH_SNAPSHOT_MARKET_SNAPSHOT_HPP_INCLUDED
