/// \file bench_zig_zag_delta.cpp
/// \brief Microbenchmark for scalar/SSE2/AVX2 Delta+ZigZag codecs.

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include <immintrin.h>

#if defined(__linux__)
#include <asm/unistd.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif
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



struct PerfCountersResult {
    std::string name;
    double cycles_per_elem{};
    double instructions_per_elem{};
    double cache_misses_per_k_elem{};
};

#if defined(__linux__)
static int perf_event_open(struct perf_event_attr* hw_event, pid_t pid, int cpu,
                           int group_fd, unsigned long flags) {
    return static_cast<int>(syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags));
}

PerfCountersResult measure_perf_counters(
    const std::string& name,
    std::size_t elems,
    const std::function<void()>& fn,
    std::size_t repeats = 5) {
    perf_event_attr pe_cycles{};
    pe_cycles.type = PERF_TYPE_HARDWARE;
    pe_cycles.size = sizeof(perf_event_attr);
    pe_cycles.config = PERF_COUNT_HW_CPU_CYCLES;
    pe_cycles.disabled = 1;
    pe_cycles.exclude_kernel = 1;
    pe_cycles.exclude_hv = 1;

    const int fd_cycles = perf_event_open(&pe_cycles, 0, -1, -1, 0);
    if (fd_cycles == -1) return {name, -1.0, -1.0, -1.0};

    perf_event_attr pe_instr{};
    pe_instr.type = PERF_TYPE_HARDWARE;
    pe_instr.size = sizeof(perf_event_attr);
    pe_instr.config = PERF_COUNT_HW_INSTRUCTIONS;
    pe_instr.disabled = 0;
    pe_instr.exclude_kernel = 1;
    pe_instr.exclude_hv = 1;
    const int fd_instr = perf_event_open(&pe_instr, 0, -1, fd_cycles, 0);

    perf_event_attr pe_cache{};
    pe_cache.type = PERF_TYPE_HARDWARE;
    pe_cache.size = sizeof(perf_event_attr);
    pe_cache.config = PERF_COUNT_HW_CACHE_MISSES;
    pe_cache.disabled = 0;
    pe_cache.exclude_kernel = 1;
    pe_cache.exclude_hv = 1;
    const int fd_cache = perf_event_open(&pe_cache, 0, -1, fd_cycles, 0);

    std::uint64_t cycles = 0, instr = 0, cache_miss = 0;
    for (std::size_t r = 0; r < repeats; ++r) {
        ioctl(fd_cycles, PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP);
        ioctl(fd_cycles, PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP);
        fn();
        ioctl(fd_cycles, PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);

        std::uint64_t c = 0, i = 0, m = 0;
        (void)read(fd_cycles, &c, sizeof(c));
        if (fd_instr != -1) (void)read(fd_instr, &i, sizeof(i));
        if (fd_cache != -1) (void)read(fd_cache, &m, sizeof(m));
        cycles += c;
        instr += i;
        cache_miss += m;
    }

    if (fd_cache != -1) close(fd_cache);
    if (fd_instr != -1) close(fd_instr);
    close(fd_cycles);

    const double denom = static_cast<double>(elems) * static_cast<double>(repeats);
    PerfCountersResult out;
    out.name = name;
    out.cycles_per_elem = static_cast<double>(cycles) / denom;
    out.instructions_per_elem = static_cast<double>(instr) / denom;
    out.cache_misses_per_k_elem = static_cast<double>(cache_miss) / (denom / 1000.0);
    return out;
}

void print_perf_table(const std::vector<PerfCountersResult>& rows) {
    std::cout << "\n[id decode perf counters]\n";
    std::cout << std::left << std::setw(28) << "impl"
              << std::right << std::setw(18) << "cycles/elem"
              << std::setw(20) << "instr/elem"
              << std::setw(24) << "cache-miss / 1k elem" << '\n';
    std::cout << std::string(90, '-') << '\n';
    for (const auto& r : rows) {
        std::cout << std::left << std::setw(28) << r.name
                  << std::right << std::setw(18) << std::fixed << std::setprecision(3) << r.cycles_per_elem
                  << std::setw(20) << std::fixed << std::setprecision(3) << r.instructions_per_elem
                  << std::setw(24) << std::fixed << std::setprecision(3) << r.cache_misses_per_k_elem
                  << '\n';
    }
    std::cout << "note: uops counters are not portable here; instructions/elem is used as a proxy.\n";
}

