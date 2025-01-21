#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>
#include <chrono>
#include <fstream>
#include <string>
#include <stdexcept>
#include <DataFeedHub/ticks.hpp>

namespace dfh {

    /// \brief Reads the entire content of a file into a std::string.
    /// \param file_path Path to the input file.
    /// \return A std::string containing the file's content.
    /// \throws std::runtime_error If the file cannot be opened.
    std::string read_file_to_string(const std::string& file_path) {
        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("Failed to open file: " + file_path);
        }

        std::streamsize file_size = file.tellg();
        if (file_size <= 0) {
            return {};
        }

        std::string content(static_cast<size_t>(file_size), '\0');
        file.seekg(0, std::ios::beg);
        if (!file.read(&content[0], file_size)) {
            throw std::runtime_error("Failed to read file: " + file_path);
        }

        return content;
    }

    /// \brief Saves binary data to a file.
    /// \param file_path Path to the output file.
    /// \param binary_data The vector containing binary data to write.
    /// \throws std::runtime_error If the file cannot be opened.
    void save_binary_to_file(const std::string& file_path, const std::vector<uint8_t>& binary_data) {
        std::ofstream file(file_path, std::ios::binary | std::ios::out);
        if (!file) {
            throw std::runtime_error("Failed to open file: " + file_path);
        }

        file.write(reinterpret_cast<const char*>(binary_data.data()), binary_data.size());
        if (!file) {
            throw std::runtime_error("Failed to write to file: " + file_path);
        }
    }

    /// \brief Parses CSV trade data from Bybit and appends to a vector of MarketTick
    /// \param csv_data The CSV data as a string.
    /// \param ticks The vector to append parsed MarketTick structures.
    void parse_bybit_trades(const std::string& csv_data, MarketTickSequence& sequence, size_t reserve_size = 1000000) {
        std::istringstream csv_stream(csv_data);
        std::string line;

        sequence.ticks.reserve(reserve_size);
        if (!std::getline(csv_stream, line)) return; // Skip the header line
        while (std::getline(csv_stream, line)) {
            std::istringstream line_stream(line);
            std::string field;

            // Read fields
            double timestamp = 0.0;
            std::string symbol;
            std::string side;
            double size = 0.0;
            double price = 0.0;
            std::string tickDirection;
            std::string trdMatchID;
            double grossValue = 0.0;
            double homeNotional = 0.0;
            double foreignNotional = 0.0;

            try {
                std::getline(line_stream, field, ','); timestamp = std::stod(field);
                std::getline(line_stream, symbol, ',');
                std::getline(line_stream, side, ',');
                std::getline(line_stream, field, ','); size = std::stod(field);
                std::getline(line_stream, field, ','); price = std::stod(field);
                std::getline(line_stream, tickDirection, ',');
                std::getline(line_stream, trdMatchID, ',');
                std::getline(line_stream, field, ','); grossValue = std::stod(field);
                std::getline(line_stream, field, ','); homeNotional = std::stod(field);
                std::getline(line_stream, field, ','); foreignNotional = std::stod(field);
            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to parse line: " + line);
            }

            // Create MarketTick and set flags
            MarketTick tick;
            tick.last = price;
            tick.volume = size;
            tick.time_ms = time_shield::fsec_to_ms(timestamp);

            if (side == "Buy") {
                tick.set_flag(TickUpdateFlags::TICK_FROM_BUY);
            } else
            if (side == "Sell") {
                tick.set_flag(TickUpdateFlags::TICK_FROM_SELL);
            }

            // Handle tickDirection
            if (tickDirection == "PlusTick") {
                tick.set_flag(TickUpdateFlags::LAST_UPDATED);
            } else
            if (tickDirection == "MinusTick") {
                tick.set_flag(TickUpdateFlags::LAST_UPDATED);
            }

            sequence.ticks.push_back(tick);
        }
    }
}

/// \brief Generates a random value within the range of the specified number of bytes.
/// \param size The number of bytes determining the range.
/// \param is_signed Whether the range includes negative values (signed).
/// \param is_time
/// \return A random integer within the range.
int64_t generate_random_delta(size_t size, bool is_signed, bool is_time) {
    //static std::mt19937 rng(std::random_device{}());
    static std::mt19937 rng(123);

    static const std::array<int64_t, 4> max_values = {
        0xFF,
        0xFFFF,
        0xFFFFFFFF,
        //0xFFFFFFFFFFFFFFFF,
        0x0FFFFFFFFFFFFFFF,
    };

    static const std::array<int64_t, 4> max_time_values = {
        0x3F,
        0x3FFF,
        0x3FFFFFFF,
        0x3FFFFFFFFFFFFFFF,
    };

    static const std::array<int64_t, 4> max_signed_values = {
        127,
        32767,
        2147483647,
        9223372036854775807,
    };

    size_t code_size = dfh::utils::bytes_to_code(size);
    int64_t min_value = is_signed ? -max_signed_values[code_size] : 0;
    int64_t max_value = is_time ? max_time_values[code_size] : is_signed ? max_signed_values[code_size] : max_values[code_size];

    std::uniform_int_distribution<int64_t> dist(min_value, max_value);
    return dist(rng);
}

