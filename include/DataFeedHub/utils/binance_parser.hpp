#pragma once
#ifndef _DFH_UTILS_BINANCE_PARSER_HPP_INCLUDED
#define _DFH_UTILS_BINANCE_PARSER_HPP_INCLUDED

/// \file binance_parser.hpp
/// \brief Parses CSV trade data from Binance.

namespace dfh::utils {

    /// \brief Parses CSV trade data from Binance futures and appends to a vector of MarketTick
    /// \param sequence MarketTickSequence to store parsed MarketTick structures.
    /// \param csv_data CSV data as a string.
    /// \param reserve_size Initial reserve size for the ticks vector.
    /// \param auto_detect_precision Enables automatic detection of price and volume precision.
    void parse_binance_futures_trades(
            MarketTickSequence& sequence,
            const std::string& csv_data,
            size_t reserve_size = 1000000,
            bool auto_detect_precision = false) {
        if (csv_data.empty()) {
            sequence.ticks.resize(0);
            return;
        }
        sequence.ticks.reserve(reserve_size);
        sequence.ticks.resize(0);

        size_t pos = 0;
        size_t price_digits = 0;
        size_t volume_digits = 0;
        size_t max_price_digits = 0;
        size_t max_volume_digits = 0;
        double last_price = -1.0;
        uint64_t timestamp = 0, time_mode = 0;
        std::string line;
        std::string field;
        std::istringstream csv_stream(csv_data);

        if (!std::isdigit(csv_data[0])) {
            // Skip the header if the first character is not a digit
            if (!std::getline(csv_stream, line)) return;
        }
        while (std::getline(csv_stream, line)) {
            std::istringstream line_stream(line);
            MarketTick tick;
            try {
                // Parse fields
                std::getline(line_stream, field, ','); // id (ignored)

                std::getline(line_stream, field, ','); // price
                const char* endptr_price = fast_double_parser::parse_number(field.c_str(), &tick.last);
                if (endptr_price == nullptr) throw std::runtime_error("Failed to parse price: " + field);

                if (auto_detect_precision) {
                    pos = field.find('.');
                    price_digits = pos != std::string::npos ? field.size() - pos - 1 : 0;
                    max_price_digits = std::max(max_price_digits, price_digits);
                }

                std::getline(line_stream, field, ','); // qty
                const char* endptr_qty = fast_double_parser::parse_number(field.c_str(), &tick.volume);
                if (endptr_qty == nullptr) throw std::runtime_error("Failed to parse quantity: " + field);

                if (auto_detect_precision) {
                    pos = field.find('.');
                    volume_digits = pos != std::string::npos ? field.size() - pos - 1 : 0;
                    max_volume_digits = std::max(max_volume_digits, volume_digits);
                }

                std::getline(line_stream, field, ','); // quote_qty (ignored)
                std::getline(line_stream, field, ','); // time
                timestamp = std::stoll(field);
                if (time_mode == 0) {
                    if (timestamp < 10000000000) {
                        time_mode = 1;
                    } else
                    if (timestamp < 10000000000000) {
                        time_mode = 2;
                    } else {
                        time_mode = 3;
                    }
                }

                switch (time_mode) {
                case 1:
                    tick.time_ms = 1000 * std::stoll(field);
                    break;
                case 2:
                    tick.time_ms = std::stoll(field);
                    break;
                case 3:
                    tick.time_ms = std::stoll(field) / 1000;
                    break;
                };

                std::getline(line_stream, field, ','); // is_buyer_maker
                if ((field == "true" || field == "True")) {
                    tick.set_flag(TickUpdateFlags::TICK_FROM_BUY);
                } else {
                    tick.set_flag(TickUpdateFlags::TICK_FROM_SELL);
                }

                std::getline(line_stream, field, ','); // is_best_match
                tick.set_flag(TickUpdateFlags::BEST_MATH, (field == "True" || field == "true"));

                if (tick.last != last_price) {
                    tick.set_flag(TickUpdateFlags::LAST_UPDATED);
                }
                last_price = tick.last;

            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to parse line: " + line + "\nError: " + e.what());
            }

            sequence.ticks.push_back(tick);
        }

        if (auto_detect_precision) {
            sequence.price_digits = max_price_digits;
            sequence.volume_digits = max_volume_digits;
        }
    }

