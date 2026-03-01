/// \file bench_zig_zag_delta_id.cpp
/// \brief Microbenchmark for ID delta ZigZag encode/decode backends.

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

constexpr std::chrono::milliseconds BENCH_MIN_TIME{250};
constexpr std::size_t BENCH_INTERLEAVED_REPEATS{12};
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
    std::cout << std::left << std::setw(32) << "impl"
              << std::right << std::setw(12) << "n"
              << std::setw(14) << "iters"
              << std::setw(10) << "rep"
              << std::setw(16) << "median ns"
              << std::setw(16) << "p95 ns"
              << std::setw(16) << "Melem/s" << '\n';
    std::cout << std::string(106, '-') << '\n';
    for (const auto& r : rows) {
        std::cout << std::left << std::setw(32) << r.name
                  << std::right << std::setw(12) << r.n
                  << std::setw(14) << r.iterations
                  << std::setw(10) << r.repeats
                  << std::setw(16) << std::fixed << std::setprecision(3) << r.ns_per_elem
                  << std::setw(16) << std::fixed << std::setprecision(3) << r.p95_ns_per_elem
                  << std::setw(16) << std::fixed << std::setprecision(2) << r.melem_per_s
                  << '\n';
    }
}

AlignedVec64<dfh::TradeTick> make_trade_ticks(std::size_t n) {
    AlignedVec64<dfh::TradeTick> ticks(n);
    std::uint64_t id = 1'000'000ULL;
    for (std::size_t i = 0; i < n; ++i) {
        ticks[i].set_trade(id, dfh::TradeSide::BUY);
        id += 1 + static_cast<std::uint64_t>(i % 3);
    }
    return ticks;
}

void bench_id_delta(std::size_t n) {
    auto ticks = make_trade_ticks(n);
    AlignedVec64<dfh::TradeTick> out(n);
    AlignedVec64<std::uint32_t> d32(n);
    AlignedVec64<std::uint64_t> d64(n);
    const std::int64_t initial_id = static_cast<std::int64_t>(ticks[0].trade_id() - 1);

    BenchCase enc32_case{"encode_id_delta_u32", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i) {
            const bool ok = dfh::compression::encode_id_delta_u32(ticks.data(), d32.data(), n, initial_id);
            g_sink ^= static_cast<std::uint64_t>(ok);
        }
    }};
    BenchCase dec32_case{"decode_id_delta_u32(dispatcher)", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i) {
            dfh::compression::decode_id_delta_u32(d32.data(), out.data(), n, initial_id);
            g_sink ^= out[n / 2].trade_id();
        }
    }};

    BenchCase enc64_case{"encode_id_delta_u64", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i) {
            dfh::compression::encode_id_delta_u64(ticks.data(), d64.data(), n, initial_id);
            g_sink ^= d64[n / 2];
        }
    }};
    BenchCase dec64_case{"decode_id_delta_u64(dispatcher)", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i) {
            dfh::compression::decode_id_delta_u64(d64.data(), out.data(), n, initial_id);
            g_sink ^= out[n / 2].trade_id();
        }
    }};

    (void)run_case(enc32_case, n);
    (void)run_case(enc64_case, n);
    (void)dfh::compression::encode_id_delta_u32(ticks.data(), d32.data(), n, initial_id);
    dfh::compression::encode_id_delta_u64(ticks.data(), d64.data(), n, initial_id);

    print_table("id delta summary",
                {run_case(enc32_case, n), run_case(dec32_case, n), run_case(enc64_case, n), run_case(dec64_case, n)});

    auto make_decode32_case = [&](const std::string& name,
                                  void (*fn)(const std::uint32_t*, dfh::TradeTick*, std::size_t, std::int64_t)) {
        return BenchCase{name, [&, fn](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                fn(d32.data(), out.data(), n, initial_id);
                g_sink ^= out[n / 2].trade_id();
            }
        }};
    };

    auto make_decode64_case = [&](const std::string& name,
                                  void (*fn)(const std::uint64_t*, dfh::TradeTick*, std::size_t, std::int64_t)) {
        return BenchCase{name, [&, fn](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                fn(d64.data(), out.data(), n, initial_id);
                g_sink ^= out[n / 2].trade_id();
            }
        }};
    };

    std::vector<BenchCase> dec32_cases;
    dec32_cases.push_back(make_decode32_case("scalar", dfh::compression::decode_id_delta_scalar_u32<dfh::TradeTick>));
#if defined(__SSE2__)
    dec32_cases.push_back(make_decode32_case("sse2", dfh::compression::decode_id_delta_sse2_u32<dfh::TradeTick>));
#endif
#if defined(__AVX2__)
    dec32_cases.push_back(make_decode32_case("avx2", dfh::compression::decode_id_delta_avx2_u32<dfh::TradeTick>));
#endif
    dec32_cases.push_back(make_decode32_case("dispatcher", dfh::compression::decode_id_delta_u32<dfh::TradeTick>));
    std::vector<BenchResult> dec32_rows = run_cases_interleaved(dec32_cases, n, BENCH_INTERLEAVED_REPEATS);
    print_table("id delta decode u32 backends", dec32_rows);

    std::vector<BenchCase> dec64_cases;
    dec64_cases.push_back(make_decode64_case("scalar", dfh::compression::decode_id_delta_scalar_u64<dfh::TradeTick>));
#if defined(__SSE2__)
    dec64_cases.push_back(make_decode64_case("sse2", dfh::compression::decode_id_delta_sse2_u64<dfh::TradeTick>));
#endif
#if defined(__AVX2__)
    dec64_cases.push_back(make_decode64_case("avx2", dfh::compression::decode_id_delta_avx2_u64<dfh::TradeTick>));
#endif
    dec64_cases.push_back(make_decode64_case("dispatcher", dfh::compression::decode_id_delta_u64<dfh::TradeTick>));
    std::vector<BenchResult> dec64_rows = run_cases_interleaved(dec64_cases, n, BENCH_INTERLEAVED_REPEATS);
    print_table("id delta decode u64 backends", dec64_rows);
}

} // namespace

int main() {
    const std::vector<std::size_t> sizes = {10'000, 50'000, 100'000};
    std::cout << "bench_zig_zag_delta_id\n";
#if defined(__AVX2__)
    std::cout << "build ISA: AVX2 enabled\n";
#elif defined(__SSE2__)
    std::cout << "build ISA: SSE2 enabled\n";
#else
    std::cout << "build ISA: scalar only\n";
#endif

    for (const std::size_t n : sizes) {
        std::cout << "\n=== N = " << n << " ===\n";
        bench_id_delta(n);
    }

    std::cout << "\nsink=" << g_sink << '\n';
    return 0;
}
