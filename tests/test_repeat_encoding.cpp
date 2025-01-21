#include <iostream>
#include <vector>
#include <cassert>
#include <random>
#include <cstdint>
#include <limits>
#include <DataFeedHub/src/compression/utils/repeat_encoding.hpp>

/// \brief Генерирует случайный массив с повторяющимися значениями.
/// \param size Размер массива.
/// \param max_value Максимальное значение элемента.
/// \param repeat_probability Вероятность, что следующий элемент будет повтором предыдущего.
/// \return Вектор случайных значений.
std::vector<uint32_t> generate_random_with_repeats(
    size_t size, uint32_t max_value, double repeat_probability) {
    std::mt19937 rng(12345);
    std::uniform_int_distribution<uint32_t> value_dist(0, max_value);
    std::bernoulli_distribution repeat_dist(repeat_probability);

    std::vector<uint32_t> data(size);
    data[0] = value_dist(rng);
    for (size_t i = 1; i < size; ++i) {
        if (repeat_dist(rng)) {
            data[i] = data[i - 1];
        } else {
            data[i] = value_dist(rng);
        }
    }
    return data;
}

/// \brief Тестирует функции encode_with_repeats и decode_with_repeats.
/// \param input_size Размер входного массива.
/// \param bits Количество бит, используемых для кодирования значений.
void test_repeat_encoding(size_t input_size, size_t bits) {
    // Сгенерировать входные данные
    auto original = generate_random_with_repeats(input_size, (1UL << bits) - 1, 0.7);

    // Кодирование
    std::vector<uint32_t> encoded(original.size() * 2); // Предполагаем запас места
    size_t encoded_size = 0;
    dfh::compression::encode_with_repeats(original.data(), original.size(), bits, encoded.data(), encoded_size);
    encoded.resize(encoded_size);

    // Декодирование
    std::vector<uint32_t> decoded(original.size() * 2);
    size_t decoded_size = 0;
    dfh::compression::decode_with_repeats(encoded.data(), encoded.size(), bits, decoded.data(), decoded_size);
    decoded.resize(decoded_size);

    // Проверка
    assert(original.size() == decoded.size());
    for (size_t i = 0; i < original.size(); ++i) {
        assert(original[i] == decoded[i]);
    }

    std::cout << "Test passed for input size = " << input_size
              << ", bits = " << bits
              << ", encoded size = " << encoded_size << "\n";
}

/// \brief Точка входа для тестирования.
int main() {
    std::cout << "Testing repeat encoding...\n";

    // Тесты с разным размером входных данных и количеством бит
    test_repeat_encoding(1000, 8);
    test_repeat_encoding(1000, 12);
    test_repeat_encoding(1000, 16);
    test_repeat_encoding(10000, 8);
    test_repeat_encoding(10000, 12);
    test_repeat_encoding(10000, 16);

    std::cout << "All tests passed!\n";
    return 0;
}