void run_perf_counters_mode(std::size_t n) {
    std::vector<dfh::TradeTick> ticks(n), out(n);
    std::uint64_t id = 1'000'000ULL;
    for (std::size_t i = 0; i < n; ++i) {
        ticks[i].set_trade(id, dfh::TradeSide::BUY);
        id += 1 + static_cast<std::uint64_t>(i % 3);
    }

    AlignedVec64<std::uint32_t> d32(n);
    AlignedVec64<std::uint64_t> d64(n);
    const std::int64_t initial_id = static_cast<std::int64_t>(ticks[0].trade_id() - 1);
    (void)dfh::compression::encode_id_delta_u32(ticks.data(), d32.data(), n, initial_id);
    dfh::compression::encode_id_delta_u64(ticks.data(), d64.data(), n, initial_id);

    std::vector<PerfCountersResult> rows;
    rows.push_back(measure_perf_counters("u32 scalar", n, [&] {
        dfh::compression::decode_id_delta_scalar_u32(d32.data(), out.data(), n, initial_id);
        g_sink ^= out[n / 2].trade_id();
    }));
#if defined(__SSE2__)
    rows.push_back(measure_perf_counters("u32 sse2", n, [&] {
        dfh::compression::decode_id_delta_sse2_u32(d32.data(), out.data(), n, initial_id);
        g_sink ^= out[n / 2].trade_id();
    }));
#endif
#if defined(__AVX2__)
    rows.push_back(measure_perf_counters("u32 avx2", n, [&] {
        dfh::compression::decode_id_delta_avx2_u32(d32.data(), out.data(), n, initial_id);
        g_sink ^= out[n / 2].trade_id();
    }));
#endif
    rows.push_back(measure_perf_counters("u32 dispatcher", n, [&] {
        dfh::compression::decode_id_delta_u32(d32.data(), out.data(), n, initial_id);
        g_sink ^= out[n / 2].trade_id();
    }));

    rows.push_back(measure_perf_counters("u64 scalar", n, [&] {
        dfh::compression::decode_id_delta_scalar_u64(d64.data(), out.data(), n, initial_id);
        g_sink ^= out[n / 2].trade_id();
    }));
#if defined(__SSE2__)
    rows.push_back(measure_perf_counters("u64 sse2", n, [&] {
        dfh::compression::decode_id_delta_sse2_u64(d64.data(), out.data(), n, initial_id);
        g_sink ^= out[n / 2].trade_id();
    }));
#endif
#if defined(__AVX2__)
    rows.push_back(measure_perf_counters("u64 avx2", n, [&] {
        dfh::compression::decode_id_delta_avx2_u64(d64.data(), out.data(), n, initial_id);
        g_sink ^= out[n / 2].trade_id();
    }));
#endif
    rows.push_back(measure_perf_counters("u64 dispatcher", n, [&] {
        dfh::compression::decode_id_delta_u64(d64.data(), out.data(), n, initial_id);
        g_sink ^= out[n / 2].trade_id();
    }));

    print_perf_table(rows);
}
#else
void run_perf_counters_mode(std::size_t) {
    std::cout << "perf counters mode is only available on Linux in this benchmark.\n";
}
#endif

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



struct BenchTick {
    std::int64_t time_ms{};
    double price{};
    std::uint64_t id{};

    static constexpr std::uint64_t max_trade_id() noexcept {
        return static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    }

    std::uint64_t trade_id() const noexcept { return id; }
    void set_trade_id(std::uint64_t v) noexcept { id = v; }
};

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

