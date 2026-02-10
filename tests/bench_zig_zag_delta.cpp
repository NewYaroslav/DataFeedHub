/// \file bench_zig_zag_delta.cpp
/// \brief Microbenchmark for scalar/SSE2/AVX2 Delta+ZigZag codecs.

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

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
    double elapsed_s{};
    double ns_per_elem{};
    double melem_per_s{};
};

struct BenchCase {
    std::string name;
    std::function<void(std::size_t iterations)> fn;
};

constexpr std::chrono::milliseconds BENCH_MIN_TIME{250};

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
    r.elapsed_s = elapsed_s;
    r.ns_per_elem = (elapsed_s * 1e9) / elems;
    r.melem_per_s = elems / elapsed_s / 1e6;
    return r;
}

template <typename T>
AlignedVec64<T> make_monotonic_series(std::size_t n, T initial) {
    AlignedVec64<T> v(n);
    T cur = initial;
    for (std::size_t i = 0; i < n; ++i) {
        cur += static_cast<T>(1 + (i % 4));
        v[i] = cur;
    }
    return v;
}

AlignedVec64<std::int32_t> make_signed_i32_series(std::size_t n, std::int32_t initial) {
    AlignedVec64<std::int32_t> v(n);
    std::int32_t cur = initial;
    for (std::size_t i = 0; i < n; ++i) {
        cur += static_cast<std::int32_t>(static_cast<int>(i % 7) - 3);
        v[i] = cur;
    }
    return v;
}

AlignedVec64<std::int64_t> make_signed_i64_series(std::size_t n, std::int64_t initial) {
    AlignedVec64<std::int64_t> v(n);
    std::int64_t cur = initial;
    for (std::size_t i = 0; i < n; ++i) {
        cur += static_cast<std::int64_t>(static_cast<int>(i % 11) - 5);
        v[i] = cur;
    }
    return v;
}

void print_table(const std::string& title, const std::vector<BenchResult>& rows) {
    std::cout << "\n[" << title << "]\n";
    std::cout << std::left << std::setw(28) << "impl"
              << std::right << std::setw(12) << "n"
              << std::setw(14) << "iters"
              << std::setw(16) << "ns/elem"
              << std::setw(16) << "Melem/s" << '\n';
    std::cout << std::string(86, '-') << '\n';
    for (const auto& r : rows) {
        std::cout << std::left << std::setw(28) << r.name
                  << std::right << std::setw(12) << r.n
                  << std::setw(14) << r.iterations
                  << std::setw(16) << std::fixed << std::setprecision(3) << r.ns_per_elem
                  << std::setw(16) << std::fixed << std::setprecision(2) << r.melem_per_s
                  << '\n';
    }
}

void bench_u32(std::size_t n) {
    constexpr std::uint32_t initial = 1000u;
    auto src = make_monotonic_series<std::uint32_t>(n, initial);
    AlignedVec64<std::uint32_t> enc(n), dec(n);

    auto make_encode_case = [&](const std::string& name,
                                bool (*fn)(const std::uint32_t*, std::uint32_t*, std::size_t, std::uint32_t)) {
        return BenchCase{name, [&, fn](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                const bool ok = fn(src.data(), enc.data(), n, initial);
                g_sink ^= static_cast<std::uint64_t>(ok);
            }
        }};
    };

    auto make_decode_case = [&](const std::string& name,
                                void (*fn)(const std::uint32_t*, std::uint32_t*, std::size_t, std::uint32_t)) {
        return BenchCase{name, [&, fn](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                fn(enc.data(), dec.data(), n, initial);
                g_sink ^= dec[n / 2];
            }
        }};
    };

    std::vector<BenchResult> enc_rows;
    enc_rows.push_back(run_case(make_encode_case("scalar", dfh::compression::encode_delta_zig_zag_scalar_u32), n));
