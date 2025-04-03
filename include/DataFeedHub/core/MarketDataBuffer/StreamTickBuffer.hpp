#pragma once
#ifndef _DFH_STREAM_TICK_BUFFER_HPP_INCLUDED
#define _DFH_STREAM_TICK_BUFFER_HPP_INCLUDED

/// \file StreamTickBuffer.hpp
/// \brief Буфер для потоковой обработки тиковых данных с восстановлением bid/ask

#include <vector>
#include <stdexcept>
#include <cstdint>
#include "ISpreadProcessor.hpp"
#include "NoneSpreadProcessor.hpp"
#include "FixedSpreadProcessor.hpp"
#include "DynamicSpreadProcessor.hpp"
#include "MedianSpreadProcessor.hpp"

namespace dfh::core {

    /// \class StreamTickBuffer
    /// \brief Manages a tick data buffer with bid/ask price restoration.
    ///
    /// This class handles historical tick data and reconstructs missing bid/ask prices
    /// using different spread estimation models (fixed, dynamic, median).
    class StreamTickBuffer {
    public:

        /// \brief Default constructor.
        StreamTickBuffer() {
            m_chunks.resize(time_shield::SEC_PER_HOUR + 1);
            m_chunks[0] = 0;
        }

        /// \brief Sets the bid/ask restoration configuration.
        /// \param config Configuration for bid/ask price reconstruction.
        void set_bidask_config(const BidAskRestoreConfig& config) {
            m_bidask_config = config;

            switch (m_bidask_config.mode) {
            case BidAskModel::NONE:
                m_spread_processor = &m_none_processor;
                break;
            case BidAskModel::FIXED_SPREAD:
                m_spread_processor = &m_fixed_processor;
                break;
            case BidAskModel::DYNAMIC_SPREAD:
                m_spread_processor = &m_dynamic_processor;
                break;
            case BidAskModel::MEDIAN_SPREAD:
                m_spread_processor = &m_median_processor;
                break;
            default:
                throw std::invalid_argument("Unknown bid/ask restoration model");
                break;
            };
        }

        /// \brief Returns the current bid/ask restoration configuration.
        /// \return Reference to the current `BidAskRestoreConfig`.
        const BidAskRestoreConfig& bidask_config() const {
            return m_bidask_config;
        }

        /// \brief Returns the tick encoding configuration.
        /// \return Reference to the `TickCodecConfig`.
        const TickCodecConfig& codec_config() const {
            return m_codec_config;
        }

        /// \brief Returns the current tick span.
        /// \return Constant reference to `MarketTickSpan`.
        const MarketTickSpan& get_tick_span() const {
            return m_tick_span;
        }

        /// \brief Retrieves the latest tick from the buffer.
        /// \return Pointer to the last tick in the buffer, or `nullptr` if empty.
        const MarketTick* get_latest_tick() const {
            return m_tick_span.size ? &m_tick_span.data[m_tick_span.size-1] : nullptr;
        }

        /// \brief Sets the time range for tick retrieval.
        /// \param start_time_ms Start time in milliseconds.
        /// \param end_time_ms End time in milliseconds.
        void set_tick_span(uint64_t start_time_ms, uint64_t end_time_ms) {
            m_tick_span.size = 0;
            m_tick_span.data = nullptr;
            if (m_ticks.empty()) return;

            constexpr uint64_t time_offset = time_shield::MS_PER_SEC - 1;
            const size_t start_pos = m_chunks[time_shield::ms_to_sec(start_time_ms - m_start_time_ms)];
            const size_t end_pos   = m_chunks[time_shield::ms_to_sec(end_time_ms - m_start_time_ms + time_offset)];

            for (size_t i = start_pos; i <= end_pos; ++i) {
                if (start_time_ms > m_ticks[i].time_ms) continue;
                m_tick_span.data = &m_ticks[i];
                m_tick_span.size = i;
                break;
            }

            if (m_tick_span.data == nullptr) return;

            for (std::ptrdiff_t i = end_pos; i >= start_pos; --i) {
                if (end_time_ms <= m_ticks[i].time_ms) continue;
                m_tick_span.size = i - m_tick_span.size + 1;
                return;
            }
        }