void bench_sorted_delta(std::size_t n) {
    auto src = make_monotonic_series<std::uint32_t>(n, 100u);
    AlignedVec64<std::uint32_t> enc(n), dec(n);

    BenchCase enc_case{"encode_delta_sorted", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i) {
            dfh::compression::encode_delta_sorted(src.data(), enc.data(), n, 100u);
            g_sink ^= enc[n / 2];
        }
    }};
    BenchCase dec_case{"decode_delta_sorted", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i) {
            dfh::compression::decode_delta_sorted(enc.data(), dec.data(), n, 100u);
            g_sink ^= dec[n / 2];
        }
    }};

    (void)run_case(enc_case, n);
    auto enc_row = run_case(enc_case, n);
    auto dec_row = run_case(dec_case, n);
    print_table("sorted delta", {enc_row, dec_row});
}

void bench_time_delta(std::size_t n) {
    auto ticks = make_tick_series(n);
    AlignedVec64<BenchTick> out(n);
    AlignedVec64<std::uint32_t> deltas(n);
    const std::int64_t initial_time = ticks[0].time_ms - 1;

    BenchCase enc_case{"encode_time_delta", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i) {
            dfh::compression::encode_time_delta(ticks.data(), deltas.data(), n, initial_time);
            g_sink ^= deltas[n / 2];
        }
    }};
    BenchCase dec_case{"decode_time_delta", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i) {
            dfh::compression::decode_time_delta(deltas.data(), out.data(), n, initial_time);
            g_sink ^= static_cast<std::uint64_t>(out[n / 2].time_ms);
        }
    }};

    (void)run_case(enc_case, n);
    auto enc_row = run_case(enc_case, n);
    auto dec_row = run_case(dec_case, n);
    print_table("time delta", {enc_row, dec_row});
}

void bench_id_delta(std::size_t n) {
    auto ticks = make_tick_series(n);
    AlignedVec64<BenchTick> out(n);
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
    const bool ok32 = dfh::compression::encode_id_delta_u32(ticks.data(), d32.data(), n, initial_id);
    if (!ok32) {
        std::cerr << "encode_id_delta_u32 returned false in benchmark setup\n";
    }
    dfh::compression::encode_id_delta_u64(ticks.data(), d64.data(), n, initial_id);

    print_table("id delta", {run_case(enc32_case, n), run_case(dec32_case, n), run_case(enc64_case, n), run_case(dec64_case, n)});

    auto make_decode32_case = [&](const std::string& name,
                                  void (*fn)(const std::uint32_t*, BenchTick*, std::size_t, std::int64_t)) {
        return BenchCase{name, [&, fn](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                fn(d32.data(), out.data(), n, initial_id);
                g_sink ^= out[n / 2].trade_id();
            }
        }};
    };

    auto make_decode64_case = [&](const std::string& name,
                                  void (*fn)(const std::uint64_t*, BenchTick*, std::size_t, std::int64_t)) {
        return BenchCase{name, [&, fn](std::size_t iterations) {
            for (std::size_t i = 0; i < iterations; ++i) {
                fn(d64.data(), out.data(), n, initial_id);
                g_sink ^= out[n / 2].trade_id();
            }
        }};
    };

    std::vector<BenchResult> dec32_rows;
    dec32_rows.push_back(run_case(make_decode32_case("scalar", dfh::compression::decode_id_delta_scalar_u32<BenchTick>), n));
#if defined(__SSE2__)
    dec32_rows.push_back(run_case(make_decode32_case("sse2", dfh::compression::decode_id_delta_sse2_u32<BenchTick>), n));
#endif
#if defined(__AVX2__)
    dec32_rows.push_back(run_case(make_decode32_case("avx2", dfh::compression::decode_id_delta_avx2_u32<BenchTick>), n));
#endif
    dec32_rows.push_back(run_case(make_decode32_case("dispatcher", dfh::compression::decode_id_delta_u32<BenchTick>), n));
    print_table("id delta decode u32 backends", dec32_rows);

    std::vector<BenchResult> dec64_rows;
    dec64_rows.push_back(run_case(make_decode64_case("scalar", dfh::compression::decode_id_delta_scalar_u64<BenchTick>), n));
