#include <iostream>
#include <vector>
#include <random>
#include <cassert>
#include <cstdlib>
#include <DataFeedHub/src/utils/vbyte.hpp>
#include <DataFeedHub/src/utils/simdcomp.hpp>

template<class T>
bool arrays_equal(const T& original, const T& decoded) {
    if (original.size() != decoded.size()) return false;
    for (size_t i = 0; i < original.size(); ++i) {
        if (original[i] != decoded[i]) {
            std::cerr << "Array test FAILED at i=" << i
                      << " original=" << original[i]
                      << " decoded=" << decoded[i] << "\n";
            return false;
        }
    }
    return true;
}

void test_vbyte_single_u32() {
    std::cout << "[vbyte_single_u32]\n";
    std::vector<uint8_t> data;
    dfh::utils::append_vbyte<uint32_t>(data, 42u);
    dfh::utils::append_vbyte<uint32_t>(data, 123456789u);
    std::cout << "data size: " << data.size() << "\n";

    size_t offset = 0;
    uint32_t val1 = dfh::utils::extract_vbyte<uint32_t>(data.data(), offset);
    uint32_t val2 = dfh::utils::extract_vbyte<uint32_t>(data.data(), offset);

    bool ok = (val1 == 42u && val2 == 123456789u);
    if (!ok) std::cerr << "Single-value test FAILED: got " << val1 << ", " << val2 << "\n";
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}

void test_vbyte_single_u64() {
    std::cout << "[vbyte_single_u64]\n";
    std::vector<uint8_t> data;
    dfh::utils::append_vbyte<uint64_t>(data, 42ull);
    dfh::utils::append_vbyte<uint64_t>(data, 123456789ull);
    std::cout << "data size: " << data.size() << "\n";

    size_t offset = 0;
    uint64_t val1 = dfh::utils::extract_vbyte<uint64_t>(data.data(), offset);
    uint64_t val2 = dfh::utils::extract_vbyte<uint64_t>(data.data(), offset);

    bool ok = (val1 == 42ull && val2 == 123456789ull);
    if (!ok) std::cerr << "Single-value test FAILED: got " << val1 << ", " << val2 << "\n";
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}

void test_vbyte_array_u32() {
    std::cout << "[vbyte_array_u32]\n";
    const size_t N = 1000;
    std::vector<uint32_t> original(N);

    std::mt19937 rng(12345); // фиксированный seed, чтобы всегда одни и те же данные
    std::uniform_int_distribution<uint32_t> dist(0, 1000000);
    for (size_t i = 0; i < N; ++i) {
        original[i] = dist(rng);
    }

    // Пакуем в двоичный буфер
    std::vector<uint8_t> data;
    dfh::utils::append_vbyte<uint32_t>(data, original.data(), N);
    std::cout << "data size: " << data.size() << "\n";

    // Декодируем обратно
    std::vector<uint32_t> decoded(N);
    size_t offset = 0;
    dfh::utils::extract_vbyte(data.data(), offset, decoded.data(), N);

    // Сравниваем
    bool ok = arrays_equal(original, decoded);
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}

void test_vbyte_array_u64() {
    std::cout << "[vbyte_array_u64]\n";
    const size_t N = 1000;
    std::vector<uint64_t> original(N);

    std::mt19937_64 rng(67890);
    std::uniform_int_distribution<uint64_t> dist(0, 1000000000ULL);
    for (size_t i = 0; i < N; ++i) {
        original[i] = dist(rng);
    }

    // Пакуем
    std::vector<uint8_t> data;
    dfh::utils::append_vbyte<uint64_t>(data, original.data(), N);
    std::cout << "data size: " << data.size() << "\n";

    // Декодируем
    std::vector<uint64_t> decoded(N);
    size_t offset = 0;
    dfh::utils::extract_vbyte(data.data(), offset, decoded.data(), N);

    // Сравниваем
    bool ok = arrays_equal(original, decoded);
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}

