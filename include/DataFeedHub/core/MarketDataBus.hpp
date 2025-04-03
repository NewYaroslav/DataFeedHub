#pragma once
#ifndef _DTH_MARKET_DATA_BUS_HPP_INCLUDED
#define _DTH_MARKET_DATA_BUS_HPP_INCLUDED

/// \file MarketDataBus.hpp
/// \brief

namespace dfh::core {

    class MarketDataBus {
    public:

        MarketDataBus(IMarketDataSource* data_source)
            : m_data_source(data_source), m_buffers(data_source) {

        }

        ~MarketDataBus() {

        }

        int32_t register_subscription(MarketDataListener* listener) {
            auto it = m_sub_map_id.find(listener);
            if (it != m_sub_map_id.end()) return -1;

            if (!m_sub_data.empty()) {
                for (int32_t sub_id = 0; sub_id < m_sub_data.size(); ++sub_id) {
                    auto &sub_data = m_sub_data[sub_id];
                    if (sub_data.enabled) continue;
                    sub_data.reset();
                    sub_data.subs_ticks.resize(m_symbol_count * m_provider_count);
                    sub_data.listener = listener;
                    sub_data.enabled  = true;

                    init_timer();
                    return sub_id;
                }
            }

            int32_t sub_id = m_sub_last_id++;
            m_sub_map_id[listener] = sub_id;
            m_sub_data.resize(sub_id + 1);
            auto &sub_data = m_sub_data[sub_id];
            sub_data.subs_ticks.resize(m_symbol_count * m_provider_count);
            sub_data.listener = listener;
            sub_data.enabled  = true;

            init_timer();
            return sub_id;
        }

        bool unregister_subscription(MarketDataListener* listener) {
            auto it = m_sub_map_id.find(listener);
            if (it == m_sub_map_id.end()) return false;
            int32_t sub_id = it->second;
            m_sub_map_id.erase(listener);

            if (sub_id == (m_sub_data.size() - 1)) {
                m_sub_data.erase(m_sub_data.begin() + sub_id);
                m_sub_last_id = m_sub_data.size();

                init_timer();
                return true;
            }

            m_sub_data[sub_id].reset()
            m_sub_last_id = m_sub_data.size();

            init_timer();
            return true;
        }

        bool subscribe_timer(int32_t sub_id, uint32_t period_ms) {
            if (sub_id >= m_sub_data.size()) return false;
            if (period_ms == 0) return false;
            auto &sub_data = m_sub_data[sub_id];
            if (!sub_data.enabled) return false;
            sub_data.period_ms = period_ms;
            return true;
        }

        bool unsubscribe_timer(int32_t sub_id) {
            if (sub_id >= m_sub_data.size()) return false;
            auto &sub_data = m_sub_data[sub_id];
            if (!sub_data.enabled) return false;
            sub_data.period_ms = 0;
            return true;
        }

        bool subscribe_ticks(int32_t sub_id, uint32_t symbol_index, uint32_t provider_index) {
            if (sub_id >= m_sub_data.size()) return false;
            auto &sub_data = m_sub_data[sub_id];
            if (!sub_data.enabled) return false;
            sub_data.subs_ticks.set(get_index(symbol_index, provider_index));
            return true;
        }

        bool unsubscribe_ticks(int32_t sub_id, uint32_t symbol_index, uint32_t provider_index) {
            if (sub_id >= m_sub_data.size()) return false;
            auto &sub_data = m_sub_data[sub_id];
            if (!sub_data.enabled) return false;
            sub_data.subs_ticks.reset(get_index(symbol_index, provider_index));
            return true;
        }

        bool unsubscribe_ticks(int32_t sub_id) {
            if (sub_id >= m_sub_data.size()) return false;
            auto &sub_data = m_sub_data[sub_id];
            if (!sub_data.enabled) return false;
            sub_data.subs_ticks.reset();
            return true;
        }

        void start(uint64_t time_ms) {
            start_timer(time_ms);

            MarketSnapshot snapshot(&m_buffers);
            snapshot.time_ms = time_ms;
            snapshot.set_flag(EventType::START);
            for (const auto& sub_data : m_sub_data) {
                if (!sub_data.enabled) continue;
                sub_data.listener->on_update(snapshot);
            }
        }

        void update(uint64_t time_ms) {
            if (time_ms < m_update_time_ms[0]) return;
            // Первый случай - время прошло больше чем для одного обновления
            if (time_ms >= m_update_time_ms[1]) {
                m_subs_ticks.reset();
                m_listeners.clear();
                for (auto& sub_data : m_sub_data) {
                    if (time_ms < sub_data.update_time_ms) continue;
                    m_subs_ticks |= sub_data.subs_ticks;
                    m_listeners.push_back(sub_data.listener);
                }

                auto subs_ticks = m_subs_ticks.indices_of_set_bits();
                for (const auto data_index : subs_ticks) {
                    m_buffers.set_tick_span(data_index, m_last_time_ms, time_ms);
                }

                MarketSnapshot snapshot(&m_buffers);
                snapshot.time_ms = time_ms;
                snapshot.set_flag(EventType::TIMER_EVENT | EventType::TICK_UPDATE);
                for (const auto listener : m_listeners) {
                    listener->on_update(snapshot);
                }

                return;
            }

            // Второй случай - нужно только одно обновление
            auto& subs_ticks = m_timer_subs[m_timer_index].subs_ticks;
            for (const auto data_index : subs_ticks) {
                m_buffers.set_tick_span(data_index, m_last_time_ms, time_ms);
            }

            MarketSnapshot snapshot(&m_buffers);
            snapshot.time_ms = time_ms;
            snapshot.set_flag(EventType::TIMER_EVENT | EventType::TICK_UPDATE);
            auto& listeners = m_timer_subs[m_timer_index].listeners;
            for (const auto listener : listeners) {
                listener->on_update(snapshot);
            }

            ++m_timer_index;
            if (m_timer_index >= m_timer_subs.size()) {
                m_timer_index = 0;
                for (auto &timer_sub : m_timer_subs) {
                    timer_sub.update_time_ms += timer_sub.period_ms;
                }
            }

            m_update_time_ms[0] = m_update_time_ms[1];
            m_update_time_ms[1] = m_timer_subs[m_timer_index].update_time_ms;
            m_last_time_ms = time_ms;
        }