    /// \brief Parses CSV trade data from Binance spot and appends to a vector of MarketTick
    /// \param sequence MarketTickSequence to store parsed MarketTick structures.
    /// \param csv_data CSV data as a string.
    /// \param reserve_size Initial reserve size for the ticks vector.
    /// \param auto_detect_precision Enables automatic detection of price and volume precision.
    void parse_binance_spot_trades(
            MarketTickSequence& sequence,
            const std::string& csv_data,
            size_t reserve_size = 1000000,
            bool auto_detect_precision = false) {
        if (csv_data.empty()) {
            sequence.ticks.resize(0);
            return;
        }
        sequence.ticks.reserve(reserve_size);
        sequence.ticks.resize(0);

        size_t pos = 0;
        size_t price_digits = 0;
        size_t volume_digits = 0;
        size_t max_price_digits = 0;
        size_t max_volume_digits = 0;
        bool is_buyer_maker = false;
        double last_price = -1.0;
        uint64_t timestamp = 0, time_mode = 0;
        std::string line;
        std::string field;
        std::istringstream csv_stream(csv_data);

        if (!std::isdigit(csv_data[0])) {
            // Skip the header if the first character is not a digit
            if (!std::getline(csv_stream, line)) return;
        }
        while (std::getline(csv_stream, line)) {
            std::istringstream line_stream(line);
            MarketTick tick;
            try {
                // Parse fields
                std::getline(line_stream, field, ','); // id (ignored)

                std::getline(line_stream, field, ','); // price
                const char* endptr_price = fast_double_parser::parse_number(field.c_str(), &tick.last);
                if (endptr_price == nullptr) throw std::runtime_error("Failed to parse price: " + field);

                if (auto_detect_precision) {
                    pos = field.find('.');
                    price_digits = pos != std::string::npos ? field.size() - pos - 1 : 0;
                    max_price_digits = std::max(max_price_digits, price_digits);
                }

                std::getline(line_stream, field, ','); // qty
                const char* endptr_qty = fast_double_parser::parse_number(field.c_str(), &tick.volume);
                if (endptr_qty == nullptr) throw std::runtime_error("Failed to parse quantity: " + field);

                if (auto_detect_precision) {
                    pos = field.find('.');
                    volume_digits = pos != std::string::npos ? field.size() - pos - 1 : 0;
                    max_volume_digits = std::max(max_volume_digits, volume_digits);
                }

                std::getline(line_stream, field, ','); // quote_qty (ignored)
                std::getline(line_stream, field, ','); // time
                timestamp = std::stoll(field);
                if (time_mode == 0) {
                    if (timestamp < 10000000000) {
                        time_mode = 1;
                    } else
                    if (timestamp < 10000000000000) {
                        time_mode = 2;
                    } else {
                        time_mode = 3;
                    }
                }

                switch (time_mode) {
                case 1:
                    tick.time_ms = 1000 * std::stoll(field);
                    break;
                case 2:
                    tick.time_ms = std::stoll(field);
                    break;
                case 3:
                    tick.time_ms = std::stoll(field) / 1000;
                    break;
                };

                std::getline(line_stream, field, ','); // is_buyer_maker
                is_buyer_maker = (field == "true");
                if (is_buyer_maker) {
                    tick.set_flag(TickUpdateFlags::TICK_FROM_BUY);
                } else {
                    tick.set_flag(TickUpdateFlags::TICK_FROM_SELL);
                }

                if (tick.last != last_price) {
                    tick.set_flag(TickUpdateFlags::LAST_UPDATED);
                }
                last_price = tick.last;

            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to parse line: " + line + "\nError: " + e.what());
            }

            sequence.ticks.push_back(tick);
        }

        if (auto_detect_precision) {
            sequence.price_digits = max_price_digits;
            sequence.volume_digits = max_volume_digits;
        }
    }

}; // namespace dfh::utils

#endif // _DFH_UTILS_BINANCE_PARSER_HPP_INCLUDED