        /// \brief Loads tick data from a data source.
        /// \param index Unique identifier for the symbol-provider pair.
        /// \param time_ms Timestamp for the requested tick data.
        /// \param data_source Pointer to a market data source.
        void fetch_ticks(uint32_t index, uint64_t time_ms, IMarketDataSource* data_source) {
            uint64_t start_time_ms = time_shield::start_of_hour_ms(time_ms);
            if (start_time_ms == m_end_time_ms && m_has_prev_data) {
                m_ticks.clear();
                data_source->fetch_ticks(index, start_time_ms, start_time_ms + time_shield::MS_PER_HOUR, m_ticks, m_codec_config);
            } else {
                // Есть разрыв последовательности, перезагружаем недостающие данные
                reload_ticks(index, start_time_ms, data_source);
            }

            m_start_time_ms = start_time_ms;
            m_end_time_ms   = start_time_ms + time_shield::MS_PER_HOUR;

            if (m_ticks.empty()) {
                std::fill(m_chunks.begin(), m_chunks.end(), 0UL);
                m_has_prev_data = false;
                return;
            }

            m_spread_processor->process(
                m_ticks, m_chunks,
                m_prev_tick, m_has_prev_data,
                m_codec_config, m_bidask_config,
                m_start_time_ms, m_end_time_ms);
        }

#       ifdef DFH_TEST_MODE
        /// \brief Для проведения тестов
        /// \tparam T Тип загрузчика данных
        /// \param time_ms Временная метка для загрузки
        /// \param loader Функция загрузки тиков
        template <typename T>
        void test_load_ticks(uint64_t time_ms, T loader) {
            if (m_ticks.empty()) {
                loader(m_ticks, m_codec_config);
            } else if (time_ms >= m_end_time_ms) {
                if (time_ms < (m_end_time_ms + time_shield::MS_PER_HOUR)) {
                    m_ticks.clear();
                    loader(m_ticks, m_codec_config);
                } else {
                    m_has_prev_data = false;
                    m_ticks.clear();
                    loader(m_ticks, m_codec_config);
                }
            } else
            if (time_ms < m_start_time_ms) {
                m_has_prev_data = false;
                m_ticks.clear();
                loader(m_ticks, m_codec_config);
            } else {
                return;
            }

            m_start_time_ms = time_shield::start_of_hour_ms(time_ms);
            m_end_time_ms = m_start_time_ms + time_shield::MS_PER_HOUR;

            m_spread_processor->process(
                m_ticks, m_chunks,
                m_prev_tick, m_has_prev_data,
                m_codec_config, m_bidask_config,
                m_start_time_ms, m_end_time_ms);
        }


        /// \brief Для проведения тестов
        /// \tparam T Тип загрузчика данных
        /// \param time_ms Временная метка для загрузки
        /// \param loader Функция загрузки тиков
        template <typename T>
        void test_load_ticks_2(uint64_t time_ms, T loader) {
            if (m_ticks.empty()) {
                loader(time_ms, m_ticks, m_codec_config);
            } else
            if (time_ms >= m_end_time_ms) {
                if (time_ms < (m_end_time_ms + time_shield::MS_PER_HOUR)) {
                    m_ticks.clear();
                    loader(time_ms, m_ticks, m_codec_config);
                } else {
                    m_has_prev_data = false;
                    m_ticks.clear();
                    loader(time_ms, m_ticks, m_codec_config);
                }
            } else
            if (time_ms < m_start_time_ms) {
                m_has_prev_data = false;
                m_ticks.clear();
                loader(time_ms, m_ticks, m_codec_config);
            } else {
                return;
            }

            m_start_time_ms = time_shield::start_of_hour_ms(time_ms);
            m_end_time_ms = m_start_time_ms + time_shield::MS_PER_HOUR;

            m_spread_processor->process(
                m_ticks, m_chunks,
                m_prev_tick, m_has_prev_data,
                m_codec_config, m_bidask_config,
                m_start_time_ms, m_end_time_ms);
        }
#       endif