#if defined(__SSE2__)
    dec64_rows.push_back(run_case(make_decode64_case("sse2", dfh::compression::decode_id_delta_sse2_u64<BenchTick>), n));
#endif
#if defined(__AVX2__)
    dec64_rows.push_back(run_case(make_decode64_case("avx2", dfh::compression::decode_id_delta_avx2_u64<BenchTick>), n));
#endif
    dec64_rows.push_back(run_case(make_decode64_case("dispatcher", dfh::compression::decode_id_delta_u64<BenchTick>), n));
    print_table("id delta decode u64 backends", dec64_rows);
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

    BenchCase enc32_case{"encode_price_delta_zig_zag_u32", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i) {
            const bool ok = dfh::compression::encode_price_delta_zig_zag_u32<BenchTick, &BenchTick::price>(
                ticks.data(), d32.data(), n, scale32, init32);
            g_sink ^= static_cast<std::uint64_t>(ok);
        }
    }};
    BenchCase dec32_case{"decode_price_delta_zig_zag_u32", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i) {
            dfh::compression::decode_price_delta_zig_zag_u32<BenchTick, &BenchTick::price>(
                d32.data(), out.data(), n, scale32, init32);
            g_sink ^= static_cast<std::uint64_t>(out[n / 2].price);
        }
    }};

    BenchCase enc64_case{"encode_price_delta_zig_zag_u64", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i) {
            dfh::compression::encode_price_delta_zig_zag_u64<BenchTick, &BenchTick::price>(
                ticks.data(), d64.data(), n, scale64, init64);
            g_sink ^= d64[n / 2];
        }
    }};
    BenchCase dec64_case{"decode_price_delta_zig_zag_u64", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i) {
            dfh::compression::decode_price_delta_zig_zag_u64<BenchTick, &BenchTick::price>(
                d64.data(), out.data(), n, scale64, init64);
            g_sink ^= static_cast<std::uint64_t>(out[n / 2].price);
        }
    }};

    // Warm up encoded buffers once before decode benches.
    (void)dfh::compression::encode_price_delta_zig_zag_u32<BenchTick, &BenchTick::price>(
        ticks.data(), d32.data(), n, scale32, init32);
    dfh::compression::encode_price_delta_zig_zag_u64<BenchTick, &BenchTick::price>(
        ticks.data(), d64.data(), n, scale64, init64);

    print_table("price delta",
        {run_case(enc32_case, n), run_case(dec32_case, n), run_case(enc64_case, n), run_case(dec64_case, n)});

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

    std::vector<BenchResult> enc32_rows;
    enc32_rows.push_back(run_case(make_enc32_case("scalar", dfh::compression::encode_price_delta_zig_zag_scalar_u32<BenchTick, &BenchTick::price>), n));
#if defined(__SSE2__)
    enc32_rows.push_back(run_case(make_enc32_case("sse2", dfh::compression::encode_price_delta_zig_zag_sse2_u32<BenchTick, &BenchTick::price>), n));
#endif
#if defined(__AVX2__)
    enc32_rows.push_back(run_case(make_enc32_case("avx2", dfh::compression::encode_price_delta_zig_zag_avx2_u32<BenchTick, &BenchTick::price>), n));
#endif
#if defined(__AVX512F__)
    enc32_rows.push_back(run_case(make_enc32_case("avx512", dfh::compression::encode_price_delta_zig_zag_avx512_u32<BenchTick, &BenchTick::price>), n));
#endif
    enc32_rows.push_back(run_case(make_enc32_case("dispatcher", dfh::compression::encode_price_delta_zig_zag_u32<BenchTick, &BenchTick::price>), n));
    print_table("price delta encode u32 backends", enc32_rows);

    std::vector<BenchResult> dec32_rows;
    dec32_rows.push_back(run_case(make_dec32_case("scalar", dfh::compression::decode_price_delta_zig_zag_scalar_u32<BenchTick, &BenchTick::price>), n));
#if defined(__SSE2__)
    dec32_rows.push_back(run_case(make_dec32_case("sse2", dfh::compression::decode_price_delta_zig_zag_sse2_u32<BenchTick, &BenchTick::price>), n));