// Тест simdcomp 1: Используем append_simdcomp(..., bit) + extract_simdcomp(..., bit)
// Здесь мы сами выбираем bit, а значения генерируем так, чтобы точно влезали.
void test_simdcomp_fixed_bit() {
    std::cout << "[simdcomp_fixed_bit]\n";
    // 1. Подготовим данные
    const size_t length = 200; // спецом некратно 128
    const uint32_t chosen_bit = 10; // значит, значения не больше 2^10 - 1 = 1023

    std::vector<uint32_t> original(length);
    std::mt19937 rng(12345);
    std::uniform_int_distribution<uint32_t> dist(0, (1u << chosen_bit) - 1);
    for (size_t i = 0; i < length; ++i) {
        original[i] = dist(rng);
    }

    // 2. Упаковываем
    std::vector<uint8_t> buffer;
    buffer.reserve(4096); // чтобы избежать лишних реаллокаций
    dfh::utils::append_simdcomp(buffer, original.data(), length, chosen_bit);

    // 3. Распаковываем
    std::vector<uint32_t> decoded(length);
    size_t offset = 0;
    dfh::utils::extract_simdcomp(buffer.data(), offset, decoded.data(), length, chosen_bit);

    // 4. Сравниваем
    bool ok = arrays_equal(original, decoded);
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}


// Тест simdcomp 2: Используем append_simdcomp(...) без параметра bit -> он вычисляется внутри.
// Аналогично, распаковываем extract_simdcomp(...) (вторую перегрузку).
static void test_simdcomp_auto_bit() {
    std::cout << "[simdcomp_auto_bit]\n";

    // 1. Сгенерируем произвольные числа, вплоть до 32-бит.
    //    Но для наглядности пусть будут разбросаны где-то до 1<<23 (пример).
    const size_t length = 300; // снова некратное 128 (2 полных блока = 256, + 44 leftover)
    std::vector<uint32_t> original(length);
    std::mt19937 rng(67890);
    std::uniform_int_distribution<uint32_t> dist(0, (1u << 23) - 1);
    for (size_t i = 0; i < length; ++i) {
        original[i] = dist(rng);
    }

    // 2. Упаковываем, пусть функция сама блок за блоком высчитывает bit = maxbits(...)
    std::vector<uint8_t> buffer;
    buffer.reserve(4096);
    dfh::utils::append_simdcomp(buffer, original.data(), length);

    // 3. Распаковываем
    std::vector<uint32_t> decoded(length);
    size_t offset = 0;
    dfh::utils::extract_simdcomp(buffer.data(), offset, decoded.data(), length);

    // 4. Сравниваем
    bool ok = arrays_equal(original, decoded);
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}


// Тест simdcomp 3: Проверяем случай, когда length < SIMDBlockSize (нет полных блоков, только короткий leftover).
void test_simdcomp_short_length() {
    std::cout << "[simdcomp_short_length]\n";

    // SIMDBlockSize обычно 128, проверим length = 50
    const size_t length = 50;
    std::vector<uint32_t> original(length);
    std::mt19937 rng(99999);
    std::uniform_int_distribution<uint32_t> dist(0, (1u << 16) - 1);
    for (size_t i = 0; i < length; ++i) {
        original[i] = dist(rng);
    }

    // Сжимаем (пусть функция сама считает bit)
    std::vector<uint8_t> buffer;
    buffer.reserve(1024);
    dfh::utils::append_simdcomp(buffer, original.data(), length);

    // Распаковываем
    std::vector<uint32_t> decoded(length);
    size_t offset = 0;
    dfh::utils::extract_simdcomp(buffer.data(), offset, decoded.data(), length);

    // Сравниваем
    bool ok = arrays_equal(original, decoded);
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}

int main() {
    // --------------------------
    // 1) Тест vbyte для одиночных значений
    // --------------------------
    test_vbyte_single_u32();
    test_vbyte_single_u64();

    // -----------------------------------
    // 2) Тест vbyte для массивов значений
    // -----------------------------------
    test_vbyte_array_u32();
    test_vbyte_array_u64();

    // -----------------------------------
    // 3) Тест simdcomp для массивов значений
    // -----------------------------------
    test_simdcomp_fixed_bit();
    test_simdcomp_auto_bit();
    test_simdcomp_short_length();

    std::cout << "\nAll tests PASSED successfully!\n";
    return EXIT_SUCCESS;
}
