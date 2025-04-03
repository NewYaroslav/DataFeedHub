#pragma once
#ifndef _DTH_MARKET_DATA_BUFFER_HPP_INCLUDED
#define _DTH_MARKET_DATA_BUFFER_HPP_INCLUDED

/// \file MarketDataBuffer.hpp
/// \brief

namespace dfh::core {

    enum class EventType : uint64_t {
        NONE           = 0,
        TICK_UPDATE    = 1 << 0, // Обновление тиков
        BAR_UPDATE     = 1 << 1, // Обновление бара
        FUNDING_UPDATE = 1 << 2, // Обновление фандинга
        TIMER_EVENT    = 1 << 3, // Событие таймера
        TEST_START     = 1 << 4, // Начало тестирования
        TEST_END       = 1 << 5, // Завершение тестирования
        REALTIME_START = 1 << 6, // Начало работы в реал-тайме
        REALTIME_STOP  = 1 << 7  // Завершение работы в реал-тайме
    };

    /// \brief Структура для хранения информации о подписке
    struct Subscription {
        void* data_ptr;             ///< Указатель на данные
        uint32_t count;             ///< Счетчик подписок
        uint32_t event_flags;       ///< Флаги событий

        Subscription(void* data, std::bitset<8> flags)
            : subscription_count(1), data_ptr(data), event_flags(flags) {}
    };

    class MarketDataBuffer {
    public:

        MarketDataBuffer(IMarketDataSource* data_source)
                : m_data_source(data_source) {
            m_symbol_count   = data_source->get_symbol_count();
            m_provider_count = data_source->get_provider_count();
            m_tick_buffers.resize(m_symbol_count * m_provider_count);
            for (size_t i = 0; i < m_tick_buffers.size(); ++i) {
                m_tick_buffers[i].set_bidask_config(data_source->bidask_config(i));
            }
        }

        void set_tick_span(size_t data_index, uint64_t start_time_ms, uint64_t end_time_ms) {
            m_tick_buffers[data_index].set_tick_span(start_time_ms, end_time_ms);
        }

        void set_tick_data_range(
                uint32_t symbol_index,
                uint32_t provider_index,
                uint64_t time_ms,
                uint64_t period_ms) {
            m_tick_buffers[get_index(symbol_index, provider_index)].
        }

        void set_tick_data_range(
                uint32_t data_index,
                uint64_t time_ms,
                uint64_t period_ms) {
            m_tick_buffers[data_index].
        }

        const MarketTick& get_tick(
                uint32_t symbol_index,
                uint32_t provider_index,
                uint32_t offset = 0) const {
            m_tick_buffers[get_index(symbol_index, provider_index)]
        }


        void update(uint64_t time_ms) {
            auto it = m_subscriptions.begin();
            while (it != m_subscriptions.end()) {
                if (!it->count) {
                    it = m_subscriptions.erase(it);
                    continue;
                }

            }
        }

        void fetch_ticks(uint32_t index, uint64_t time_ms) {
            m_tick_buffers[index].fetch_ticks(index, time_ms, m_data_source);
        }


    private:
        IMarketDataSource* m_data_source = nullptr;
        std::vector<StreamTickBuffer> m_tick_buffers;
        std::list<Subscription> m_subscriptions;
        size_t m_symbol_count = 0;      ///< Количество символов (для спота и фьючерсов номер символа совпадает)
        size_t m_provider_count = 0;    ///< Количество провайдеров данных (важно: одна и та же биржа может иметь несколько провайдеров, обычно это спотовый рынок и фьчерсный).
        size_t m_data_count = 0;
        uint64_t m_time_ms = 0;

        inline const size_t get_index(
                size_t symbol_index,
                size_t provider_index) const noexcept {
            return provider_index * m_symbol_count + symbol_index;
        }
    };

};

#endif // _DTH_MARKET_DATA_BUFFER_HPP_INCLUDED
