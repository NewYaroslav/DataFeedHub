#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <cstdint>
#include <cassert>
#include <immintrin.h> // Для SIMD
#include <DataFeedHub/src/utils/aligned_allocator.hpp>
#include <DataFeedHub/src/utils/sse_double_int64_utils.hpp>

// Структура MarketTick
struct MarketTick {
    double ask;
    double bid;
    double last;
    double volume;
    uint64_t time_ms;
    uint64_t received_ms;
    uint64_t flags;
};

void convert_prices_to_integers_sse2(const MarketTick* ticks, int64_t* output, size_t size, double price_scale) {
    constexpr size_t simd_width = 2;
    size_t aligned_size = size - (size % simd_width);

    __m128d scale_vec = _mm_set1_pd(price_scale);
    __m128d scaled;

    for (size_t i = 0; i < aligned_size; i += simd_width) {
        scaled = _mm_mul_pd(_mm_set_pd(ticks[i + 1].last, ticks[i].last), scale_vec);
        output[i] = _mm_cvtsd_si64(scaled);
        output[i + 1] = _mm_cvtsd_si64(_mm_unpackhi_pd(scaled, scaled));
    }

    for (size_t i = aligned_size; i < size; ++i) {
        output[i] = static_cast<int64_t>(ticks[i].last * price_scale);
    }
}

void convert_integers_to_prices_sse2(const int64_t* input, MarketTick* ticks, size_t size, double price_scale) {
    constexpr size_t simd_width = 2;
    size_t aligned_size = size - (size % simd_width);
    const double inv_price_scale = 1.0 / price_scale;

    __m128d scale_vec = _mm_set1_pd(inv_price_scale), prices;

    for (size_t i = 0; i < aligned_size; i += simd_width) {
        prices = _mm_mul_pd(dfh::utils::int64_to_double(
            _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i]))),
                scale_vec);
        ticks[i].last = _mm_cvtsd_f64(prices);
        ticks[i + 1].last = _mm_cvtsd_f64(_mm_unpackhi_pd(prices, prices));
    }

    for (size_t i = aligned_size; i < size; ++i) {
        ticks[i].last = static_cast<double>(input[i]) * inv_price_scale;
    }
}

//------------------------------------------------------------------------------

void convert_prices_to_integers_aligned_sse2(
        const MarketTick* ticks,
        double* aligned_last_prices,
        int64_t* output, size_t size, double price_scale) {

    constexpr size_t simd_width = 2; // SSE2 обрабатывает 2 double за раз
    size_t aligned_size = size - (size % simd_width);

    for (size_t i = 0; i < aligned_size; i += simd_width) {
        _mm_store_pd(&aligned_last_prices[i], _mm_set_pd(ticks[i + 1].last, ticks[i].last));
    }

    __m128d scale_vec = _mm_set1_pd(price_scale);
    __m128d scaled;

    for (size_t i = 0; i < aligned_size; i += simd_width) {
        scaled = _mm_mul_pd(_mm_load_pd(&aligned_last_prices[i]), scale_vec);
        output[i] = _mm_cvtsd_si64(scaled);
        output[i + 1] = _mm_cvtsd_si64(_mm_unpackhi_pd(scaled, scaled));
    }

    for (size_t i = aligned_size; i < size; ++i) {
        output[i] = static_cast<int64_t>(aligned_last_prices[i] * price_scale);
    }
}

//------------------------------------------------------------------------------

void convert_prices_to_integers(const MarketTick* ticks, int64_t* output, size_t size, double price_scale) {
    for (size_t i = 0; i < size; ++i) {
        output[i] = std::llround(ticks[i].last * price_scale);
    }
}

void convert_integers_to_prices(const int64_t* input, MarketTick* ticks, size_t size, double price_scale) {
    double inv_price_scale = 1.0 / price_scale;
    for (size_t i = 0; i < size; ++i) {
        ticks[i].last = static_cast<double>(input[i]) * inv_price_scale;
    }
}

//------------------------------------------------------------------------------

void convert_prices_to_integers(const MarketTick* ticks, double* prices, int64_t* output, size_t size, double price_scale) {
    for (size_t i = 0; i < size; ++i) {
        prices[i] = ticks[i].last;
    }
    for (size_t i = 0; i < size; ++i) {
        output[i] = std::round(prices[i] * price_scale);
    }
}