/// \brief Generates random ticks based on configuration and delta size settings.
/// \param num_ticks Number of ticks to generate.
/// \param config The encoding configuration.
/// \param delta_spec A vector of tuples specifying the byte sizes for price, volume, and time deltas.
///                   Each tuple is of the form (price_delta_size, volume_delta_size, time_delta_size, sample_length).
/// \return A vector of randomly generated ticks.
std::vector<dfh::MarketTick> generate_random_ticks_with_deltas(
    size_t num_ticks,
    const dfh::TickEncodingConfig& config,
    const std::vector<std::tuple<size_t, size_t, size_t, size_t>>& delta_spec) {

    std::vector<dfh::MarketTick> ticks(num_ticks);
    size_t current_sample = 0, sample_index = 0;

    double price_scale  = dfh::utils::pow10<double>(config.price_digits);
    double volume_scale = dfh::utils::pow10<double>(config.volume_digits);

    uint64_t current_time_ms = time_shield::timestamp_ms(2024, 01, 01); // Arbitrary starting time
    uint64_t current_received_ms = current_time_ms + time_shield::MS_PER_SEC;
    double current_bid = 10000.0, current_ask = 10000.01, current_last = 10000.02, current_volume = 100.0;

    for (size_t i = 0; i < num_ticks; ++i) {
        // Determine the delta sizes for the current sample
        const auto& [price_delta_size, volume_delta_size, time_delta_size, sample_length] = delta_spec[sample_index];

        // Generate deltas for prices
        int64_t delta_bid = generate_random_delta(price_delta_size, true, false);
        int64_t delta_ask = generate_random_delta(price_delta_size, true, false);
        int64_t delta_last = generate_random_delta(price_delta_size, true, false);

        // Generate deltas for volume
        int64_t delta_volume = generate_random_delta(volume_delta_size, true, false);
        int64_t delta_time_ms = generate_random_delta(time_delta_size, false, true);

        // Assign flags and apply deltas
        if (config.enable_tick_flags) {
            switch (i % 5) { // Cycle through update scenarios
                case 0:
                    ticks[i].set_flag(dfh::TickUpdateFlags::BID_UPDATED);
                    current_bid += delta_bid / price_scale;
                    break;
                case 1:
                    ticks[i].set_flag(dfh::TickUpdateFlags::ASK_UPDATED);
                    current_ask += delta_ask / price_scale;
                    break;
                case 2:
                    ticks[i].set_flag(dfh::TickUpdateFlags::LAST_UPDATED);
                    current_last += delta_last / price_scale;
                    break;
                case 3:
                    ticks[i].set_flag(dfh::TickUpdateFlags::VOLUME_UPDATED);
                    current_volume += delta_volume / volume_scale;
                    break;
                case 4:
                    ticks[i].set_flag(dfh::TickUpdateFlags::BID_UPDATED);
                    ticks[i].set_flag(dfh::TickUpdateFlags::ASK_UPDATED);
                    ticks[i].set_flag(dfh::TickUpdateFlags::LAST_UPDATED);
                    ticks[i].set_flag(dfh::TickUpdateFlags::VOLUME_UPDATED);
                    current_bid += delta_bid / price_scale;
                    current_ask += delta_ask / price_scale;
                    current_last += delta_last / price_scale;
                    current_volume += delta_volume / volume_scale;
                    break;
            }
        }
        current_time_ms += delta_time_ms;
        current_received_ms = current_time_ms + time_shield::MS_PER_SEC;

        // Populate the tick
        ticks[i].bid  = dfh::utils::normalize_double(current_bid, config.price_digits);
        ticks[i].ask  = dfh::utils::normalize_double(current_ask, config.price_digits);
        ticks[i].last = dfh::utils::normalize_double(current_last, config.price_digits);
        ticks[i].volume = dfh::utils::normalize_double(current_volume, config.volume_digits);
        ticks[i].time_ms = current_time_ms;
        ticks[i].received_ms = config.enable_received_time ? current_received_ms : 0;

        // Update sample index
        if (++current_sample >= sample_length) {
            current_sample = 0;
            sample_index = (sample_index + 1) % delta_spec.size();
        }
    }

    return ticks;
}