#if defined(__SSE2__)
    enc_rows.push_back(run_case(make_encode_case("sse2", dfh::compression::encode_delta_zig_zag_sse2_u32), n));
#endif
#if defined(__AVX2__)
    enc_rows.push_back(run_case(make_encode_case("avx2", dfh::compression::encode_delta_zig_zag_avx2_u32), n));
#endif
#if defined(__AVX512F__)
    enc_rows.push_back(run_case(make_encode_case("avx512", dfh::compression::encode_delta_zig_zag_avx512_u32), n));
#endif
    enc_rows.push_back(run_case(make_encode_case("dispatcher", dfh::compression::encode_delta_zig_zag_u32), n));

    (void)dfh::compression::encode_delta_zig_zag_u32(src.data(), enc.data(), n, initial);

    std::vector<BenchResult> dec_rows;
    dec_rows.push_back(run_case(make_decode_case("scalar", dfh::compression::decode_delta_zig_zag_scalar_u32), n));
#if defined(__SSE2__)
    dec_rows.push_back(run_case(make_decode_case("sse2", dfh::compression::decode_delta_zig_zag_sse2_u32), n));
#endif
#if defined(__AVX2__)
    dec_rows.push_back(run_case(make_decode_case("avx2", dfh::compression::decode_delta_zig_zag_avx2_u32), n));
#endif
#if defined(__AVX512F__)
    dec_rows.push_back(run_case(make_decode_case("avx512", dfh::compression::decode_delta_zig_zag_avx512_u32), n));
#endif
    dec_rows.push_back(run_case(make_decode_case("dispatcher", dfh::compression::decode_delta_zig_zag_u32), n));

    print_table("u32 encode", enc_rows);
    print_table("u32 decode", dec_rows);
}

void bench_i32(std::size_t n) {
    constexpr std::int32_t initial = -5000;
    auto src = make_signed_i32_series(n, initial);
    AlignedVec64<std::uint32_t> enc(n);
    AlignedVec64<std::int32_t> dec(n);

    auto make_encode_case = [&](const std::string& name,
                                bool (*fn)(const std::int32_t*, std::uint32_t*, std::size_t, std::uint32_t)) {
        return BenchCase{name, [&, fn](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                const bool ok = fn(src.data(), enc.data(), n, static_cast<std::uint32_t>(initial));
                g_sink ^= static_cast<std::uint64_t>(ok);
            }
        }};
    };

    auto make_decode_case = [&](const std::string& name,
                                void (*fn)(const std::uint32_t*, std::int32_t*, std::size_t, std::int32_t)) {
        return BenchCase{name, [&, fn](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                fn(enc.data(), dec.data(), n, initial);
                g_sink ^= static_cast<std::uint64_t>(static_cast<std::uint32_t>(dec[n / 2]));
            }
        }};
    };

    std::vector<BenchResult> enc_rows;
    enc_rows.push_back(run_case(make_encode_case("scalar", dfh::compression::encode_delta_zig_zag_scalar_i32), n));
#if defined(__SSE2__)
    enc_rows.push_back(run_case(make_encode_case("sse2", dfh::compression::encode_delta_zig_zag_sse2_i32), n));
#endif
#if defined(__AVX2__)
    enc_rows.push_back(run_case(make_encode_case("avx2", dfh::compression::encode_delta_zig_zag_avx2_i32), n));
#endif
#if defined(__AVX512F__)
    enc_rows.push_back(run_case(make_encode_case("avx512", dfh::compression::encode_delta_zig_zag_avx512_i32), n));
#endif
    enc_rows.push_back(run_case(make_encode_case("dispatcher", dfh::compression::encode_delta_zig_zag_i32), n));

    (void)dfh::compression::encode_delta_zig_zag_i32(src.data(), enc.data(), n, static_cast<std::uint32_t>(initial));

    std::vector<BenchResult> dec_rows;
    dec_rows.push_back(run_case(make_decode_case("scalar", dfh::compression::decode_delta_zig_zag_scalar_i32), n));