        /// \brief Adds real-time tick data to the buffer and writes to the database.
        /// \tparam T Type of the function used for database writing.
        /// \param ticks Reference to the vector of new ticks.
        /// \param db_writer Functor for writing the tick array to the database.
        /// \param calculate_last_updated Flag indicating whether to calculate `LAST_UPDATED` for all ticks.
        template <typename T>
        void append_ticks(
            const std::vector<MarketTick>& ticks,
            T db_writer,
            bool calculate_last_updated = false) {
            if (ticks.empty()) return;

            for (const auto& tick : ticks) {
                if (!m_ticks.empty() && tick.time_ms <= m_ticks.back().time_ms) {
                    throw std::invalid_argument("Ticks must be in chronological order.");
                }

                if (!m_ticks.empty() && calculate_last_updated) {
                    if (!utils::compare_with_precision(tick.last, m_ticks.back().last, m_codec_config.price_digits)) {
                        m_ticks.back().set_flag(TickUpdateFlags::LAST_UPDATED);
                    }
                }

                m_ticks.push_back(tick);

                if (m_ticks.front().time_ms < m_start_time_ms) {
                    m_start_time_ms = time_shield::start_of_hour_ms(tick.time_ms);
                }

                if (tick.time_ms >= m_start_time_ms + time_shield::MS_PER_HOUR) {
                    db_writer(m_ticks);
                    m_ticks.clear();
                    m_start_time_ms = time_shield::start_of_hour_ms(tick.time_ms);
                }
            }

            m_spread_processor->process(
                m_ticks, m_chunks, m_prev_tick, m_has_prev_data, m_codec_config, m_bidask_config, m_start_time_ms, m_end_time_ms);
        }

        /// \brief Returns the number of ticks in the buffer.
        /// \return Number of stored ticks.
        size_t tick_count() const {
            return m_ticks.size();
        }

    private:
        std::vector<MarketTick> m_ticks;    ///< Buffer of market ticks.
        std::vector<uint32_t>   m_chunks;   ///< Indices of data chunks.

        MarketTick     m_prev_tick;   ///< Previous tick.
        MarketTickSpan m_tick_span;   ///< Current tick range.
        bool m_has_prev_data = false; ///< Flag indicating whether previous data exists.

        TickCodecConfig     m_codec_config; ///< Tick codec configuration.
        BidAskRestoreConfig m_bidask_config; ///< Bid/ask restoration configuration.

        uint64_t m_start_time_ms = 0; ///< Start time of the buffer in milliseconds.
        uint64_t m_end_time_ms   = 0; ///< End time of the buffer in milliseconds.

        NoneSpreadProcessor    m_none_processor;
        FixedSpreadProcessor   m_fixed_processor;
        DynamicSpreadProcessor m_dynamic_processor;
        MedianSpreadProcessor  m_median_processor;
        ISpreadProcessor*      m_spread_processor = &m_none_processor;

        /// \brief Загрузка и обработка данных за один час.
        /// \tparam T Тип загрузчика данных
        /// \param loader Функция загрузки тиков
        /// \param prev_time_ms Временная метка начала диапазона
        /// \param start_time_ms Временная метка конца диапазона
        void load_and_process(
                uint32_t index,
                uint64_t prev_time_ms,
                uint64_t start_time_ms,
                IMarketDataSource* data_source) {
            m_ticks.clear();
            data_source->fetch_ticks(
                index,
                prev_time_ms,
                prev_time_ms + time_shield::MS_PER_HOUR,
                m_ticks,
                m_codec_config);
            if (m_ticks.empty()) return;
            m_spread_processor->process(
                m_ticks, m_chunks,
                m_prev_tick, m_has_prev_data,
                m_codec_config, m_bidask_config,
                prev_time_ms, start_time_ms);
        }

        void reload_ticks(
                uint32_t index,
                uint64_t start_time_ms,
                IMarketDataSource* data_source) {
            m_has_prev_data = false;
            const uint64_t prev_time_ms = start_time_ms - time_shield::MS_PER_HOUR;
            load_and_process(index, prev_time_ms, start_time_ms, data_source);
            m_ticks.clear();
            data_source->fetch_ticks(
                index,
                start_time_ms,
                start_time_ms + time_shield::MS_PER_HOUR,
                m_ticks,
                m_codec_config);
        }

    }; // StreamTickBuffer

}; // namespace dfh::core

#endif // _DFH_STREAM_TICK_BUFFER_HPP_INCLUDED