// Utility function to execute a test and display results
void execute_test(
        const std::string& test_name,
        const std::vector<dfh::MarketTick>& ticks,
        const dfh::TickEncodingConfig& config) {
    std::cout << "\n=== " << test_name << " ===\n";

    dfh::MarketTickCodec codec;
    codec.set_config(config);
    // Encode ticks
    std::vector<uint8_t> binary_data = codec.encode(ticks);
    std::cout << "binary_data size: " << binary_data.size() << std::endl;
    std::string filename = "test\\" + std::to_string(ticks.back().time_ms) + ".bin";
    dfh::save_binary_to_file(filename, binary_data);

    // Decode ticks
    dfh::TickEncodingConfig decoded_config;
    codec.set_config(decoded_config);

    std::vector<dfh::MarketTick> decoded_ticks = codec.decode(binary_data);

    // Compare results
    std::cout << std::fixed << std::setprecision(config.price_digits);
    bool all_equal = true;

    if (ticks.size() != decoded_ticks.size()) {
        std::cout << "Size error!\n";
        std::cout << "ticks: " << ticks.size() << std::endl;
        std::cout << "decoded_ticks: " << decoded_ticks.size() << std::endl;
        return;
    }

    for (size_t i = 0; i < ticks.size(); ++i) {
        const auto& orig = ticks[i];
        const auto& decoded = decoded_ticks[i];

        if (!dfh::compare_ticks(orig, decoded, config)) {
            all_equal = false;

            std::cout << "Tick " << i << ":\n";
            std::cout << "  Original: Bid=" << orig.bid << ", Ask=" << orig.ask
                      << ", Last=" << orig.last << ", Volume=" << orig.volume
                      << ", Time=" << orig.time_ms << ", Received=" << orig.received_ms
                      << ", Flags=" << orig.flags << "\n";
            std::cout << "  Decoded:  Bid=" << decoded.bid << ", Ask=" << decoded.ask
                      << ", Last=" << decoded.last << ", Volume=" << decoded.volume
                      << ", Time=" << decoded.time_ms << ", Received=" << decoded.received_ms
                      << ", Flags=" << decoded.flags << "\n";

            std::cout << "  --> Mismatch detected!\n";
            break;
        }
    }

    if (all_equal) {
        std::cout << "All ticks match successfully.\n";
    } else {
        std::cout << "Errors found in tick decoding.\n";
    }
}

// Test 1: Full-featured test with flags, real volume, and received_ms
void test_full_featured() {
    dfh::TickEncodingConfig config;

    /*
    const size_t num_ticks = 42000;
    std::vector<std::tuple<size_t, size_t, size_t, size_t>> delta_spec = {
        {1, 1, 1, 100},
        {2, 1, 1, 10},
        {1, 1, 1, 2},
        {2, 1, 1, 10},
        {1, 2, 1, 3},
        {1, 1, 2, 5},
        {1, 1, 1, 10},
        {2, 2, 2, 2},
        {1, 1, 1, 3},
        {1, 2, 1, 4},
        {1, 1, 2, 5},
        {2, 1, 1, 2},
        {2, 2, 2, 3},
        {1, 1, 1, 2},
        {1, 2, 1, 3},
    };
    */

    //std::vector<dfh::MarketTick> ticks = generate_random_ticks_with_deltas(num_ticks, config, delta_spec);
    dfh::MarketTickSequence sequence;
    sequence.price_digits = 2;
    sequence.volume_digits = 3;

    auto csv_data = dfh::read_file_to_string("BTCUSDT2024-10-01.csv");
    dfh::parse_bybit_trades(csv_data, sequence, 3000000);

    config.price_digits = sequence.price_digits;
    config.volume_digits = sequence.volume_digits;
    config.enable_trade_based_encoding = true;
    config.enable_tick_flags    = true;
    config.enable_received_time = false;
    config.fixed_spread = 1;

    std::vector<dfh::MarketTick> ticks;
    uint64_t hour = time_shield::start_of_hour_ms(sequence.ticks[0].time_ms);
    for (size_t i = 0; i < sequence.ticks.size(); ++i) {
        uint64_t tick_hour = time_shield::start_of_hour_ms(sequence.ticks[i].time_ms);
        if (tick_hour != hour) {
            hour = tick_hour;
            execute_test("Test 1: Full-featured (Flags, Real Volume, Received MS)", ticks, config);
            ticks.clear();
            std::cout << "tick_hour " << tick_hour << std::endl;
        }
        ticks.push_back(sequence.ticks[i]);
    }
}

void test_trade_based_encoding() {
    dfh::TickEncodingConfig config;
    config.price_digits = 5;
    config.volume_digits = 3;
    config.fixed_spread = 1;
    config.encoding_mode = dfh::TickEncodingMode::TRADE_BASED_MIXED;
    config.enable_trade_based_encoding = true;
    config.enable_tick_flags    = true;
    config.enable_received_time = true;

    const size_t num_ticks = 42000;
    std::vector<std::tuple<size_t, size_t, size_t, size_t>> delta_spec = {
        {1, 1, 1, 100},
        {2, 1, 1, 10},
        {1, 1, 1, 2},
        {2, 1, 1, 10},
        {1, 2, 1, 3},
        {1, 1, 2, 5},
        {1, 1, 1, 10},
        {2, 2, 2, 2},
        {1, 1, 1, 3},
        {1, 2, 1, 4},
        {1, 1, 2, 5},
        {2, 1, 1, 2},
        {2, 2, 2, 3},
        {1, 1, 1, 2},
        {1, 2, 1, 3},
    };

    std::vector<dfh::MarketTick> ticks = generate_random_ticks_with_deltas(num_ticks, config, delta_spec);

    execute_test("Test 2: ", ticks, config);
}

// Main function to run all tests
int main() {
    test_full_featured();
    return 0;
}
