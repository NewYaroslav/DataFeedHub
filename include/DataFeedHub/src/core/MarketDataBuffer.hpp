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

        void set_tick_data_range(
                uint32_t symbol_index,
                uint32_t provider_index,
                uint64_t time_ms,
                uint64_t period_ms) {
            m_tick_buffers[get_index(symbol_index, provider_index)].
        }

        const MarketTick& get_tick(
                uint32_t symbol_index,
                uint32_t provider_index,
                uint32_t offset = 0) const {
            m_tick_buffers[get_index(symbol_index, provider_index)]
        }

        const size_t get_tick_count(
                uint32_t symbol_index,
                uint32_t provider_index) const {
            return
        }

        const std::vector<MarketTick> get_ticks(
                uint32_t symbol_index,
                uint32_t provider_index) const {
            return
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


    private:
        std::vector<StreamTickBuffer> m_tick_buffers;
        std::list<Subscription> m_subscriptions;
        size_t m_symbol_count = 0;
        size_t m_provider_count = 0;
        size_t m_data_count = 0;
        uint64_t m_time_ms = 0;

        inline const size_t get_index(
                size_t symbol_index,
                size_t provider_index) const {
            return provider_index * m_symbol_count + symbol_index;
        }
    };

};

#endif // _DTH_MARKET_DATA_BUFFER_HPP_INCLUDED