void convert_integers_to_prices(const int64_t* input, double* prices, MarketTick* ticks, size_t size, double price_scale) {
    double inv_price_scale = 1.0 / price_scale;
    for (size_t i = 0; i < size; ++i) {
        prices[i] = static_cast<double>(input[i]);
    }
    for (size_t i = 0; i < size; ++i) {
        ticks[i].last = prices[i] * inv_price_scale;
    }
}

//------------------------------------------------------------------------------

void convert_prices_to_deltas_sse2(
    const MarketTick* ticks, int32_t* output, size_t size, double price_scale, double initial_price) {
    constexpr size_t simd_width = 4;
    size_t aligned_size = size - (size % simd_width);

#   if defined(__SSE4_1__)
    __m128d scale_vec = _mm_set1_pd(price_scale);
    __m128d prev_price = _mm_set1_pd(std::round(initial_price * price_scale));
    __m128d scaled1, scaled2;

    for (size_t i = 0; i < aligned_size; i += simd_width) {
        scaled1 = _mm_round_pd(_mm_mul_pd(_mm_set_pd(ticks[i + 1].last, ticks[i].last), scale_vec), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
        scaled2 = _mm_round_pd(_mm_mul_pd(_mm_set_pd(ticks[i + 3].last, ticks[i + 2].last), scale_vec), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);

        _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
            _mm_unpacklo_epi64(
                _mm_cvttpd_epi32(_mm_sub_pd(scaled1, _mm_shuffle_pd(prev_price, scaled1, 1))),
                _mm_cvttpd_epi32(_mm_sub_pd(scaled2, _mm_shuffle_pd(scaled1, scaled2, 1)))));
        prev_price = scaled2;
    }

    int64_t scaled_price, prev = std::llround(initial_price * price_scale);
    for (size_t i = aligned_size; i < size; ++i) {
        scaled_price = std::llround(ticks[i].last * price_scale);
        output[i] = static_cast<int32_t>(scaled_price - prev);
        prev = scaled_price;
    }
#   endif
}

void convert_deltas_to_prices_sse2(
        const int32_t* deltas,  // вход: массив дельт
        MarketTick* ticks,      // выход: здесь восстановим ticks[i].last
        size_t size,
        double price_scale,
        double initial_price) {
    const double inv_price_scale = 1.0 / price_scale;
#   if defined(__SSE4_1__)
    constexpr size_t simd_width = 4;
    const size_t aligned_size = size - (size % simd_width);

    const __m128i zero = _mm_setzero_si128();
    const __m128d zero_pd = _mm_setzero_pd();
    const __m128d scale_vec = _mm_set1_pd(inv_price_scale);

    __m128d prev_scaled = _mm_set1_pd(std::round(initial_price * price_scale));
    __m128d deltas_lo, deltas_hi, scaled1, scaled2, prices;
    __m128i d4;

    for (size_t i = 0; i < aligned_size; i += simd_width) {
        d4 = _mm_load_si128(reinterpret_cast<const __m128i*>(&deltas[i]));
        deltas_lo = _mm_cvtepi32_pd(d4);
        deltas_hi = _mm_cvtepi32_pd(_mm_unpackhi_epi64(d4, zero));

        scaled1 = _mm_add_pd(_mm_add_pd(prev_scaled, deltas_lo), _mm_unpacklo_pd(zero_pd, deltas_lo));
        scaled2 = _mm_add_pd(_mm_add_pd(_mm_unpackhi_pd(scaled1, scaled1), deltas_hi), _mm_unpacklo_pd(zero_pd, deltas_hi));
        prev_scaled = _mm_unpackhi_pd(scaled2, scaled2);

        prices = _mm_mul_pd(scaled1, scale_vec);
        ticks[i].last     = _mm_cvtsd_f64(prices);
        ticks[i + 1].last = _mm_cvtsd_f64(_mm_unpackhi_pd(prices, prices));

        prices = _mm_mul_pd(scaled2, scale_vec);
        ticks[i + 2].last = _mm_cvtsd_f64(prices);
        ticks[i + 3].last = _mm_cvtsd_f64(_mm_unpackhi_pd(prices, prices));
    }

    int64_t scaled_price, prev = aligned_size > 0
        ? std::llround(ticks[aligned_size - 1].last * price_scale)
        : std::llround(initial_price * price_scale);
    for (size_t i = aligned_size; i < size; ++i) {
        scaled_price = prev + static_cast<int64_t>(deltas[i]);
        ticks[i].last = static_cast<double>(scaled_price) * inv_price_scale;
        prev = scaled_price;
    }
#else
    int64_t scaled_price, prev = static_cast<int64_t>(std::llround(initial_price * price_scale));
    for (size_t i = 0; i < size; ++i) {
        scaled_price = prev + deltas[i];
        ticks[i].last = static_cast<double>(scaled_price) * inv_price_scale;
        prev = scaled_price;
    }
#endif
}

