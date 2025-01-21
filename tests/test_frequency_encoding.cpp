/// \file test_frequency_encoding.cpp
/// \brief Demonstrates and tests the DFH library's frequency encoding/decoding functions.

#include <iostream>
#include <vector>
#include <random>
#include <cassert>
#include <DataFeedHub/src/compression/utils/frequency_encoding.hpp>

/// \brief Generates a random array of uint32_t.
/// \param size Number of elements to generate.
/// \param min_value Minimum possible value.
/// \param max_value Maximum possible value.
/// \return A std::vector<uint32_t> with random values in [min_value, max_value].
std::vector<uint32_t> generate_random_uint32_array(size_t size, uint32_t min_value, uint32_t max_value) {
    static std::mt19937_64 rng(12345ULL); // Фиксированный seed для воспроизводимости
    std::uniform_int_distribution<uint32_t> dist(min_value, max_value);

    std::vector<uint32_t> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = dist(rng);
    }
    return data;
}

/// \brief Compares two arrays for equality.
/// \param a The first array.
/// \param b The second array.
/// \return true if arrays are equal, false otherwise.
template <typename T>
bool arrays_equal(const std::vector<T>& a, const std::vector<T>& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) {
            std::cout << i << " " << a[i] << " " << b[i] << "\n";
            return false;
        }
    }
    return true;
}

/// \brief Test routine for frequency encoding and decoding.
/// \param size Number of elements in the input data.
void test_frequency_encoding(size_t size) {
    std::cout << "[Test Frequency Encoding] size = " << size << "\n";

    // Generate random data
    auto input_values = generate_random_uint32_array(size, 1, 10); // Частотное кодирование на данных с ограниченным диапазоном

    // Step 1: Frequency encoding
    std::vector<uint32_t> encoded_values(size);
    std::vector<uint32_t> sorted_values;
    std::vector<uint32_t> sorted_to_index_map;
    dfh::compression::encode_frequency(
        input_values.data(),
        encoded_values.data(),
        size,
        sorted_values,
        sorted_to_index_map
    );

    // Step 2: Decode using uint32 -> uint32
    std::vector<uint32_t> decoded_values(size);
    std::vector<uint32_t> code_to_value(sorted_values.size());
    dfh::compression::decode_frequency(
        encoded_values.data(),
        decoded_values.data(),
        size,
        code_to_value.data(),
        sorted_values.data(),
        sorted_to_index_map.data(),
        sorted_values.size()
    );
    assert(arrays_equal(input_values, decoded_values));
    std::cout << "  => uint32 -> uint32 decoding: OK\n";

    // Step 3: Decode using uint32 -> uint64
    std::vector<uint64_t> decoded_values64(size);
    std::vector<uint64_t> code_to_value64(sorted_values.size());
    std::vector<uint64_t> sorted_values64(sorted_values.begin(), sorted_values.end());
    dfh::compression::decode_frequency(
        encoded_values.data(),
        decoded_values64.data(),
        size,
        code_to_value64.data(),
        sorted_values64.data(),
        sorted_to_index_map.data(),
        sorted_values.size()
    );
    assert(arrays_equal(std::vector<uint64_t>(input_values.begin(), input_values.end()), decoded_values64));
    std::cout << "  => uint32 -> uint64 decoding: OK\n";

    // Step 4: Decode using uint64 -> uint64
    std::vector<uint64_t> encoded_values64(encoded_values.begin(), encoded_values.end());
    std::vector<uint64_t> decoded_values64_2(size);
    dfh::compression::decode_frequency(
        encoded_values64.data(),
        decoded_values64_2.data(),
        size,
        code_to_value64.data(),
        sorted_values64.data(),
        sorted_to_index_map.data(),
        sorted_values.size()
    );
    assert(arrays_equal(decoded_values64, decoded_values64_2));
    std::cout << "  => uint64 -> uint64 decoding: OK\n";

    std::cout << "[Test Frequency Encoding] Passed\n\n";
}

/// \brief Entry point: run all tests with various sizes.
int main() {
    // Test frequency encoding/decoding with various sizes
    test_frequency_encoding(1);     // Минимальный размер
    test_frequency_encoding(10);    // Маленький размер
    test_frequency_encoding(50);    // Средний размер
    test_frequency_encoding(1000);  // Большой массив

    std::cout << "All tests passed successfully!\n";
    return 0;
}
