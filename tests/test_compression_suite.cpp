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
#include <map>
#include <set>
#include <unordered_map>
#include <DataFeedHub/ticks.hpp>
#include <fast_double_parser.h>

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
                std::getline(line_stream, field, ',');
                fast_double_parser::parse_number(field.c_str(), &timestamp);
                std::getline(line_stream, symbol, ',');
                std::getline(line_stream, side, ',');
                std::getline(line_stream, field, ',');
                fast_double_parser::parse_number(field.c_str(), &size);
                std::getline(line_stream, field, ',');
                fast_double_parser::parse_number(field.c_str(), &price);
                std::getline(line_stream, tickDirection, ',');
                std::getline(line_stream, trdMatchID, ',');
                std::getline(line_stream, field, ','); //grossValue = std::stod(field);
                std::getline(line_stream, field, ','); //homeNotional = std::stod(field);
                std::getline(line_stream, field, ','); //foreignNotional = std::stod(field);
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

    dfh::MarketTickSequence sequence;
    sequence.price_digits = 2;
    sequence.volume_digits = 3;

    auto csv_data = dfh::read_file_to_string("BTCUSDT2024-10-01.csv");
    dfh::parse_bybit_trades(csv_data, sequence, 3000000);

    config.price_digits  = sequence.price_digits;
    config.volume_digits = sequence.volume_digits;
    config.enable_trade_based_encoding = true;
    config.enable_tick_flags           = true;
    config.enable_received_time        = false;
    config.fixed_spread = 1;

    double price_scale = dfh::utils::pow10<double>(config.price_digits);
    double volume_scale = dfh::utils::pow10<double>(config.volume_digits);
    std::vector<uint32_t> prices(sequence.ticks.size());
    std::vector<uint32_t> volumes(sequence.ticks.size());
    std::vector<uint64_t> times(sequence.ticks.size());
    prices[0] = 0;
    for (size_t i = 1; i < sequence.ticks.size(); ++i) {
        int64_t curr = static_cast<int64_t>(std::round(sequence.ticks[i].last * price_scale));
        int64_t prev = static_cast<int64_t>(std::round(sequence.ticks[i-1].last * price_scale));
        prices[i] = static_cast<uint32_t>(std::abs(curr - prev));
    }
    for (size_t i = 0; i < sequence.ticks.size(); ++i) {
        volumes[i] = static_cast<uint32_t>(std::round(sequence.ticks[i].volume * volume_scale));
        times[i] = sequence.ticks[i].time_ms;
    }

    {
        std::vector<uint8_t> binary_simdcomp;
        std::vector<uint8_t> binary_vbyte;
        dfh::utils::append_simdcomp(binary_simdcomp, prices);
        dfh::utils::append_vbyte(binary_vbyte, prices);
        std::cout << "prices simdcomp: " << binary_simdcomp.size() << std::endl;
        std::cout << "prices vbyte:    " << binary_vbyte.size() << std::endl;
    }
    {
        std::vector<uint32_t> sorted_values;
        std::vector<uint32_t> unsorted_codes;
        dfh::utils::encode_frequency_inplace<std::vector<uint32_t>, uint32_t>(prices, sorted_values, unsorted_codes);

        std::vector<uint32_t> prices_u32 = dfh::utils::encode_with_repeats(prices, 0);
        dfh::utils::encode_delta_inplace(sorted_values, 0);
        dfh::utils::encode_delta_zigzag_inplace(unsorted_codes, 0);

        std::vector<uint8_t> binary_simdcomp;
        std::vector<uint8_t> binary_vbyte;
        dfh::utils::append_simdcomp(binary_simdcomp, prices_u32);
        dfh::utils::append_vbyte(binary_vbyte, prices_u32);
        std::cout << "prices (0) simdcomp: " << binary_simdcomp.size() << std::endl;
        std::cout << "prices (0) vbyte:    " << binary_vbyte.size() << std::endl;
    }
    {
        std::vector<uint32_t> sorted_values;
        std::vector<uint32_t> unsorted_codes;
        dfh::utils::encode_frequency_inplace<std::vector<uint32_t>, uint32_t>(prices, sorted_values, unsorted_codes);

        std::vector<uint32_t> prices_u32 = dfh::utils::encode_with_repeats(prices, 1);
        dfh::utils::encode_delta_inplace(sorted_values, 0);
        dfh::utils::encode_delta_zigzag_inplace(unsorted_codes, 0);

        std::vector<uint8_t> binary_simdcomp;
        std::vector<uint8_t> binary_vbyte;
        dfh::utils::append_simdcomp(binary_simdcomp, prices_u32);
        dfh::utils::append_vbyte(binary_vbyte, prices_u32);
        std::cout << "prices (1) simdcomp: " << binary_simdcomp.size() << std::endl;
        std::cout << "prices (1) vbyte:    " << binary_vbyte.size() << std::endl;

        std::vector<uint8_t> binary_simdcomp2;
        std::vector<uint8_t> binary_vbyte2;
        dfh::utils::append_simdcomp(binary_simdcomp2, sorted_values);
        dfh::utils::append_vbyte(binary_vbyte2, sorted_values);
        std::cout << "prices (1)s simdcomp: " << binary_simdcomp2.size() << std::endl;
        std::cout << "prices (1)s vbyte:    " << binary_vbyte2.size() << std::endl;

        std::vector<uint8_t> binary_simdcomp3;
        std::vector<uint8_t> binary_vbyte3;
        dfh::utils::append_simdcomp(binary_simdcomp3, unsorted_codes);
        dfh::utils::append_vbyte(binary_vbyte3, unsorted_codes);
        std::cout << "prices (1)u simdcomp: " << binary_simdcomp3.size() << std::endl;
        std::cout << "prices (1)u vbyte:    " << binary_vbyte3.size() << std::endl;
    }
    {
        std::vector<uint8_t> binary_simdcomp;
        std::vector<uint8_t> binary_vbyte;
        std::vector<uint8_t> binary_varintgb;
        dfh::utils::append_simdcomp(binary_simdcomp, volumes);
        dfh::utils::append_vbyte(binary_vbyte, volumes);
        std::cout << "volumes simdcomp: " << binary_simdcomp.size() << std::endl;
        std::cout << "volumes vbyte:    " << binary_vbyte.size() << std::endl;
        std::cout << "volumes varintgb: " << binary_varintgb.size() << std::endl;
    }
    {
        uint32_t minvolume = 0xFFFFFFFF;
        for (size_t i = 0; i < sequence.ticks.size(); ++i) {
            if (volumes[i] < minvolume) minvolume = volumes[i];
        }
        for (size_t i = 0; i < sequence.ticks.size(); ++i) {
            volumes[i] -= minvolume;
        }
        std::cout << "volumes-minvolume:    " << minvolume << std::endl;
        uint32_t zero_count = 0;
        for (size_t i = 0; i < sequence.ticks.size(); ++i) {
            if (volumes[i] == 0) {
                ++zero_count;
            }
        }
        std::cout << "volumes-zero_count:   " << zero_count << std::endl;
    }
    {
        std::vector<uint32_t> sorted_values;
        std::vector<uint32_t> unsorted_codes;
        dfh::utils::encode_frequency_inplace<std::vector<uint32_t>, uint32_t>(volumes, sorted_values, unsorted_codes);
        dfh::utils::encode_delta_inplace(sorted_values, 0);
        dfh::utils::encode_delta_zigzag_inplace(unsorted_codes, 0);

        std::vector<uint8_t> binary_simdcomp2;
        std::vector<uint8_t> binary_vbyte2;
        dfh::utils::append_simdcomp(binary_simdcomp2, sorted_values);
        dfh::utils::append_vbyte(binary_vbyte2, sorted_values);
        std::cout << "volumes s simdcomp: " << binary_simdcomp2.size() << std::endl;
        std::cout << "volumes s vbyte:    " << binary_vbyte2.size() << std::endl;

        std::vector<uint8_t> binary_simdcomp3;
        std::vector<uint8_t> binary_vbyte3;
        dfh::utils::append_simdcomp(binary_simdcomp3, unsorted_codes);
        dfh::utils::append_vbyte(binary_vbyte3, unsorted_codes);
        std::cout << "volumes u simdcomp: " << binary_simdcomp3.size() << std::endl;
        std::cout << "volumes u vbyte:    " << binary_vbyte3.size() << std::endl;

        for (int i = 0; i < 5; ++i) {
            std::vector<uint32_t> volumes_u32 = dfh::utils::encode_with_repeats(volumes, i);

            std::vector<uint8_t> binary_simdcomp;
            std::vector<uint8_t> binary_vbyte;
            dfh::utils::append_simdcomp(binary_simdcomp, volumes_u32);
            dfh::utils::append_vbyte(binary_vbyte, volumes_u32);
            std::cout << "volumes (" << i << ") simdcomp: " << binary_simdcomp.size() << std::endl;
            std::cout << "volumes (" << i << ") vbyte:    " << binary_vbyte.size() << std::endl;
        }
    }
    {
#if(0)
        std::set<uint32_t> volumes_set;
        for (size_t i = 0; i < volumes.size(); ++i) {
            volumes_set.insert(volumes[i]);
        }

        std::unordered_map<uint32_t, uint32_t> volume_to_index;
        volume_to_index.reserve(volumes_set.size());

        std::vector<uint32_t> sorted_values(volumes_set.size());
        uint32_t code = 0;
        for (auto &p : volumes_set) {
            volume_to_index[p] = code;
            sorted_values[code] = p;
            std::cout << "code: " << code << " p: " << p << std::endl;
            code++;
        }

        for (int i = sorted_values.size() - 1; i >= 1; --i) {
            sorted_values[i] = sorted_values[i] - sorted_values[i - 1];
        }

#endif

#if(1)
        std::map<uint32_t, uint32_t> volumes_map;
        for (size_t i = 0; i < volumes.size(); ++i) {
            volumes_map[volumes[i]]++;
        }

        std::vector<std::pair<uint32_t, uint32_t>> freq_pairs;
        freq_pairs.reserve(volumes_map.size());
        for (auto &kv : volumes_map) {
            freq_pairs.emplace_back(kv.second, kv.first);
        }

        std::sort(freq_pairs.begin(), freq_pairs.end(),
            [](const auto &a, const auto &b) {
                if (a.first != b.first) {
                    return a.first > b.first;
                }
                return a.second < b.second;
            }
        );

        std::unordered_map<uint32_t, uint32_t> volume_to_index;
        volume_to_index.reserve(freq_pairs.size());

        uint32_t code = 0;
        for (auto &p : freq_pairs) {
            volume_to_index[p.second] = code;
            code++;
        }

        std::vector<uint32_t> sorted_values;
        std::vector<uint32_t> unsorted_codes;
        sorted_values.reserve(volumes_map.size());
        for (auto &kv : volumes_map) {
            sorted_values.emplace_back(kv.first);
            unsorted_codes.emplace_back(kv.first);
        }

        /*
        for (int i = sorted_values.size() - 1; i >= 1; --i) {
            sorted_values[i] = sorted_values[i] - sorted_values[i - 1];
        }
        */


#endif
        /*
        std::multimap<uint32_t, uint32_t> volumes_freq;
        for (auto &volume : volumes_map) {
            volumes_freq.insert({volume.second, volume.first});
        }

        size_t code = volumes_map.size() - 1;
        std::unordered_map<uint32_t, uint32_t> volume_to_index;
        for (auto &volume : volumes_freq) {
            volume_to_index[volume.second] = code--;
        }
        */

        /*
        std::vector<uint32_t> unique_volumes(volumes_map.size());
        for (size_t i = 0; i < volumes_map.size(); ++i) {
            unique_volumes[i] = volume_to_index[volumes[i]];
        }
        */

        std::vector<uint32_t> volumes_compressed(volumes.size());
        for (size_t i = 0; i < volumes.size(); ++i) {
            volumes_compressed[i] = volume_to_index[volumes[i]];
        }

        std::vector<uint32_t> volumes_u32;
        uint32_t max_volume = 0x07;
        uint32_t volume_count = 0;
        uint32_t prev_volume = volumes_compressed[0];
        for (size_t i = 0; i < volumes_compressed.size(); ++i) {
            if (volumes_compressed[i] <= max_volume && volumes_compressed[i] == prev_volume) {
                ++volume_count;
                continue;
            }

            if (volume_count > 0) {
                uint32_t temp = ((prev_volume << 1) | 0x1) | (volume_count << 4);
                volumes_u32.push_back(temp);
                volume_count = 0;
            }
            prev_volume = volumes_compressed[i];
            volumes_u32.push_back(volumes_compressed[i] << 1); // 000
        }
        std::cout << "volumes_u32: " << volumes_u32.size() << std::endl;

        std::vector<uint8_t> binary_simdcomp;
        std::vector<uint8_t> binary_vbyte;
        dfh::utils::append_simdcomp(binary_simdcomp, volumes_u32);
        dfh::utils::append_vbyte(binary_vbyte, volumes_u32);

        std::vector<uint8_t> binary_simdcomp2;
        std::vector<uint8_t> binary_vbyte2;
        dfh::utils::append_simdcomp(binary_simdcomp2, sorted_values);
        dfh::utils::append_vbyte(binary_vbyte2, sorted_values);

        std::vector<uint8_t> binary_simdcomp3;
        std::vector<uint8_t> binary_vbyte3;
        dfh::utils::append_simdcomp(binary_simdcomp3, unsorted_codes);
        dfh::utils::append_vbyte(binary_vbyte3, unsorted_codes);

        /*
        for (int i = unsorted_codes.size() - 1; i >= 1; --i) {
            unsorted_codes[i] = std::abs((int32_t)unsorted_codes[i] - (int32_t)unsorted_codes[i - 1]);
            unsorted_codes[i] <<= 1;
        }
        */
        auto t1 = unsorted_codes;
        dfh::utils::encode_delta_zigzag_inplace(unsorted_codes, 0);
        auto t2 = unsorted_codes;
        dfh::utils::decode_delta_zigzag_inplace(t2, 0);
        if (t1 != t2) {
            std::cout << "t1 != t2" << std::endl;
        }

        std::vector<uint8_t> binary_simdcomp4;
        std::vector<uint8_t> binary_vbyte4;
        dfh::utils::append_simdcomp(binary_simdcomp4, unsorted_codes);
        dfh::utils::append_vbyte(binary_vbyte4, unsorted_codes);

        //dfh::utils::append_varintgb(binary_varintgb, volumes_compressed);


        std::cout << "volumes-Huffman simdcomp2: " << binary_simdcomp2.size() << std::endl;
        std::cout << "volumes-Huffman vbyte2:    " << binary_vbyte2.size() << std::endl;

        std::cout << "volumes-Huffman simdcomp3: " << binary_simdcomp3.size() << std::endl;
        std::cout << "volumes-Huffman vbyte3:    " << binary_vbyte3.size() << std::endl;

        std::cout << "volumes-Huffman simdcomp4: " << binary_simdcomp4.size() << std::endl;
        std::cout << "volumes-Huffman vbyte4:    " << binary_vbyte4.size() << std::endl;


        std::cout << "volumes-Huffman simdcomp: " << binary_simdcomp.size() + binary_simdcomp4.size() << std::endl;
        std::cout << "volumes-Huffman vbyte:    " << binary_vbyte.size() + binary_simdcomp4.size() << std::endl;

        size_t offset = 0;
        std::vector<uint32_t> r_simdcomp = dfh::utils::extract_simdcomp(binary_simdcomp.data(), offset);
        offset = 0;
        std::vector<uint32_t> r_vbyte_v32 = dfh::utils::extract_vbyte_v32(binary_vbyte.data(), offset);

        if (r_simdcomp != volumes_u32) {
            std::cout << "r_simdcomp != volumes_u32; " << r_simdcomp.size() << std::endl;
            for (size_t i = 0; i < std::min(r_simdcomp.size(), volumes_u32.size()); ++i) {
                if (r_simdcomp[i] != volumes_u32[i]) {
                    std::cout << i << " " << r_simdcomp[i] << " " << volumes_u32[i] << std::endl;
                }
            }
        }
        if (r_vbyte_v32 != volumes_u32) {
            std::cout << "r_simdcomp != volumes_u32; " << r_vbyte_v32.size() << std::endl;
        }
    }

    {
        uint64_t start_time_ms = times[0];
        for (int i = 0; i < times.size(); ++i) {
            times[i] -= start_time_ms;
        }
        dfh::utils::encode_delta_inplace(times, 0);
        std::vector<uint32_t> converted_values;
        converted_values.reserve(times.size());
        for (uint64_t value : times) {
            converted_values.push_back(static_cast<uint32_t>(value));
        }
        std::vector<uint8_t> binary_simdcomp_a;
        std::vector<uint8_t> binary_vbyte_a;
        dfh::utils::append_simdcomp(binary_simdcomp_a, converted_values);
        dfh::utils::append_vbyte(binary_vbyte_a, converted_values);
        std::cout << "times all simdcomp: " << binary_simdcomp_a.size() << std::endl;
        std::cout << "times all vbyte:    " << binary_vbyte_a.size() << std::endl;


        std::vector<uint32_t> sorted_values;
        std::vector<uint32_t> unsorted_codes;
        dfh::utils::encode_frequency_inplace<std::vector<uint64_t>, uint32_t>(times, sorted_values, unsorted_codes);
        dfh::utils::encode_delta_inplace(sorted_values, 0);
        dfh::utils::encode_delta_zigzag_inplace(unsorted_codes, 0);

        std::cout << "times sorted_values:    " << sorted_values.size() << std::endl;

        std::vector<uint8_t> binary_simdcomp2;
        std::vector<uint8_t> binary_vbyte2;
        dfh::utils::append_simdcomp(binary_simdcomp2, sorted_values);
        dfh::utils::append_vbyte(binary_vbyte2, sorted_values);
        std::cout << "times s simdcomp: " << binary_simdcomp2.size() << std::endl;
        std::cout << "times s vbyte:    " << binary_vbyte2.size() << std::endl;

        std::vector<uint8_t> binary_simdcomp3;
        std::vector<uint8_t> binary_vbyte3;
        dfh::utils::append_simdcomp(binary_simdcomp3, unsorted_codes);
        dfh::utils::append_vbyte(binary_vbyte3, unsorted_codes);
        std::cout << "times u simdcomp: " << binary_simdcomp3.size() << std::endl;
        std::cout << "times u vbyte:    " << binary_vbyte3.size() << std::endl;

        for (int i = 0; i < 5; ++i) {
            std::vector<uint32_t> volumes_u32 = dfh::utils::encode_with_repeats(times, i);

            std::vector<uint8_t> binary_simdcomp;
            std::vector<uint8_t> binary_vbyte;
            dfh::utils::append_simdcomp(binary_simdcomp, volumes_u32);
            dfh::utils::append_vbyte(binary_vbyte, volumes_u32);
            std::cout << "times (" << i << ") simdcomp: " << binary_simdcomp.size() << std::endl;
            std::cout << "times (" << i << ") vbyte:    " << binary_vbyte.size() << std::endl;
        }
    }
}

// Main function to run all tests
int main() {
    test_full_featured();
    return 0;
}