void convert_prices_to_deltas_scalar(
    const MarketTick* ticks, int32_t* output, size_t size, double price_scale, double initial_price) {
    int64_t scaled_price, prev = std::llround(initial_price * price_scale);
    for (size_t i = 0; i < size; ++i) {
        scaled_price = std::llround(ticks[i].last * price_scale);
        output[i] = static_cast<int32_t>(scaled_price - prev);
        prev = scaled_price;
    }
}

void convert_deltas_to_prices_scalar(
        const int32_t* deltas,  // вход: массив дельт
        MarketTick* ticks,      // выход: здесь восстановим ticks[i].last
        size_t size,
        double price_scale,
        double initial_price) {
    int64_t prev = static_cast<int64_t>(std::llround(initial_price * price_scale));
    double inv_initial_price = 1.0 / price_scale;
    for (size_t i = 0; i < size; ++i) {
        int64_t scaled_price = prev + deltas[i];
        ticks[i].last = static_cast<double>(scaled_price) * inv_initial_price;
        prev = scaled_price;
    }
}

//------------------------------------------------------------------------------

void convert_deltas_to_prices_zig_zag_sse2(
        const uint32_t* deltas,
        MarketTick* ticks,
        size_t size,
        double price_scale,
        double initial_price) {
    const double inv_price_scale = 1.0 / price_scale;
#   if defined(__SSE4_1__)
    constexpr size_t simd_width = 4;
    const size_t aligned_size = size - (size % simd_width);

    const __m128i one = _mm_set1_epi32(1);
    const __m128i zero = _mm_setzero_si128();
    const __m128d zero_pd = _mm_setzero_pd();
    const __m128d scale_vec = _mm_set1_pd(inv_price_scale);

    __m128d prev_scaled = _mm_set1_pd(std::round(initial_price * price_scale));
    __m128d deltas_lo, deltas_hi, scaled1, scaled2, prices;
    __m128i deltas4, zig_zag;

    for (size_t i = 0; i < aligned_size; i += simd_width) {
        zig_zag = _mm_load_si128(reinterpret_cast<const __m128i*>(&deltas[i]));
        deltas4 = _mm_xor_si128(_mm_srli_epi32(zig_zag, 1), _mm_sub_epi32(zero, _mm_and_si128(zig_zag, one)));

        deltas_lo = _mm_cvtepi32_pd(deltas4);
        deltas_hi = _mm_cvtepi32_pd(_mm_unpackhi_epi64(deltas4, zero));

        scaled1 = _mm_add_pd(_mm_add_pd(prev_scaled, deltas_lo), _mm_unpacklo_pd(zero_pd, deltas_lo));
        scaled2 = _mm_add_pd(_mm_add_pd(_mm_unpackhi_pd(scaled1, scaled1), deltas_hi), _mm_unpacklo_pd(zero_pd, deltas_hi));
        prev_scaled = _mm_unpackhi_pd(scaled2, scaled2);

        prices = _mm_mul_pd(scaled1, scale_vec);
        ticks[i].last     = _mm_cvtsd_f64(prices);
        ticks[i + 1].last = _mm_cvtsd_f64(_mm_unpackhi_pd(prices, prices));

        prices = _mm_mul_pd(scaled2, scale_vec);
        ticks[i + 2].last = _mm_cvtsd_f64(prices);
        ticks[i + 3].last = _mm_cvtsd_f64(_mm_unpackhi_pd(prices, prices));
    }

    int64_t scaled_price, prev = aligned_size > 0
        ? std::llround(ticks[aligned_size - 1].last * price_scale)
        : std::llround(initial_price * price_scale);
    for (size_t i = aligned_size; i < size; ++i) {
        scaled_price = prev + static_cast<int64_t>(deltas[i]);
        ticks[i].last = static_cast<double>(scaled_price) * inv_price_scale;
        prev = scaled_price;
    }
#   else
    int64_t scaled_price, prev = static_cast<int64_t>(std::llround(initial_price * price_scale));
    for (size_t i = 0; i < size; ++i) {
        scaled_price = prev + deltas[i];
        ticks[i].last = static_cast<double>(scaled_price) * inv_price_scale;
        prev = scaled_price;
    }
#   endif
}

