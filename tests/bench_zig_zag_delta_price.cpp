/// \file bench_zig_zag_delta_price.cpp
/// \brief Microbenchmark for price delta ZigZag encode/decode backends.

#include <chrono>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

#include <immintrin.h>

#include "DataFeedHub/data/ticks/TradeTick.hpp"
#include "DataFeedHub/compression/utils/zig_zag_delta.hpp"
#include "DataFeedHub/utils/aligned_allocator.hpp"

namespace {

template <typename T>
using AlignedVec64 = std::vector<T, dfh::utils::aligned_allocator<T, 64>>;

struct BenchResult {
    std::string name;
    std::size_t n{};
    std::size_t iterations{};
    std::size_t repeats{};
    double ns_per_elem{};
    double p95_ns_per_elem{};
    double melem_per_s{};
};

struct BenchCase {
    std::string name;
    std::function<void(std::size_t)> fn;
};

struct BenchTick {
    std::int64_t time_ms{};
    double price{};
    std::uint64_t id{};
    std::uint64_t trade_id() const noexcept { return id; }
    void set_trade_id(std::uint64_t v) noexcept { id = v; }
};

constexpr std::chrono::milliseconds BENCH_MIN_TIME{250};
constexpr std::size_t BENCH_INTERLEAVED_REPEATS{12};
constexpr std::size_t BENCH_U64_DECODE_MICRO_REPEATS{20};
volatile std::uint64_t g_sink = 0;

std::size_t calibrate_iterations(const BenchCase& bc) {
    std::size_t it = 1;
    while (true) {
        const auto start = std::chrono::steady_clock::now();
        bc.fn(it);
        const auto end = std::chrono::steady_clock::now();
        if (end - start >= BENCH_MIN_TIME || it > (1u << 20)) return it;
        it *= 2;
    }
}

BenchResult run_case(const BenchCase& bc, std::size_t n) {
    const std::size_t it = calibrate_iterations(bc);
    const auto start = std::chrono::steady_clock::now();
    bc.fn(it);
    const auto end = std::chrono::steady_clock::now();
    const double elapsed_s = std::chrono::duration<double>(end - start).count();
    const double elems = static_cast<double>(it) * static_cast<double>(n);

    BenchResult r;
    r.name = bc.name;
    r.n = n;
    r.iterations = it;
    r.repeats = 1;
    r.ns_per_elem = (elapsed_s * 1e9) / elems;
    r.p95_ns_per_elem = r.ns_per_elem;
    r.melem_per_s = elems / elapsed_s / 1e6;
    return r;
}

double calc_median(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    const std::size_t mid = sorted.size() / 2;
    if ((sorted.size() & 1u) != 0u) return sorted[mid];
    return 0.5 * (sorted[mid - 1] + sorted[mid]);
}

double calc_p95(const std::vector<double>& values) {
    if (values.empty()) return 0.0;
    std::vector<double> sorted = values;
    std::sort(sorted.begin(), sorted.end());
    const std::size_t idx = static_cast<std::size_t>(
        std::ceil(0.95 * static_cast<double>(sorted.size() - 1)));
    return sorted[idx];
}

std::vector<BenchResult> run_cases_interleaved(
        const std::vector<BenchCase>& cases,
        std::size_t n,
        std::size_t repeats,
        std::size_t iteration_divisor = 4) {
    if (cases.empty()) return {};

    std::vector<std::size_t> calibrated_iters;
    calibrated_iters.reserve(cases.size());
    for (const auto& c : cases) {
        calibrated_iters.push_back(calibrate_iterations(c));
    }

    const std::size_t min_calibrated = *std::min_element(calibrated_iters.begin(), calibrated_iters.end());
    const std::size_t it = std::max<std::size_t>(1, min_calibrated / std::max<std::size_t>(1, iteration_divisor));

    std::vector<std::vector<double>> samples(cases.size());
    for (std::size_t r = 0; r < repeats; ++r) {
        for (std::size_t k = 0; k < cases.size(); ++k) {
            const std::size_t idx = (k + r) % cases.size();
            const auto start = std::chrono::steady_clock::now();
            cases[idx].fn(it);
            const auto end = std::chrono::steady_clock::now();
            const double elapsed_s = std::chrono::duration<double>(end - start).count();
            const double elems = static_cast<double>(it) * static_cast<double>(n);
            samples[idx].push_back((elapsed_s * 1e9) / elems);
        }
    }

    std::vector<BenchResult> rows;
    rows.reserve(cases.size());
    for (std::size_t i = 0; i < cases.size(); ++i) {
        BenchResult r;
        r.name = cases[i].name;
        r.n = n;
        r.iterations = it;
        r.repeats = repeats;
        r.ns_per_elem = calc_median(samples[i]);
        r.p95_ns_per_elem = calc_p95(samples[i]);
        r.melem_per_s = (r.ns_per_elem > 0.0) ? (1e3 / r.ns_per_elem) : 0.0;
        rows.push_back(r);
    }
    return rows;
}

void print_table(const std::string& title, const std::vector<BenchResult>& rows) {
    std::cout << "\n[" << title << "]\n";
    std::cout << std::left << std::setw(36) << "impl"
              << std::right << std::setw(12) << "n"
              << std::setw(14) << "iters"
              << std::setw(10) << "rep"
              << std::setw(16) << "median ns"
              << std::setw(16) << "p95 ns"
              << std::setw(16) << "Melem/s" << '\n';
    std::cout << std::string(110, '-') << '\n';
    for (const auto& r : rows) {
        std::cout << std::left << std::setw(36) << r.name
                  << std::right << std::setw(12) << r.n
                  << std::setw(14) << r.iterations
                  << std::setw(10) << r.repeats
                  << std::setw(16) << std::fixed << std::setprecision(3) << r.ns_per_elem
                  << std::setw(16) << std::fixed << std::setprecision(3) << r.p95_ns_per_elem
                  << std::setw(16) << std::fixed << std::setprecision(2) << r.melem_per_s
                  << '\n';
    }
}

AlignedVec64<BenchTick> make_tick_series(std::size_t n) {
    AlignedVec64<BenchTick> ticks(n);
    std::int64_t ts = 1'700'000'000'000LL;
    std::uint64_t id = 1'000'000ULL;
    for (std::size_t i = 0; i < n; ++i) {
        ticks[i].time_ms = ts;
        ticks[i].price = 1000.0 + 25.0 * std::sin(static_cast<double>(i) * 0.15) + 0.15 * static_cast<double>(i);
        ticks[i].id = id;
        ts += 5 + static_cast<std::int64_t>(i % 4);
        id += 1 + static_cast<std::uint64_t>(i % 3);
    }
    return ticks;
}

void bench_price_delta(std::size_t n) {
    auto ticks = make_tick_series(n);
    AlignedVec64<BenchTick> out(n);
    AlignedVec64<std::uint32_t> d32(n);
    AlignedVec64<std::uint64_t> d64(n);

    constexpr double scale32 = 100.0;
    constexpr std::int64_t init32 = 100'000;
    constexpr double scale64 = 100'000.0;
    constexpr std::int64_t init64 = 10'000'000'000LL;

    // Warm up encoded buffers once before decode benches.
    (void)dfh::compression::encode_price_delta_zig_zag_u32<BenchTick, &BenchTick::price>(
        ticks.data(), d32.data(), n, scale32, init32);
    dfh::compression::encode_price_delta_zig_zag_u64<BenchTick, &BenchTick::price>(
        ticks.data(), d64.data(), n, scale64, init64);

    auto make_enc32_case = [&](const std::string& name, auto fn) {
        return BenchCase{name, [&](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                const bool ok = fn(ticks.data(), d32.data(), n, scale32, init32);
                g_sink ^= static_cast<std::uint64_t>(ok);
            }
        }};
    };
    auto make_dec32_case = [&](const std::string& name, auto fn) {
        return BenchCase{name, [&](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                fn(d32.data(), out.data(), n, scale32, init32);
                g_sink ^= static_cast<std::uint64_t>(out[n / 2].price);
            }
        }};
    };
    auto make_enc64_case = [&](const std::string& name, auto fn) {
        return BenchCase{name, [&](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                fn(ticks.data(), d64.data(), n, scale64, init64);
                g_sink ^= d64[n / 2];
            }
        }};
    };
    auto make_dec64_case = [&](const std::string& name, auto fn) {
        return BenchCase{name, [&](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                fn(d64.data(), out.data(), n, scale64, init64);
                g_sink ^= static_cast<std::uint64_t>(out[n / 2].price);
            }
        }};
    };

    std::vector<BenchCase> enc32_cases;
    enc32_cases.push_back(make_enc32_case("scalar", dfh::compression::encode_price_delta_zig_zag_scalar_u32<BenchTick, &BenchTick::price>));