#endif
#if defined(__AVX2__)
    dec32_rows.push_back(run_case(make_dec32_case("avx2", dfh::compression::decode_price_delta_zig_zag_avx2_u32<BenchTick, &BenchTick::price>), n));
#endif
#if defined(__AVX512F__)
    dec32_rows.push_back(run_case(make_dec32_case("avx512", dfh::compression::decode_price_delta_zig_zag_avx512_u32<BenchTick, &BenchTick::price>), n));
#endif
    dec32_rows.push_back(run_case(make_dec32_case("dispatcher", dfh::compression::decode_price_delta_zig_zag_u32<BenchTick, &BenchTick::price>), n));
    print_table("price delta decode u32 backends", dec32_rows);

    std::vector<BenchResult> enc64_rows;
    enc64_rows.push_back(run_case(make_enc64_case("scalar", dfh::compression::encode_price_delta_zig_zag_scalar_u64<BenchTick, &BenchTick::price>), n));
#if defined(__AVX2__)
    enc64_rows.push_back(run_case(make_enc64_case("avx2", dfh::compression::encode_price_delta_zig_zag_avx2_u64<BenchTick, &BenchTick::price>), n));
#endif
#if defined(__AVX512F__)
    enc64_rows.push_back(run_case(make_enc64_case("avx512", dfh::compression::encode_price_delta_zig_zag_avx512_u64<BenchTick, &BenchTick::price>), n));
#endif
    enc64_rows.push_back(run_case(make_enc64_case("dispatcher", dfh::compression::encode_price_delta_zig_zag_u64<BenchTick, &BenchTick::price>), n));
    print_table("price delta encode u64 backends", enc64_rows);

    std::vector<BenchResult> dec64_rows;
    dec64_rows.push_back(run_case(make_dec64_case("scalar", dfh::compression::decode_price_delta_zig_zag_scalar_u64<BenchTick, &BenchTick::price>), n));
#if defined(__SSE2__)
    dec64_rows.push_back(run_case(make_dec64_case("sse2", dfh::compression::decode_price_delta_zig_zag_sse2_u64<BenchTick, &BenchTick::price>), n));
#endif
#if defined(__AVX2__)
    dec64_rows.push_back(run_case(make_dec64_case("avx2", dfh::compression::decode_price_delta_zig_zag_avx2_u64<BenchTick, &BenchTick::price>), n));
#endif
#if defined(__AVX512F__)
    dec64_rows.push_back(run_case(make_dec64_case("avx512", dfh::compression::decode_price_delta_zig_zag_avx512_u64<BenchTick, &BenchTick::price>), n));
#endif
    dec64_rows.push_back(run_case(make_dec64_case("dispatcher", dfh::compression::decode_price_delta_zig_zag_u64<BenchTick, &BenchTick::price>), n));
    print_table("price delta decode u64 backends", dec64_rows);