        // Событие по таймеру
        // 1. Выгружаем все данные по всем подпискам на данное время
        // 2. Вызываем всех подписчиков
        // 3. События по таймеру зависят от количества разных периодов
        // Есть два сценария:
        // 1. время увеличивается плавно, от одного варианта группировки подписчиков к следующему
        // 2. время перескакивает сразу несколько групп, тогда надо вызвать их всех обработчиик, т.е. тут используем аккумуляцию

    private:
        IMarketDataSource*   m_data_source;
        MarketDataBuffer     m_buffers;


        struct SubData {
            utils::DynamicBitset subs_ticks;
            MarketDataListener* listener = nullptr;
            uint64_t next_time_ms = 0;
            uint32_t period_ms    = 0;
            bool     enabled      = false;

            void reset() {
                subs_ticks.reset();
                listener = nullptr;
                next_time_ms = 0;
                period_ms    = 0;
                enabled      = false;
            }
        };

        std::unordered_map<MarketDataListener*, int32_t> m_sub_map_id;
        utils::DynamicBitset             m_subs_ticks;
        std::vector<MarketDataListener*> m_listeners;
        std::vector<SubData> m_sub_data;
        size_t               m_symbol_count   = 0;     ///< Количество активов
        size_t               m_provider_count = 0;     ///< Количество поставщиков
        uint64_t             m_update_time_ms[2] = {}; ///< Метка времени обновления данных
        uint64_t             m_last_time_ms = 0;
        int32_t              m_sub_last_id  = 0;       ///< Счётчик ID

        struct TimerSub {
            std::vector<MarketDataListener*> listeners;
            std::vector<size_t>              subs_ticks;
            uint64_t update_time_ms;
            uint32_t period_ms;
        };

        std::vector<TimerSub> m_timer_subs;   ///< Список таймеров по времени срабатывания для нормальной последовательности
        int32_t               m_timer_index = 0; ///< Индекс таймера m_timer_subs

        void init_timer(uint64_t time_ms) {
            m_timer_subs.clear();
            // Собираем уникальные периоды
            std::set<uint32_t> periods;
            for (size_t i = 0; i < m_sub_data.size(); ++i) {
                periods.insert(m_sub_data[i].period_ms);
            }
            // Заполянем данные таймеров
            m_timer_subs.resize(periods.size());
            size_t index = 0;
            for (const auto& period : periods) {
                m_timer_subs[index++].period_ms = period;
            }

            utils::DynamicBitset subs_ticks;
            subs_ticks.resize(m_symbol_count * m_provider_count);
            for (size_t i = 0; i < m_timer_subs.size(); ++i) {
                subs_ticks.reset();
                td::vector<MarketDataListener*> listeners;
                listeners.reserve(m_sub_data.size());
                for (size_t j = 0; j < m_sub_data.size(); ++j) {
                    if (m_timer_subs[i].period_ms != m_sub_data[j].period_ms) continue;
                    subs_ticks |= m_sub_data[j].subs_ticks;
                    listeners.push_back(m_sub_data[j].listener);
                }
                m_timer_subs[i].subs_ticks = subs_ticks.indices_of_set_bits();
                m_timer_subs[i].listeners  = std::move(listeners);
            }
            // Инициализируем время таймеров
            uint64_t update_time_ms = std::numeric_limits<uint64_t>::max();
            for (size_t i = 0; i < m_timer_subs.size(); ++i) {
                auto &timer_subs = m_timer_subs[i];
                timer_subs.update_time_ms = timer_subs.period_ms + time_shield::start_of_period(timer_subs.period_ms, time_ms);
                if (m_next_time_ms > timer_subs.update_time_ms) m_next_time_ms = timer_subs.update_time_ms;
            }
        }

        void start_timer(uint64_t time_ms) {
            // Находим первое время
            m_update_time_ms[0] = std::numeric_limits<uint64_t>::max();
            for (size_t i = 0; i < m_timer_subs.size(); ++i) {
                auto &timer_subs = m_timer_subs[i];
                timer_subs.update_time_ms = timer_subs.period_ms + time_shield::start_of_period(timer_subs.period_ms, time_ms);
                if (m_update_time_ms[0] > timer_subs.update_time_ms) m_update_time_ms[0] = timer_subs.update_time_ms;
            }
            // Находим следующее время обновления данных
            m_update_time_ms[1] = std::numeric_limits<uint64_t>::max();
            for (size_t i = 0; i < m_timer_subs.size(); ++i) {
                auto &timer_subs = m_timer_subs[i];
                if (m_update_time_ms[0] >= timer_subs.update_time_ms) continue;
                if (m_update_time_ms[1] > timer_subs.update_time_ms) m_update_time_ms[1] = timer_subs.update_time_ms;
            }
        }


        void publish(const MarketSnapshot& snapshot) {
            for (auto& sub_data : m_sub_data) {
                sub_data.listener->on_update(snapshot);
            }
        }













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