void convert_prices_to_deltas_zig_zag_scalar(
        const MarketTick* ticks,
        uint32_t* output,
        size_t size,
        double price_scale,
        double initial_price) {
    int64_t scaled_price, prev = std::llround(initial_price * price_scale);
    int32_t delta;
    for (size_t i = 0; i < size; ++i) {
        scaled_price = std::llround(ticks[i].last * price_scale);
        delta = static_cast<int32_t>(scaled_price - prev);
        output[i] = static_cast<uint32_t>((delta << 1) ^ (delta >> 31));
        prev = scaled_price;
    }
}

void convert_deltas_to_prices_zig_zag_scalar(
        const uint32_t* deltas,
        MarketTick* ticks,
        size_t size,
        double price_scale,
        double initial_price) {
    int64_t scaled_price, prev = static_cast<int64_t>(std::llround(initial_price * price_scale));
    double inv_initial_price = 1.0 / price_scale;
    int32_t delta;
    for (size_t i = 0; i < size; ++i) {
        delta = (deltas[i] >> 1) ^ -(deltas[i] & 1);
        scaled_price = prev + delta;
        ticks[i].last = static_cast<double>(scaled_price) * inv_initial_price;
        prev = scaled_price;
    }
}

//------------------------------------------------------------------------------

// Функция для генерации случайных тиков
std::vector<MarketTick> generate_random_ticks(size_t count) {
    std::vector<MarketTick> ticks(count);
    std::mt19937_64 rng(12345ULL);
    std::uniform_real_distribution<double> price_dist(0.1, 100.0);

    for (auto& tick : ticks) {
        tick.ask = price_dist(rng);
        tick.bid = price_dist(rng);
        tick.last = price_dist(rng);
        tick.volume = price_dist(rng);
        tick.time_ms = rng();
        tick.received_ms = rng();
        tick.flags = 0;
    }
    return ticks;
}

// Шаблонная функция для сравнения времени выполнения
template <typename Func1, typename Func2>
void measure_time(Func1 func1, Func2 func2, size_t iterations = 10) {
    double total_time1 = 0.0;
    double total_time2 = 0.0;

    for (size_t i = 0; i < iterations; ++i) {
        auto start_time1 = std::chrono::high_resolution_clock::now();
        func1();
        auto end_time1 = std::chrono::high_resolution_clock::now();

        auto start_time2 = std::chrono::high_resolution_clock::now();
        func2();
        auto end_time2 = std::chrono::high_resolution_clock::now();

        auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end_time1 - start_time1).count();
        auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count();

        total_time1 += static_cast<double>(duration1);
        total_time2 += static_cast<double>(duration2);
    }

    double average_time1 = total_time1 / iterations;
    double average_time2 = total_time2 / iterations;

    std::cout << "Avg time 1/2: "
              << average_time1 << " us; "
              << average_time2 << " us; "
              << iterations << " iterations." << std::endl;
}

template<class T>
bool arrays_equal(const T& a, const T& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) {
            std::cout << "Input[" << i << "]: " << a[i] << ", Output: " << b[i] << std::endl;
            return false;
        }
    }
    return true;
}

template<class T>
bool arrays_equal(const T& a, const T& b, double price_scale) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        int64_t a_scale = std::llround(a[i].last * price_scale);
        int64_t b_scale = std::llround(b[i].last * price_scale);
        if (std::abs(a_scale - b_scale > 0)) {
            std::cout << "Input[" << i << "]: " << a_scale << ", Output: " << b_scale << std::endl;
            return false;
        }
    }
    return true;
}