#if defined(__AVX2__)
    // Stage-level probe for u64 encode path: helps quantify round/scale vs delta+zigzag costs.
    AlignedVec64<std::int64_t> scaled(n);
    auto scale_scalar = [&] {
        for (std::size_t i = 0; i < n; ++i) {
            scaled[i] = static_cast<std::int64_t>(std::llround(ticks[i].price * scale64));
        }
    };
    auto scale_avx2 = [&] {
        const __m256d scale_v = _mm256_set1_pd(scale64);
        const __m256d sign_mask_v = _mm256_set1_pd(-0.0);
        const __m256d half_v = _mm256_set1_pd(0.5);
        std::size_t i = 0;
        for (; i + 4 <= n; i += 4) {
            const __m256d x = _mm256_setr_pd(ticks[i + 0].price, ticks[i + 1].price, ticks[i + 2].price, ticks[i + 3].price);
            const __m256d v = _mm256_mul_pd(x, scale_v);
            const __m256d sign = _mm256_and_pd(v, sign_mask_v);
            const __m256d y = _mm256_add_pd(v, _mm256_or_pd(half_v, sign));
            alignas(32) double y_arr[4];
            _mm256_storeu_pd(y_arr, y);
            scaled[i + 0] = static_cast<std::int64_t>(y_arr[0]);
            scaled[i + 1] = static_cast<std::int64_t>(y_arr[1]);
            scaled[i + 2] = static_cast<std::int64_t>(y_arr[2]);
            scaled[i + 3] = static_cast<std::int64_t>(y_arr[3]);
        }
        for (; i < n; ++i) {
            scaled[i] = static_cast<std::int64_t>(std::llround(ticks[i].price * scale64));
        }
    };
    auto delta_scalar = [&] {
        std::int64_t prev = init64;
        for (std::size_t i = 0; i < n; ++i) {
            const std::int64_t d = scaled[i] - prev;
            d64[i] = dfh::compression::zigzag_encode_u64(d);
            prev = scaled[i];
        }
    };
    auto delta_avx2 = [&] {
        std::int64_t prev = init64;
        std::size_t i = 0;
        for (; i + 4 <= n; i += 4) {
            const __m256i cur = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(scaled.data() + i));
            alignas(32) std::int64_t c[4];
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(c), cur);
            const __m256i prevv = _mm256_setr_epi64x(prev, c[0], c[1], c[2]);
            const __m256i d = _mm256_sub_epi64(cur, prevv);
            const __m256i sign = _mm256_cmpgt_epi64(_mm256_setzero_si256(), d);
            const __m256i zz = _mm256_xor_si256(_mm256_slli_epi64(d, 1), sign);
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(d64.data() + i), zz);
            prev = c[3];
        }
        for (; i < n; ++i) {
            const std::int64_t d = scaled[i] - prev;
            d64[i] = dfh::compression::zigzag_encode_u64(d);
            prev = scaled[i];
        }
    };

    std::vector<BenchResult> stage_rows;
    stage_rows.push_back(run_case(BenchCase{"scale+round scalar", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i) {
            scale_scalar();
            g_sink ^= static_cast<std::uint64_t>(scaled[n / 2]);
        }
    }}, n));
    stage_rows.push_back(run_case(BenchCase{"scale+round avx2", [&](std::size_t iterations) {
        for (std::size_t i = 0; i < iterations; ++i) {
            scale_avx2();
            g_sink ^= static_cast<std::uint64_t>(scaled[n / 2]);
        }
    }}, n));
    stage_rows.push_back(run_case(BenchCase{"delta+zigzag scalar", [&](std::size_t iterations) {
        scale_scalar();
        for (std::size_t i = 0; i < iterations; ++i) {
            delta_scalar();
            g_sink ^= d64[n / 2];
        }
    }}, n));
    stage_rows.push_back(run_case(BenchCase{"delta+zigzag avx2", [&](std::size_t iterations) {
        scale_scalar();
        for (std::size_t i = 0; i < iterations; ++i) {
            delta_avx2();
            g_sink ^= d64[n / 2];
        }
    }}, n));
    print_table("price delta encode u64 stages", stage_rows);
#endif
}

} // namespace

int main(int argc, char** argv) {
    const std::vector<std::size_t> sizes = {10'000, 50'000, 100'000};

    bool use_perf_counters = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--perf-counters") == 0) {
            use_perf_counters = true;
        }
    }

    std::cout << "bench_zig_zag_delta\n";
#if defined(__AVX2__)
    std::cout << "build ISA: AVX2 enabled\n";
#elif defined(__SSE2__)
    std::cout << "build ISA: SSE2 enabled\n";
#else
    std::cout << "build ISA: scalar only\n";
#endif

    if (use_perf_counters) {
        run_perf_counters_mode(100'000);
    }

    for (const std::size_t n : sizes) {
        std::cout << "\n=== N = " << n << " ===\n";
        bench_u32(n);
        bench_i32(n);
        bench_u64(n);
        bench_i64(n);
        bench_sorted_delta(n);
        bench_time_delta(n);
        bench_id_delta(n);
        bench_price_delta(n);
    }

    std::cout << "\nsink=" << g_sink << '\n';
    return 0;
}