#if defined(__SSE2__)
    enc32_cases.push_back(make_enc32_case("sse2", dfh::compression::encode_price_delta_zig_zag_sse2_u32<BenchTick, &BenchTick::price>));
#endif
#if defined(__AVX2__)
    enc32_cases.push_back(make_enc32_case("avx2", dfh::compression::encode_price_delta_zig_zag_avx2_u32<BenchTick, &BenchTick::price>));
#endif
#if defined(__AVX512F__)
    enc32_cases.push_back(make_enc32_case("avx512", dfh::compression::encode_price_delta_zig_zag_avx512_u32<BenchTick, &BenchTick::price>));
#endif
    enc32_cases.push_back(make_enc32_case("dispatcher", dfh::compression::encode_price_delta_zig_zag_u32<BenchTick, &BenchTick::price>));
    std::vector<BenchResult> enc32_rows = run_cases_interleaved(enc32_cases, n, BENCH_INTERLEAVED_REPEATS);
    print_table("price delta encode u32 backends", enc32_rows);

    std::vector<BenchCase> dec32_cases;
    dec32_cases.push_back(make_dec32_case("scalar", dfh::compression::decode_price_delta_zig_zag_scalar_u32<BenchTick, &BenchTick::price>));
#if defined(__SSE2__)
    dec32_cases.push_back(make_dec32_case("sse2", dfh::compression::decode_price_delta_zig_zag_sse2_u32<BenchTick, &BenchTick::price>));