/// \brief
void test_convert_prices_to_deltas(size_t tick_count, double price_scale) {
    std::cout << "[convert_prices_to_deltas] size = " << tick_count << ", price_scale = " << price_scale << "\n";

    auto ticks = generate_random_ticks(tick_count);

    std::vector<int32_t, dfh::utils::aligned_allocator<int32_t, 16>> direct_output_scalar(tick_count);
    std::vector<int32_t, dfh::utils::aligned_allocator<int32_t, 16>> direct_output_sse2(tick_count);

    convert_prices_to_deltas_sse2(ticks.data(), direct_output_sse2.data(), tick_count, price_scale, ticks[0].last);
    convert_prices_to_deltas_scalar(ticks.data(), direct_output_scalar.data(), tick_count, price_scale, ticks[0].last);

    // Compare
    bool ok = arrays_equal(direct_output_scalar, direct_output_sse2);
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}

void test_convert_deltas_to_prices(size_t tick_count, double price_scale) {
    std::cout << "[convert_deltas_to_prices] size = " << tick_count << ", price_scale = " << price_scale << "\n";

    auto ticks = generate_random_ticks(tick_count);

    std::vector<int32_t, dfh::utils::aligned_allocator<int32_t, 16>> direct_output_scalar(tick_count);
    //std::vector<int64_t, dfh::utils::aligned_allocator<int64_t, 16>> aligned_buffer(tick_count);

    convert_prices_to_deltas_scalar(ticks.data(), direct_output_scalar.data(), tick_count, price_scale, ticks[0].last);

    std::vector<MarketTick> output_ticks_sse2(tick_count);
    std::vector<MarketTick> output_ticks_scalar(tick_count);
    convert_deltas_to_prices_sse2(direct_output_scalar.data(), output_ticks_sse2.data(), tick_count, price_scale, ticks[0].last);
    convert_deltas_to_prices_scalar(direct_output_scalar.data(), output_ticks_scalar.data(), tick_count, price_scale, ticks[0].last);
    // Compare
    bool ok = arrays_equal(output_ticks_sse2, output_ticks_scalar, price_scale);
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}

void test_speed_convert_deltas_to_prices(size_t tick_count, double price_scale) {
    std::cout << "[speed convert_deltas_to_prices] size = " << tick_count << ", price_scale = " << price_scale << "\n";

    auto ticks = generate_random_ticks(tick_count);

    std::vector<int32_t, dfh::utils::aligned_allocator<int32_t, 16>> direct_output_scalar(tick_count);
    std::vector<int64_t, dfh::utils::aligned_allocator<int64_t, 16>> aligned_buffer(tick_count);

    convert_prices_to_deltas_scalar(ticks.data(), direct_output_scalar.data(), tick_count, price_scale, ticks[0].last);

    std::vector<MarketTick> output_ticks_sse2(tick_count);
    std::vector<MarketTick> output_ticks_scalar(tick_count);

    measure_time([&]() {
        convert_deltas_to_prices_scalar(direct_output_scalar.data(), output_ticks_scalar.data(), tick_count, price_scale, ticks[0].last);
    }, [&]() {
        convert_deltas_to_prices_sse2(direct_output_scalar.data(), output_ticks_sse2.data(), tick_count, price_scale, ticks[0].last);
    }, 1000);
}

void test_convert_deltas_zig_zag_to_prices(size_t tick_count, double price_scale) {
    std::cout << "[convert_deltas_zig_zag_to_prices] size = " << tick_count << ", price_scale = " << price_scale << "\n";

    auto ticks = generate_random_ticks(tick_count);

    std::vector<uint32_t, dfh::utils::aligned_allocator<uint32_t, 16>> direct_output_scalar(tick_count);
    convert_prices_to_deltas_zig_zag_scalar(ticks.data(), direct_output_scalar.data(), tick_count, price_scale, ticks[0].last);

    std::vector<MarketTick> output_ticks_sse2(tick_count);
    std::vector<MarketTick> output_ticks_scalar(tick_count);
    convert_deltas_to_prices_zig_zag_sse2(direct_output_scalar.data(), output_ticks_sse2.data(), tick_count, price_scale, ticks[0].last);
    convert_deltas_to_prices_zig_zag_scalar(direct_output_scalar.data(), output_ticks_scalar.data(), tick_count, price_scale, ticks[0].last);
    // Compare
    bool ok = arrays_equal(output_ticks_sse2, output_ticks_scalar, price_scale);
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}

