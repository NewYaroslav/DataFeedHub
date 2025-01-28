#pragma once
#ifndef _DTH_MARKET_DATA_BUS_HPP_INCLUDED
#define _DTH_MARKET_DATA_BUS_HPP_INCLUDED

/// \file MarketDataBus.hpp
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

        START          = 1 << 4,     // Начало работы (тестирования или реальной торговли)
        STOP           = 1 << 5,     // Завершение работы (тестирования или реальной торговли)
        REALTIME_MODE  = 1 << 6      // Флаг, указывающий на режим реальной торговли
    };

    struct TimerSub {
        uint32_t period_ms;
        uint64_t time_ms;
        uint64_t subs;

        bool operator<(const TimerSub& other) const {
            return period_ms < other.period_ms;
        }

        std::vector<uint32_t> tick_subs;
    };

    struct SubscriptionData {
        uint32_t period_ms;
        bool enable_timer;

    };

    struct SubData {
        MarketDataListener* listener;
        utils::DynamicBitset subs_ticks;
    };

    struct TickSub {
        uint32_t symbol_index;
        uint32_t provider_index;
        uint64_t subscribers;


        provider_index * m_symbol_count + symbol_index;
    };

    class MarketDataBus {
    public:

        MarketDataBus() {
            const size_t max_id = 64;
            m_sub_data.resize(max_id, 0);
        }

        bool register_subscription(MarketDataListener* listener) {
            const size_t max_id = 64;
            if (m_sub_last_id >= max_id) return false;
            m_sub_id[listener] = m_sub_last_id;
            SubData& sub = m_sub_data[m_sub_last_id++];
            sub.listener = listener;
            sub.subs_ticks.reset();
            return true;
        }

        bool unregister_subscription(MarketDataListener* listener) {
            auto it = m_sub_id.find(listener);
            if (it == m_sub_id.end()) return false;
            SubData& sub = m_sub_data[it->second];
            sub.listener = nullptr;
            sub.subs_ticks.reset();
            for (size_t i = it->second + 1; i < m_sub_data.size(); ++i) {
                SubData& sub = m_sub_data[i];
                if (!sub.listener) {
                    m_sub_last_id = i;
                    break;
                }
                SubData& prev_sub = m_sub_data[i - 1];
                prev_sub.listener = sub.listener;
                prev_sub.subs_ticks = std::move(sub);
                sub.listener = nullptr;
                sub.subs_ticks.clear();
            }
            return true;
        }

        bool subscribe_timer(MarketDataListener* listener, uint32_t period_ms) {
            const size_t max_id = 64;
            if (subscriber_id >= max_id) return false;

            if (m_timer_subscriptions.empty()) {
                TimerSubscription sub;
                sub.period_ms = period_ms;
                sub.time_ms = 0;
                sub.subscribers |= (1ULL << subscriber_id);
                m_timer_subscriptions.push_back(sub);
                return true;
            }

            auto it = std::lower_bound(
                    m_timer_subscriptions.begin(),
                    m_timer_subscriptions.end(),
                    period_ms,
                    [](const TimerSubscription& sub, uint32_t target) {
                return sub.period_ms < target;
            });

            if (it != m_timer_subscriptions.end() &&
                it->period_ms == period_ms) {
                it->subscribers |= (1ULL << subscriber_id);
            } else {
                TimerSubscription sub;
                sub.period_ms = period_ms;
                sub.time_ms = 0;
                sub.subscribers |= (1ULL << subscriber_id);
                m_timer_subscriptions.insert(it, sub);
            }
            return true;
        }

        bool unsubscribe_timer(uint32_t period_ms, size_t subscriber_id) {

        }

        bool subscribe_ticks(uint32_t symbol_index, uint32_t provider_index, size_t subscriber_id) {
            const size_t max_id = 64;
            if (subscriber_id >= max_id) return false;
            SubData& sub = m_subscribers[subscriber_id];

            const size_t index = get_index(symbol_index, provider_index);
            if (index >= sub.subs_ticks.size()) {
                sub.subs_ticks.resize(index);
            }
            sub.subs_ticks.set(index);
        }

        bool unsubscribe_ticks(uint32_t symbol_index, uint32_t provider_index, size_t subscriber_id) {
            const size_t max_id = 64;
            if (subscriber_id >= max_id) return false;
            SubData& sub = m_subscribers[subscriber_id];

            const size_t index = get_index(symbol_index, provider_index);
            if (index >= sub.subs_ticks.size()) {
                sub.subs_ticks.resize(index);
            }
            sub.subs_ticks.reset(index);
        }

        void publish(const MarketSnapshot& snapshot) {
            for (auto& callback : subscribers[data_type]) {
                callback(data);
            }
        }

        void start(uint64_t time_ms) {
            m_next_time_ms = std::numeric_limits<uint64_t>::max();
            for (size_t i = 0; i < m_timer_subscriptions.size(); ++i) {
                auto& sub = m_timer_subscriptions[i];
                sub.time_ms = sub.period_ms + time_shield::start_of_period(sub.period_ms, time_ms);
                if (m_next_time_ms > sub.time_ms) m_next_time_ms = sub.time_ms;
            }

            MarketSnapshot snapshot(this);
            snapshot.time_ms = time_ms;
            snapshot.set_flag(EventType::START);
            for (size_t i = 0; i < m_subscribers.size(); ++i) {
                m_subscribers[i]->on_data(snapshot);
            }
        }

        void stop(uint64_t time_ms) {

        }

        void update(uint64_t time_ms) {
            if (time_ms < m_next_time_ms) return;

            // Проверяем необходимость обновить буферы тиков
            if (m_buffers.is_tick_buffer_expired(time_ms)) {
                const uint64_t last_hour = m_next_time_ms / time_shield::MS_PER_HOUR;
                const uint64_t new_hour  = time_ms / time_shield::MS_PER_HOUR;
                if (last_hour != new_hour) {


                }


                utils::DynamicBitset subs_ticks;
                // Собираем информацию о подписках
                for (size_t i = 0; i < m_sub_last_id; ++i) {
                    if (m_sub_data[i].subs_ticks.size() > subs_ticks.size()) {
                        subs_ticks.resize(m_sub_data[i].subs_ticks.size());
                    }
                    subs_ticks |= m_sub_data[i].subs_ticks;
                }
                // Загружаем данные
                for (size_t i = 0; i < subs_ticks.size(); ++i) {
                    if (!subs_ticks.is_set(i)) continue;
                    TickSub& sub = m_tick_subs[i];
                    m_buffers.load_ticks(
                        sub.symbol_index,
                        sub.provider_index,
                        time_ms,
                        [](uint64_t key_db,
                           std::vector<MarketTick>& ticks,
                           TickEncodingConfig& tick_config) {

                    });
                }
            }

            // Обрабатываем подписки по таймеру
            for (size_t i = 0; i < m_timer_subs.size(); ++i) {
                auto& sub = m_timer_subs[i];
                if (time_ms < sub.next_time_ms) continue;

                // Устанавливаем данные буфера тиков
                for (size_t i = 0; i < sub.tick_subs.size(); ++i) {
                    uint32_t symbol_index = sub.tick_subs[i] & 0xFFFF;
                    uint32_t provider_index = (sub.tick_subs[i] >> 16) & 0xFFFF;
                    m_buffers.set_ticks_in_range(symbol_index, provider_index, sub.time_ms, time_ms);
                }

                // Устанавливаем данные буфера фандинга
                for (size_t i = 0; i < sub.funding_subs.size(); ++i) {
                    uint32_t symbol_index = sub.funding_subs[i] & 0xFFFF;
                    uint32_t provider_index = (sub.funding_subs[i] >> 16) & 0xFFFF;
                    m_buffers.set_klines(symbol_index, provider_index, time_ms);
                }

                // Устанавливаем данные буфера фандинга
                for (size_t i = 0; i < sub.funding_subs.size(); ++i) {
                    uint32_t symbol_index = sub.funding_subs[i] & 0xFFFF;
                    uint32_t provider_index = (sub.funding_subs[i] >> 16) & 0xFFFF;
                    m_buffers.set_funding(symbol_index, provider_index, time_ms);
                }

                // Устанавливаем данные буфера mark price
                for (size_t i = 0; i < sub.mark_price_subs.size(); ++i) {
                    uint32_t symbol_index = sub.mark_price_subs[i] & 0xFFFF;
                    uint32_t provider_index = (sub.mark_price_subs[i] >> 16) & 0xFFFF;
                    m_buffers.set_mark_price_klines(symbol_index, provider_index, time_ms);
                }

                // Устанавливаем данные буфера premium index
                for (size_t i = 0; i < sub.premium_index_subs.size(); ++i) {
                    uint32_t symbol_index = sub.premium_index_subs[i] & 0xFFFF;
                    uint32_t provider_index = (sub.premium_index_subs[i] >> 16) & 0xFFFF;
                    m_buffers.set_premium_index_klines(symbol_index, provider_index, time_ms);
                }

                // Вызываем события для подписчиков

            }

            // Устанавливаем тики за прошедший период






            provider_index * m_symbol_count + symbol_index;

            m_buffers.get_ticks_in_range(
                    sub.symbol_index,
                    sub.provider_index,
                    time_ms,
                    [](uint64_t key_db,
                       std::vector<MarketTick>& ticks,
                       TickEncodingConfig& tick_config) {

                });


            // Выгружаем тики за период



            // Обновляем данные тиков
            for (size_t i = 0; i < m_ticks_subscriptions.size(); ++i) {
                if ()
            }

            // Находим следующее время
            m_next_time_ms = std::numeric_limits<uint64_t>::max();
            uint64_t subscribers = 0;
            for (size_t i = 0; i < m_timer_subscriptions.size(); ++i) {
                auto& sub = m_timer_subscriptions[i];
                if (time_ms >= sub.time_ms) {
                    subscribers |= sub.subscribers;
                }
                sub.time_ms = sub.period_ms + time_shield::start_of_period(sub.period_ms, time_ms);
                if (m_next_time_ms > sub.time_ms) m_next_time_ms = sub.time_ms;
            }
        }

        uint64_t get_next_time_ms() const {
            return m_next_time_ms;
        }

    private:
        MarketDataBuffer m_buffers;
        std::vector<uint32_t> m_merged_tick_subs;

        std::unordered_map<size_t, TickSubscription> m_tick_subscriptions;

        size_t m_sub_last_id = 0;
        std::unordered_map<MarketDataListener*, size_t> m_sub_id;
        std::vector<SubData>    m_sub_data;
        std::vector<TimerSub>   m_timer_subs;
        std::vector<TickSub>    m_tick_subs;
        uint64_t m_next_time_ms = 0;
        size_t   m_symbol_count = 0;

        inline const size_t get_index(
                size_t symbol_index,
                size_t provider_index) const {
            return provider_index * m_symbol_count + symbol_index;
        }
    };

};

#endif // _DTH_MARKET_DATA_BUS_HPP_INCLUDED
