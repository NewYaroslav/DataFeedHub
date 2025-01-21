#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>
#include <DataFeedHub/ticks.hpp>

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

    size_t code_size = dfh::size_to_code(size);
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

    int64_t price_scale = dfh::get_power_of_ten(config.price_digits);
    int64_t volume_scale = dfh::get_power_of_ten(config.volume_digits);

    uint64_t current_time_ms = time_shield::timestamp_ms(2024, 01, 01); // Arbitrary starting time
    uint64_t current_received_ms = current_time_ms + time_shield::MS_PER_SEC;
    double current_bid = 10000.0, current_ask = 10000.01, current_last = 10000.02;
    uint64_t current_volume = 100;
    double current_volume_real = 100.0;

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
        if (config.use_tick_flags) {
            switch (i % 5) { // Cycle through update scenarios
                case 0:
                    ticks[i].set_flag(dfh::TickUpdateFlags::BID_UPDATED);
                    current_bid += delta_bid / static_cast<double>(price_scale);
                    break;
                case 1:
                    ticks[i].set_flag(dfh::TickUpdateFlags::ASK_UPDATED);
                    current_ask += delta_ask / static_cast<double>(price_scale);
                    break;
                case 2:
                    ticks[i].set_flag(dfh::TickUpdateFlags::LAST_UPDATED);
                    current_last += delta_last / static_cast<double>(price_scale);
                    break;
                case 3:
                    ticks[i].set_flag(dfh::TickUpdateFlags::VOLUME_UPDATED);
                    current_volume += config.use_real_volume ? 0 : delta_volume;
                    current_volume_real += config.use_real_volume ? (delta_volume / static_cast<double>(volume_scale)) : 0.0;
                    break;
                case 4:
                    ticks[i].set_flag(dfh::TickUpdateFlags::BID_UPDATED);
                    ticks[i].set_flag(dfh::TickUpdateFlags::ASK_UPDATED);
                    ticks[i].set_flag(dfh::TickUpdateFlags::LAST_UPDATED);
                    ticks[i].set_flag(dfh::TickUpdateFlags::VOLUME_UPDATED);
                    current_bid += delta_bid / static_cast<double>(price_scale);
                    current_ask += delta_ask / static_cast<double>(price_scale);
                    current_last += delta_last / static_cast<double>(price_scale);
                    current_volume += config.use_real_volume ? 0 : delta_volume;
                    current_volume_real += config.use_real_volume ? (delta_volume / static_cast<double>(volume_scale)) : 0.0;
                    break;
            }
        }
        current_time_ms += delta_time_ms;
        current_received_ms = current_time_ms + time_shield::MS_PER_SEC;

        // Populate the tick
        ticks[i].bid = dfh::normalize_double(current_bid, config.price_digits);
        ticks[i].ask = dfh::normalize_double(current_ask, config.price_digits);
        ticks[i].last = dfh::normalize_double(current_last, config.price_digits);
        ticks[i].volume = config.use_real_volume ? 0 : current_volume;
        ticks[i].volume_real = config.use_real_volume ?
            dfh::normalize_double(current_volume_real, config.volume_digits) : 0.0;
        ticks[i].time_ms = current_time_ms;
        ticks[i].received_ms = config.use_received_ms ? current_received_ms : 0;

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

    // Decode ticks
    dfh::TickEncodingConfig decoded_config;
    codec.set_config(decoded_config);

    auto start_decode = std::chrono::high_resolution_clock::now();
    std::vector<dfh::MarketTick> decoded_ticks = codec.decode(binary_data);
    auto end_decode = std::chrono::high_resolution_clock::now();

    auto duration_decode = std::chrono::duration_cast<std::chrono::milliseconds>(end_decode - start_decode);

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
                      << ", Last=" << orig.last << ", Volume=" << orig.volume << ", VolumeR=" << orig.volume_real
                      << ", Time=" << orig.time_ms << ", Received=" << orig.received_ms
                      << ", Flags=" << orig.flags << "\n";
            std::cout << "  Decoded:  Bid=" << decoded.bid << ", Ask=" << decoded.ask
                      << ", Last=" << decoded.last << ", Volume=" << decoded.volume << ", VolumeR=" << decoded.volume_real
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
    config.price_digits = 5;
    config.volume_digits = 3;
    config.use_real_volume = true;
    config.use_tick_flags = false;
    config.use_received_ms = true;

    const size_t num_ticks = 2000;
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
        /*
        {2, 2, 1, 10},
        {2, 1, 2, 10},
        {1, 2, 2, 10},
        {1, 1, 1, 10},
        {1, 2, 1, 10},
        {2, 1, 1, 10},
        {1, 1, 2, 10},
        {2, 2, 2, 10},
        {1, 1, 2, 10},
        {4, 4, 4, 10},
        {2, 2, 2, 10},
        */
    };

    std::vector<dfh::MarketTick> ticks = generate_random_ticks_with_deltas(num_ticks, config, delta_spec);

    execute_test("Test 1: Full-featured (Flags, Real Volume, Received MS)", ticks, config);
}