void test_speed_convert_deltas_zig_zag_to_prices(size_t tick_count, double price_scale) {
    std::cout << "[speed convert_deltas_zig_zag_to_prices] size = " << tick_count << ", price_scale = " << price_scale << "\n";

    auto ticks = generate_random_ticks(tick_count);

    std::vector<uint32_t, dfh::utils::aligned_allocator<uint32_t, 16>> direct_output_scalar(tick_count);
    convert_prices_to_deltas_zig_zag_scalar(ticks.data(), direct_output_scalar.data(), tick_count, price_scale, ticks[0].last);

    std::vector<MarketTick> output_ticks_sse2(tick_count);
    std::vector<MarketTick> output_ticks_scalar(tick_count);

    measure_time([&]() {
        convert_deltas_to_prices_zig_zag_scalar(direct_output_scalar.data(), output_ticks_scalar.data(), tick_count, price_scale, ticks[0].last);
    }, [&]() {
        convert_deltas_to_prices_zig_zag_sse2(direct_output_scalar.data(), output_ticks_sse2.data(), tick_count, price_scale, ticks[0].last);
    }, 1000);
}

int main() {
    const size_t tick_count = 1000000;
    const double price_scale = 10000.0; // Пример масштаба

    test_convert_deltas_zig_zag_to_prices(tick_count, price_scale);
    test_speed_convert_deltas_zig_zag_to_prices(tick_count, price_scale);

    test_convert_prices_to_deltas(tick_count, price_scale);
    test_convert_deltas_to_prices(tick_count, price_scale);
    test_speed_convert_deltas_to_prices(tick_count, price_scale);

    // Генерация случайных тиков
    auto ticks = generate_random_ticks(tick_count);

    // Результаты обработки
    std::vector<int64_t, dfh::utils::aligned_allocator<int64_t, 16>> direct_output(tick_count);
    std::vector<int64_t, dfh::utils::aligned_allocator<int64_t, 16>> aligned_output(tick_count);
    std::vector<double, dfh::utils::aligned_allocator<double, 16>>   aligned_last_prices(tick_count); // Выровненный массив для last цен

    // Сравнение времени выполнения
    std::cout << "convert_prices_to_integers" << std::endl;
    measure_time([&]() {
        convert_prices_to_integers(ticks.data(), aligned_output.data(), tick_count, price_scale);
    }, [&]() {
        convert_prices_to_integers(ticks.data(), aligned_last_prices.data(), aligned_output.data(), tick_count, price_scale);
    }, 1000);

    std::cout << "convert_integers_to_prices" << std::endl;
    measure_time([&]() {
        convert_integers_to_prices(aligned_output.data(), ticks.data(), tick_count, price_scale);
    }, [&]() {
        convert_integers_to_prices(aligned_output.data(), aligned_last_prices.data(), ticks.data(), tick_count, price_scale);
    }, 1000);

    std::cout << "convert_prices_to_integers_sse2" << std::endl;
    measure_time([&]() {
        convert_prices_to_integers(ticks.data(), aligned_output.data(), tick_count, price_scale);
    }, [&]() {
        convert_prices_to_integers_sse2(ticks.data(), aligned_output.data(), tick_count, price_scale);
    }, 1000);

    std::cout << "convert_prices_to_integers_aligned_sse2" << std::endl;
    measure_time([&]() {
        convert_prices_to_integers(ticks.data(), aligned_output.data(), tick_count, price_scale);
    }, [&]() {
        convert_prices_to_integers_aligned_sse2(ticks.data(), aligned_last_prices.data(), aligned_output.data(), tick_count, price_scale);
    }, 1000);

    std::cout << "convert_integers_to_prices_sse2" << std::endl;
    measure_time([&]() {
        convert_integers_to_prices(aligned_output.data(), ticks.data(), tick_count, price_scale);
    }, [&]() {
        convert_integers_to_prices_sse2(aligned_output.data(), ticks.data(), tick_count, price_scale);
    }, 1000);

    return 0;
}