#endif
#if defined(__AVX2__)
    dec32_cases.push_back(make_dec32_case("avx2", dfh::compression::decode_price_delta_zig_zag_avx2_u32<BenchTick, &BenchTick::price>));
#endif
#if defined(__AVX512F__)
    dec32_cases.push_back(make_dec32_case("avx512", dfh::compression::decode_price_delta_zig_zag_avx512_u32<BenchTick, &BenchTick::price>));
#endif
    dec32_cases.push_back(make_dec32_case("dispatcher", dfh::compression::decode_price_delta_zig_zag_u32<BenchTick, &BenchTick::price>));
    std::vector<BenchResult> dec32_rows = run_cases_interleaved(dec32_cases, n, BENCH_INTERLEAVED_REPEATS);
    print_table("price delta decode u32 backends", dec32_rows);

    std::vector<BenchCase> enc64_cases;
    enc64_cases.push_back(make_enc64_case("scalar", dfh::compression::encode_price_delta_zig_zag_scalar_u64<BenchTick, &BenchTick::price>));
#if defined(__AVX2__)
    enc64_cases.push_back(make_enc64_case("avx2", dfh::compression::encode_price_delta_zig_zag_avx2_u64<BenchTick, &BenchTick::price>));
#endif
#if defined(__AVX512F__)
    enc64_cases.push_back(make_enc64_case("avx512", dfh::compression::encode_price_delta_zig_zag_avx512_u64<BenchTick, &BenchTick::price>));
#endif
    enc64_cases.push_back(make_enc64_case("dispatcher", dfh::compression::encode_price_delta_zig_zag_u64<BenchTick, &BenchTick::price>));
    std::vector<BenchResult> enc64_rows = run_cases_interleaved(enc64_cases, n, BENCH_INTERLEAVED_REPEATS);
    print_table("price delta encode u64 backends", enc64_rows);

    std::vector<BenchCase> dec64_cases;
    dec64_cases.push_back(make_dec64_case("scalar", dfh::compression::decode_price_delta_zig_zag_scalar_u64<BenchTick, &BenchTick::price>));
#if defined(__SSE2__)
    dec64_cases.push_back(make_dec64_case("sse2", dfh::compression::decode_price_delta_zig_zag_sse2_u64<BenchTick, &BenchTick::price>));
#endif
#if defined(__AVX2__)
    dec64_cases.push_back(make_dec64_case("avx2", dfh::compression::decode_price_delta_zig_zag_avx2_u64<BenchTick, &BenchTick::price>));
#endif
#if defined(__AVX512F__)
    dec64_cases.push_back(make_dec64_case("avx512", dfh::compression::decode_price_delta_zig_zag_avx512_u64<BenchTick, &BenchTick::price>));
#endif
    dec64_cases.push_back(make_dec64_case("dispatcher", dfh::compression::decode_price_delta_zig_zag_u64<BenchTick, &BenchTick::price>));
    std::vector<BenchResult> dec64_rows = run_cases_interleaved(dec64_cases, n, BENCH_INTERLEAVED_REPEATS);
    print_table("price delta decode u64 backends", dec64_rows);

    std::vector<BenchResult> dec64_micro_rows =
        run_cases_interleaved(dec64_cases, n, BENCH_U64_DECODE_MICRO_REPEATS, 2);
    print_table("price delta decode u64 microbench (interleaved)", dec64_micro_rows);
}

} // namespace

int main() {
    const std::vector<std::size_t> sizes = {10'000, 50'000, 100'000};
    std::cout << "bench_zig_zag_delta_price\n";
#if defined(__AVX2__)
    std::cout << "build ISA: AVX2 enabled\n";
#elif defined(__SSE2__)
    std::cout << "build ISA: SSE2 enabled\n";
#else
    std::cout << "build ISA: scalar only\n";
#endif

    for (const std::size_t n : sizes) {
        std::cout << "\n=== N = " << n << " ===\n";
        bench_price_delta(n);
    }

    std::cout << "\nsink=" << g_sink << '\n';
    return 0;
}