// Main function to run all tests
int main() {
    test_full_featured();
    //test_integer_volume();
    //test_no_flags_no_received_ms();
    //test_dynamic_delta_transitions();

    return 0;
}

/*
int main() {
    // Seed random number generator
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // Generate random ticks
    const size_t num_ticks = 10;
    std::vector<dfh::MarketTick> original_ticks(num_ticks);
    for (size_t i = 0; i < num_ticks; ++i) {
        original_ticks[i].bid = (std::rand() % 10000) / 100.0; // Random bid price
        original_ticks[i].ask = original_ticks[i].bid + (std::rand() % 100) / 1000.0; // Random ask price
        original_ticks[i].last = (std::rand() % 10000) / 100.0; // Random last price
        original_ticks[i].volume = std::rand() % 1000; // Random volume
        original_ticks[i].time_ms = 1000000 + i * 1000; // Incremental time in ms
        original_ticks[i].received_ms = original_ticks[i].time_ms + 5; // Received slightly after
        original_ticks[i].set_flag(dfh::TickUpdateFlags::BID_UPDATED);
        original_ticks[i].set_flag(dfh::TickUpdateFlags::ASK_UPDATED);
        original_ticks[i].set_flag(dfh::TickUpdateFlags::LAST_UPDATED);
        original_ticks[i].set_flag(dfh::TickUpdateFlags::VOLUME_UPDATED);
        if (i == 0) std::cout << "+time: " << original_ticks[i].time_ms << std::endl;
        if (i == num_ticks - 1) std::cout << "++time: " << original_ticks[i].time_ms << std::endl;
    }

    // Encoding configuration
    dfh::TickEncodingConfig config;
    config.price_digits = 2;
    config.volume_digits = 0;
    config.use_tick_flags = true;
    config.use_received_ms = false;

    // Encode ticks
    std::vector<uint8_t> binary_data = dfh::encode_market_ticks(original_ticks, config);

    // Decode ticks
    std::vector<dfh::MarketTick> decoded_ticks = dfh::decode_market_ticks(binary_data, config);

    // Compare and output results
    std::cout << std::fixed << std::setprecision(2);
    bool all_equal = true;
    for (size_t i = 0; i < original_ticks.size(); ++i) {
        const auto& orig = original_ticks[i];
        const auto& decoded = decoded_ticks[i];

        std::cout << "Tick " << i + 1 << ":\n";
        std::cout << "  Original: Bid=" << orig.bid << ", Ask=" << orig.ask
                  << ", Last=" << orig.last << ", Volume=" << orig.volume
                  << ", Time=" << orig.time_ms << ", Received=" << orig.received_ms << ", "
                  << ", Flags=" << orig.flags << "\n";
        std::cout << "  Decoded:  Bid=" << decoded.bid << ", Ask=" << decoded.ask
                  << ", Last=" << decoded.last << ", Volume=" << decoded.volume
                  << ", Time=" << decoded.time_ms << ", Received=" << decoded.received_ms << ", "
                  << ", Flags=" << decoded.flags << "\n";

        if (!dfh::compare_ticks(orig, decoded, config)) {
            all_equal = false;
            std::cout << "  --> Mismatch detected!\n";
        }
        std::cout << "\n";
    }

    if (all_equal) {
        std::cout << "All ticks match successfully.\n";
    } else {
        std::cout << "Errors found in tick decoding.\n";
    }

    return 0;
}
*/

/*
// Utility function to generate random ticks
std::vector<dfh::MarketTick> generate_random_ticks(size_t num_ticks, bool use_received_ms) {
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    std::vector<dfh::MarketTick> ticks(num_ticks);
    for (size_t i = 0; i < num_ticks; ++i) {
        ticks[i].bid = (std::rand() % 10000) / 100.0;
        ticks[i].ask = ticks[i].bid + (std::rand() % 100) / 1000.0;
        ticks[i].last = (std::rand() % 10000) / 100.0;
        ticks[i].volume = std::rand() % 1000;
        ticks[i].volume_real = ticks[i].volume + (std::rand() % 100) / 1000.0;
        ticks[i].time_ms = 1000000 + i * 1000;
        ticks[i].received_ms = use_received_ms ? ticks[i].time_ms + 5 : 0;

        ticks[i].set_flag(dfh::TickUpdateFlags::BID_UPDATED);
        ticks[i].set_flag(dfh::TickUpdateFlags::ASK_UPDATED);
        ticks[i].set_flag(dfh::TickUpdateFlags::LAST_UPDATED);
        ticks[i].set_flag(dfh::TickUpdateFlags::VOLUME_UPDATED);
    }
    return ticks;
}
*/

