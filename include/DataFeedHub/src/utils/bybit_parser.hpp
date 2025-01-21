#pragma once
#ifndef _DFH_UTILS_BYBIT_PARSER_HPP_INCLUDED
#define _DFH_UTILS_BYBIT_PARSER_HPP_INCLUDED

/// \file bybit_parser.hpp
/// \brief Parses CSV trade data from Bybit.

#include <fast_double_parser.h>

namespace dfh::utils {

    /// \brief Parses CSV trade data from Bybit and appends to a vector of MarketTick
    /// \param sequence The MarketTickSequence to store parsed MarketTick structures.
    /// \param csv_data The CSV data as a string.
    /// \param reserve_size The initial reserve size for the ticks vector.
    /// \param auto_detect_precision Enables automatic detection of price and volume precision.
    void parse_bybit_trades(
            MarketTickSequence& sequence,
            const std::string& csv_data,
            size_t reserve_size = 1000000,
            bool auto_detect_precision = false) {
        sequence.ticks.reserve(reserve_size);
        sequence.ticks.resize(0);

        size_t max_price_digits = 0;
        size_t max_volume_digits = 0;
        size_t price_digits = 0;
        size_t volume_digits = 0;
        double timestamp = 0.0;
        std::string field;
        std::string symbol;
        std::string side;
        std::string tickDirection;
        std::string line;
        std::istringstream csv_stream(csv_data);
        if (!std::getline(csv_stream, line)) return; // Skip the header line
        while (std::getline(csv_stream, line)) {
            std::istringstream line_stream(line);
            MarketTick tick;
            try {
                std::getline(line_stream, field, ',');
                const char* endptr_time = fast_double_parser::parse_number(field.c_str(), &timestamp);
                if (endptr_time == nullptr) throw std::runtime_error("Failed to parse number: " + field);
                std::getline(line_stream, symbol, ',');
                std::getline(line_stream, side, ',');
                std::getline(line_stream, field, ',');
                const char* endptr_volume = fast_double_parser::parse_number(field.c_str(), &tick.volume);
                if (endptr_volume == nullptr) throw std::runtime_error("Failed to parse number: " + field);
                if (auto_detect_precision) {
                    auto pos = field.find('.');
                    volume_digits = pos != std::string::npos ? field.size() - pos - 1 : 0;
                    if (volume_digits > max_volume_digits) max_volume_digits = volume_digits;
                }
                std::getline(line_stream, field, ',');
                const char* endptr_last = fast_double_parser::parse_number(field.c_str(), &tick.last);
                if (endptr_last == nullptr) throw std::runtime_error("Failed to parse number: " + field);
                if (auto_detect_precision) {
                    auto pos = field.find('.');
                    price_digits = pos != std::string::npos ? field.size() - pos - 1 : 0;
                    if (price_digits > max_price_digits) max_price_digits = price_digits;
                }
                std::getline(line_stream, tickDirection, ',');
            } catch (const std::runtime_error& e) {
                throw;
            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to parse line: " + line);
            }

            tick.time_ms = time_shield::fsec_to_ms(timestamp);

            if (side == "Buy") {
                tick.set_flag(TickUpdateFlags::TICK_FROM_BUY);
            } else
            if (side == "Sell") {
                tick.set_flag(TickUpdateFlags::TICK_FROM_SELL);
            }

            if (tickDirection == "PlusTick") {
                tick.set_flag(TickUpdateFlags::LAST_UPDATED);
            } else
            if (tickDirection == "MinusTick") {
                tick.set_flag(TickUpdateFlags::LAST_UPDATED);
            }

            sequence.ticks.push_back(tick);
        }

        if (auto_detect_precision) {
            sequence.price_digits = max_price_digits;
            sequence.volume_digits = max_volume_digits;
        }
    }

}; // namespace dfh::utils

#endif // _DFH_UTILS_BYBIT_PARSER_HPP_INCLUDED