#if defined(__SSE2__)
    dec_rows.push_back(run_case(make_decode_case("sse2", dfh::compression::decode_delta_zig_zag_sse2_i32), n));
#endif
#if defined(__AVX2__)
    dec_rows.push_back(run_case(make_decode_case("avx2", dfh::compression::decode_delta_zig_zag_avx2_i32), n));
#endif
#if defined(__AVX512F__)
    dec_rows.push_back(run_case(make_decode_case("avx512", dfh::compression::decode_delta_zig_zag_avx512_i32), n));
#endif
    dec_rows.push_back(run_case(make_decode_case("dispatcher", dfh::compression::decode_delta_zig_zag_i32), n));

    print_table("i32 encode", enc_rows);
    print_table("i32 decode", dec_rows);
}

void bench_u64(std::size_t n) {
    constexpr std::uint64_t initial = 700000ULL;
    auto src = make_monotonic_series<std::uint64_t>(n, initial);
    AlignedVec64<std::uint64_t> enc(n), dec(n);

    auto make_encode_case = [&](const std::string& name,
                                void (*fn)(const std::uint64_t*, std::uint64_t*, std::size_t, std::uint64_t)) {
        return BenchCase{name, [&, fn](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                fn(src.data(), enc.data(), n, initial);
                g_sink ^= enc[n / 2];
            }
        }};
    };

    auto make_decode_case_i64_init = [&](const std::string& name,
                                         void (*fn)(const std::uint64_t*, std::uint64_t*, std::size_t, std::int64_t)) {
        return BenchCase{name, [&, fn](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                fn(enc.data(), dec.data(), n, static_cast<std::int64_t>(initial));
                g_sink ^= dec[n / 2];
            }
        }};
    };

    auto make_decode_case_u64_init = [&](const std::string& name,
                                         void (*fn)(const std::uint64_t*, std::uint64_t*, std::size_t, std::uint64_t)) {
        return BenchCase{name, [&, fn](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                fn(enc.data(), dec.data(), n, initial);
                g_sink ^= dec[n / 2];
            }
        }};
    };

    std::vector<BenchResult> enc_rows;
    enc_rows.push_back(run_case(make_encode_case("scalar", dfh::compression::encode_delta_zig_zag_scalar_u64), n));
#if defined(__SSE2__)
    enc_rows.push_back(run_case(make_encode_case("sse2", dfh::compression::encode_delta_zig_zag_sse2_u64), n));
#endif
#if defined(__AVX2__)
    enc_rows.push_back(run_case(make_encode_case("avx2", dfh::compression::encode_delta_zig_zag_avx2_u64), n));
#endif
#if defined(__AVX512F__)
    enc_rows.push_back(run_case(make_encode_case("avx512", dfh::compression::encode_delta_zig_zag_avx512_u64), n));
#endif
    enc_rows.push_back(run_case(make_encode_case("dispatcher", dfh::compression::encode_delta_zig_zag_u64), n));

    dfh::compression::encode_delta_zig_zag_u64(src.data(), enc.data(), n, initial);

    std::vector<BenchResult> dec_rows;
    dec_rows.push_back(run_case(make_decode_case_i64_init("scalar", dfh::compression::decode_delta_zig_zag_scalar_u64), n));
#if defined(__SSE2__)
    dec_rows.push_back(run_case(make_decode_case_i64_init("sse2", dfh::compression::decode_delta_zig_zag_sse2_u64), n));
#endif
#if defined(__AVX2__)
    dec_rows.push_back(run_case(make_decode_case_i64_init("avx2", dfh::compression::decode_delta_zig_zag_avx2_u64), n));
#endif
#if defined(__AVX512F__)
    dec_rows.push_back(run_case(make_decode_case_i64_init("avx512", dfh::compression::decode_delta_zig_zag_avx512_u64), n));
#endif
    dec_rows.push_back(run_case(make_decode_case_u64_init("dispatcher", dfh::compression::decode_delta_zig_zag_u64), n));

    print_table("u64 encode", enc_rows);
    print_table("u64 decode", dec_rows);
}

void bench_i64(std::size_t n) {
    constexpr std::int64_t initial = -700000LL;
    auto src = make_signed_i64_series(n, initial);
    AlignedVec64<std::uint64_t> enc(n);
    AlignedVec64<std::int64_t> dec(n);

    auto make_encode_case = [&](const std::string& name,
                                void (*fn)(const std::int64_t*, std::uint64_t*, std::size_t, std::int64_t)) {
        return BenchCase{name, [&, fn](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                fn(src.data(), enc.data(), n, initial);
                g_sink ^= enc[n / 2];
            }
        }};
    };

    auto make_decode_case = [&](const std::string& name,
                                void (*fn)(const std::uint64_t*, std::int64_t*, std::size_t, std::int64_t)) {
        return BenchCase{name, [&, fn](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                fn(enc.data(), dec.data(), n, initial);
                g_sink ^= static_cast<std::uint64_t>(dec[n / 2]);
            }
        }};
    };

    std::vector<BenchResult> enc_rows;
    enc_rows.push_back(run_case(make_encode_case("scalar", dfh::compression::encode_delta_zig_zag_scalar_i64), n));
#if defined(__SSE2__)
    enc_rows.push_back(run_case(make_encode_case("sse2", dfh::compression::encode_delta_zig_zag_sse2_i64), n));
#endif
#if defined(__AVX2__)
    enc_rows.push_back(run_case(make_encode_case("avx2", dfh::compression::encode_delta_zig_zag_avx2_i64), n));
#endif
#if defined(__AVX512F__)
    enc_rows.push_back(run_case(make_encode_case("avx512", dfh::compression::encode_delta_zig_zag_avx512_i64), n));
#endif
    enc_rows.push_back(run_case(make_encode_case("dispatcher", dfh::compression::encode_delta_zig_zag_i64), n));

    dfh::compression::encode_delta_zig_zag_i64(src.data(), enc.data(), n, initial);

    std::vector<BenchResult> dec_rows;
    dec_rows.push_back(run_case(make_decode_case("scalar", dfh::compression::decode_delta_zig_zag_scalar_i64), n));
#if defined(__SSE2__)
    dec_rows.push_back(run_case(make_decode_case("sse2", dfh::compression::decode_delta_zig_zag_sse2_i64), n));
#endif
#if defined(__AVX2__)
    dec_rows.push_back(run_case(make_decode_case("avx2", dfh::compression::decode_delta_zig_zag_avx2_i64), n));
#endif
#if defined(__AVX512F__)
    dec_rows.push_back(run_case(make_decode_case("avx512", dfh::compression::decode_delta_zig_zag_avx512_i64), n));
#endif
    dec_rows.push_back(run_case(make_decode_case("dispatcher", dfh::compression::decode_delta_zig_zag_i64), n));

    print_table("i64 encode", enc_rows);
    print_table("i64 decode", dec_rows);
}

} // namespace

int main() {
    const std::vector<std::size_t> sizes = {10'000, 50'000, 100'000};

    std::cout << "bench_zig_zag_delta\n";
#if defined(__AVX2__)
    std::cout << "build ISA: AVX2 enabled\n";
#elif defined(__SSE2__)
    std::cout << "build ISA: SSE2 enabled\n";
#else
    std::cout << "build ISA: scalar only\n";
#endif

    for (const std::size_t n : sizes) {
        std::cout << "\n=== N = " << n << " ===\n";
        bench_u32(n);
        bench_i32(n);
        bench_u64(n);
        bench_i64(n);
    }

    std::cout << "\nsink=" << g_sink << '\n';
    return 0;
}
