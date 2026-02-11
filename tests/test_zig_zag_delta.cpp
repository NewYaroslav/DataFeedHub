/// \file test_zig_zag_delta.cpp
/// \brief Time-series based tests for DataFeedHub/compression/utils/zig_zag_delta.hpp.

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#include <immintrin.h>
#include "DataFeedHub/utils/aligned_allocator.hpp"
#include "DataFeedHub/data/ticks/TradeTick.hpp"
#include "DataFeedHub/compression/utils/zig_zag_delta.hpp"

namespace {

template <typename T>
using AlignedVec64 = std::vector<T, dfh::utils::aligned_allocator<T, 64>>;

struct SyntheticTick {
    std::int64_t time_ms{};
    double price{};
    std::uint64_t id{};

    static constexpr std::uint64_t max_trade_id() noexcept {
        return static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max());
    }

    std::uint64_t trade_id() const noexcept { return id; }
    void set_trade_id(std::uint64_t v) noexcept { id = v; }
};

template <typename TickType>
void set_tick_id(TickType& tick, std::uint64_t id) {
    tick.set_trade_id(id);
}

template <>
void set_tick_id(dfh::TradeTick& tick, std::uint64_t id) {
    tick.set_trade(id, dfh::TradeSide::BUY);
}

template <typename TickType>
std::vector<TickType> make_series_generic(std::size_t n) {
    std::vector<TickType> ticks(n);

    std::int64_t ts = 1'700'000'000'000LL;
    std::uint64_t id = 1'000'000ULL;
    constexpr double offset = 1000.0;
    constexpr double amplitude = 25.0;
    constexpr double trend = 0.15;

    for (std::size_t i = 0; i < n; ++i) {
        const double wave = amplitude * std::sin(static_cast<double>(i) * 0.15);
        ticks[i].time_ms = ts;
        ticks[i].price = offset + wave + trend * static_cast<double>(i);
        set_tick_id(ticks[i], id);

        ts += 5 + static_cast<std::int64_t>(i % 4);
        id += 1 + static_cast<std::uint64_t>(i % 3);
    }

    return ticks;
}

std::vector<SyntheticTick> make_series(std::size_t n) { return make_series_generic<SyntheticTick>(n); }
std::vector<dfh::TradeTick> make_trade_series(std::size_t n) { return make_series_generic<dfh::TradeTick>(n); }

void test_delta_sorted_roundtrip_u32_long() {
    constexpr std::size_t n = 4099;
    AlignedVec64<std::uint32_t> input(n), deltas(n), output(n);
    for (std::size_t i = 0; i < n; ++i) {
        input[i] = static_cast<std::uint32_t>(i * 3 + (i % 11));
    }

    dfh::compression::encode_delta_sorted(input.data(), deltas.data(), input.size(), 0u);
    dfh::compression::decode_delta_sorted(deltas.data(), output.data(), output.size(), 0u);
    assert(input == output);
}

template <typename TickType>
void test_time_id_and_price_delta_apis_impl(std::vector<TickType> ticks) {
    std::vector<TickType> decoded(ticks.size());

    AlignedVec64<std::uint32_t> time_delta(ticks.size());
    dfh::compression::encode_time_delta(ticks.data(), time_delta.data(), ticks.size(), ticks.front().time_ms - 1);
    dfh::compression::decode_time_delta(time_delta.data(), decoded.data(), decoded.size(), ticks.front().time_ms - 1);
    for (std::size_t i = 0; i < ticks.size(); ++i) {
        assert(ticks[i].time_ms == decoded[i].time_ms);
    }

    const auto initial_id = static_cast<std::int64_t>(ticks.front().trade_id() - 1);

    AlignedVec64<std::uint32_t> id_delta32(ticks.size());
    [[maybe_unused]] const bool ok32 =
            dfh::compression::encode_id_delta_u32(ticks.data(), id_delta32.data(), ticks.size(), initial_id);
    dfh::compression::decode_id_delta_u32(id_delta32.data(), decoded.data(), decoded.size(), initial_id);

    AlignedVec64<std::uint64_t> id_delta64(ticks.size());
    dfh::compression::encode_id_delta_u64(ticks.data(), id_delta64.data(), ticks.size(), initial_id);
    dfh::compression::decode_id_delta_u64(id_delta64.data(), decoded.data(), decoded.size(), initial_id);

    for (std::size_t i = 0; i < ticks.size(); ++i) {
        assert(ticks[i].trade_id() == decoded[i].trade_id());
    }

    AlignedVec64<std::uint32_t> price_delta32(ticks.size());
    [[maybe_unused]] const bool price32 =
            dfh::compression::encode_price_delta_zig_zag_u32<TickType, &TickType::price>(
                    ticks.data(), price_delta32.data(), ticks.size(), 100.0, 100000);
    dfh::compression::decode_price_delta_zig_zag_u32<TickType, &TickType::price>(
            price_delta32.data(), decoded.data(), decoded.size(), 100.0, 100000);

    AlignedVec64<std::uint64_t> price_delta64(ticks.size());
    dfh::compression::encode_price_delta_zig_zag_u64<TickType, &TickType::price>(
            ticks.data(), price_delta64.data(), ticks.size(), 100000.0, 10000000000LL);
    dfh::compression::decode_price_delta_zig_zag_u64<TickType, &TickType::price>(
            price_delta64.data(), decoded.data(), decoded.size(), 100000.0, 10000000000LL);
}

void test_time_id_and_price_delta_apis() {
    test_time_id_and_price_delta_apis_impl(make_series(4099));
    test_time_id_and_price_delta_apis_impl(make_trade_series(4099));
}



template <typename TickType>
void test_sorted_and_time_delta_edges_impl() {
    {
        std::vector<std::uint32_t> input{10u, 10u, 15u, 100u, 100u};
        std::vector<std::uint32_t> deltas(input.size());
        std::vector<std::uint32_t> decoded(input.size());
        dfh::compression::encode_delta_sorted(input.data(), deltas.data(), input.size(), 7u);
        dfh::compression::decode_delta_sorted(deltas.data(), decoded.data(), decoded.size(), 7u);
        assert(input == decoded);
    }

    {
        std::vector<TickType> ticks(3), decoded(3);
        ticks[0].time_ms = 1'000;
        ticks[1].time_ms = 1'000;
        ticks[2].time_ms = 1'250;
        std::vector<std::uint32_t> deltas(ticks.size());
        dfh::compression::encode_time_delta(ticks.data(), deltas.data(), ticks.size(), 900);
        dfh::compression::decode_time_delta(deltas.data(), decoded.data(), decoded.size(), 900);
        for (std::size_t i = 0; i < ticks.size(); ++i) {
            assert(ticks[i].time_ms == decoded[i].time_ms);
        }
    }
}

template <typename TickType>
void test_id_delta_roundtrip_all_backends_impl(std::vector<TickType> ticks) {
    std::vector<TickType> decoded(ticks.size());
    const auto initial_id = static_cast<std::int64_t>(ticks.front().trade_id() - 1);

    AlignedVec64<std::uint32_t> enc32(ticks.size());
    const bool ok_u32 = dfh::compression::encode_id_delta_u32(ticks.data(), enc32.data(), ticks.size(), initial_id);
    assert(ok_u32);
    dfh::compression::decode_id_delta_u32(enc32.data(), decoded.data(), decoded.size(), initial_id);
    for (std::size_t i = 0; i < ticks.size(); ++i) assert(ticks[i].trade_id() == decoded[i].trade_id());

    dfh::compression::decode_id_delta_scalar_u32(enc32.data(), decoded.data(), decoded.size(), initial_id);
    for (std::size_t i = 0; i < ticks.size(); ++i) assert(ticks[i].trade_id() == decoded[i].trade_id());

#if defined(__SSE2__)
    dfh::compression::decode_id_delta_sse2_u32(enc32.data(), decoded.data(), decoded.size(), initial_id);
    for (std::size_t i = 0; i < ticks.size(); ++i) assert(ticks[i].trade_id() == decoded[i].trade_id());
#endif
#if defined(__AVX2__)
    dfh::compression::decode_id_delta_avx2_u32(enc32.data(), decoded.data(), decoded.size(), initial_id);
    for (std::size_t i = 0; i < ticks.size(); ++i) assert(ticks[i].trade_id() == decoded[i].trade_id());
#endif

    AlignedVec64<std::uint64_t> enc64(ticks.size());
    dfh::compression::encode_id_delta_u64(ticks.data(), enc64.data(), ticks.size(), initial_id);
    dfh::compression::decode_id_delta_u64(enc64.data(), decoded.data(), decoded.size(), initial_id);
    for (std::size_t i = 0; i < ticks.size(); ++i) assert(ticks[i].trade_id() == decoded[i].trade_id());

    dfh::compression::decode_id_delta_scalar_u64(enc64.data(), decoded.data(), decoded.size(), initial_id);
    for (std::size_t i = 0; i < ticks.size(); ++i) assert(ticks[i].trade_id() == decoded[i].trade_id());

#if defined(__SSE2__)
    dfh::compression::decode_id_delta_sse2_u64(enc64.data(), decoded.data(), decoded.size(), initial_id);
    for (std::size_t i = 0; i < ticks.size(); ++i) assert(ticks[i].trade_id() == decoded[i].trade_id());
#endif
#if defined(__AVX2__)
    dfh::compression::decode_id_delta_avx2_u64(enc64.data(), decoded.data(), decoded.size(), initial_id);
    for (std::size_t i = 0; i < ticks.size(); ++i) assert(ticks[i].trade_id() == decoded[i].trade_id());
#endif
}

template <typename TickType>
void test_price_delta_roundtrip_all_backends_impl(std::vector<TickType> ticks) {
    std::vector<TickType> decoded(ticks.size());

    for (std::size_t i = 0; i < ticks.size(); ++i) {
        ticks[i].price = 1000.0 + static_cast<double>(i % 1000) * 0.01;
    }

    constexpr double scale32 = 100.0;
    constexpr std::int64_t init32 = 100'000;
    AlignedVec64<std::uint32_t> enc32(ticks.size());
    const bool ok_disp32 = dfh::compression::encode_price_delta_zig_zag_u32<TickType, &TickType::price>(
        ticks.data(), enc32.data(), ticks.size(), scale32, init32);
    if (ok_disp32) {
        dfh::compression::decode_price_delta_zig_zag_u32<TickType, &TickType::price>(
            enc32.data(), decoded.data(), decoded.size(), scale32, init32);
    }

    const bool ok_scalar32 = dfh::compression::encode_price_delta_zig_zag_scalar_u32<TickType, &TickType::price>(
        ticks.data(), enc32.data(), ticks.size(), scale32, init32);
    if (ok_scalar32) {
        dfh::compression::decode_price_delta_zig_zag_scalar_u32<TickType, &TickType::price>(
            enc32.data(), decoded.data(), decoded.size(), scale32, init32);
    }

#if defined(__SSE2__)
    const bool ok_sse232 = dfh::compression::encode_price_delta_zig_zag_sse2_u32<TickType, &TickType::price>(
        ticks.data(), enc32.data(), ticks.size(), scale32, init32);
    if (ok_sse232) {
        dfh::compression::decode_price_delta_zig_zag_sse2_u32<TickType, &TickType::price>(
            enc32.data(), decoded.data(), decoded.size(), scale32, init32);
    }
#endif
#if defined(__AVX2__)
    const bool ok_avx232 = dfh::compression::encode_price_delta_zig_zag_avx2_u32<TickType, &TickType::price>(
        ticks.data(), enc32.data(), ticks.size(), scale32, init32);
    if (ok_avx232) {
        dfh::compression::decode_price_delta_zig_zag_avx2_u32<TickType, &TickType::price>(
            enc32.data(), decoded.data(), decoded.size(), scale32, init32);
    }
#endif
#if defined(__AVX512F__)
    const bool ok_avx51232 = dfh::compression::encode_price_delta_zig_zag_avx512_u32<TickType, &TickType::price>(
        ticks.data(), enc32.data(), ticks.size(), scale32, init32);
    if (ok_avx51232) {
        dfh::compression::decode_price_delta_zig_zag_avx512_u32<TickType, &TickType::price>(
            enc32.data(), decoded.data(), decoded.size(), scale32, init32);
    }
#endif

    constexpr double scale64 = 100'000.0;
    constexpr std::int64_t init64 = 10'000'000'000LL;
    AlignedVec64<std::uint64_t> enc64(ticks.size());
    dfh::compression::encode_price_delta_zig_zag_u64<TickType, &TickType::price>(
        ticks.data(), enc64.data(), ticks.size(), scale64, init64);
    dfh::compression::decode_price_delta_zig_zag_u64<TickType, &TickType::price>(
        enc64.data(), decoded.data(), decoded.size(), scale64, init64);

    dfh::compression::encode_price_delta_zig_zag_scalar_u64<TickType, &TickType::price>(
        ticks.data(), enc64.data(), ticks.size(), scale64, init64);
    dfh::compression::decode_price_delta_zig_zag_scalar_u64<TickType, &TickType::price>(
        enc64.data(), decoded.data(), decoded.size(), scale64, init64);

#if defined(__AVX2__)
    dfh::compression::encode_price_delta_zig_zag_avx2_u64<TickType, &TickType::price>(
        ticks.data(), enc64.data(), ticks.size(), scale64, init64);
    dfh::compression::decode_price_delta_zig_zag_avx2_u64<TickType, &TickType::price>(
        enc64.data(), decoded.data(), decoded.size(), scale64, init64);
#endif
#if defined(__AVX512F__)
    dfh::compression::encode_price_delta_zig_zag_avx512_u64<TickType, &TickType::price>(
        ticks.data(), enc64.data(), ticks.size(), scale64, init64);
    dfh::compression::decode_price_delta_zig_zag_avx512_u64<TickType, &TickType::price>(
        enc64.data(), decoded.data(), decoded.size(), scale64, init64);
#endif

#if defined(__SSE2__)
    dfh::compression::decode_price_delta_zig_zag_sse2_u64<TickType, &TickType::price>(
        enc64.data(), decoded.data(), decoded.size(), scale64, init64);
#endif
#if defined(__AVX2__)
    dfh::compression::decode_price_delta_zig_zag_avx2_u64<TickType, &TickType::price>(
        enc64.data(), decoded.data(), decoded.size(), scale64, init64);
#endif
#if defined(__AVX512F__)
    dfh::compression::decode_price_delta_zig_zag_avx512_u64<TickType, &TickType::price>(
        enc64.data(), decoded.data(), decoded.size(), scale64, init64);
#endif
}

void test_missing_apis_roundtrip_and_backends() {
    auto ticks_synth = make_series(4099);
    auto ticks_trade = make_trade_series(4099);
    test_sorted_and_time_delta_edges_impl<SyntheticTick>();
    test_sorted_and_time_delta_edges_impl<dfh::TradeTick>();
    test_id_delta_roundtrip_all_backends_impl(ticks_synth);
    test_id_delta_roundtrip_all_backends_impl(ticks_trade);
    test_price_delta_roundtrip_all_backends_impl(ticks_synth);
    test_price_delta_roundtrip_all_backends_impl(ticks_trade);
}
void test_generic_delta_zigzag_dispatchers_long() {
    constexpr std::size_t n = 4097;

    AlignedVec64<std::uint32_t> src_u32(n), enc_u32(n), dec_u32(n);
    std::uint32_t u32_cur = 1000;
    for (std::size_t i = 0; i < n; ++i) {
        u32_cur += static_cast<std::uint32_t>(1 + (i % 3));
        src_u32[i] = u32_cur;
    }
    [[maybe_unused]] const bool ok_u32 =
            dfh::compression::encode_delta_zig_zag_u32(src_u32.data(), enc_u32.data(), n, 1000u);
    dfh::compression::decode_delta_zig_zag_u32(enc_u32.data(), dec_u32.data(), n, 1000u);
    assert(src_u32 == dec_u32);

    AlignedVec64<std::int32_t> src_i32(n), dec_i32(n);
    AlignedVec64<std::uint32_t> enc_i32(n);
    std::int32_t i32_cur = -5000;
    for (std::size_t i = 0; i < n; ++i) {
        i32_cur += static_cast<std::int32_t>((i % 7) - 3);
        src_i32[i] = i32_cur;
    }
    [[maybe_unused]] const bool ok_i32 =
            dfh::compression::encode_delta_zig_zag_i32(src_i32.data(), enc_i32.data(), n, -5000);
    dfh::compression::decode_delta_zig_zag_i32(enc_i32.data(), dec_i32.data(), n, -5000);
    assert(src_i32 == dec_i32);

    AlignedVec64<std::uint64_t> src_u64(n), enc_u64(n), dec_u64(n);
    std::uint64_t u64_cur = 500'000'000ULL;
    for (std::size_t i = 0; i < n; ++i) {
        u64_cur += static_cast<std::uint64_t>(1 + (i % 9));
        src_u64[i] = u64_cur;
    }
    dfh::compression::encode_delta_zig_zag_u64(src_u64.data(), enc_u64.data(), n, 500'000'000ULL);
    dfh::compression::decode_delta_zig_zag_u64(enc_u64.data(), dec_u64.data(), n, 500'000'000ULL);
    assert(src_u64 == dec_u64);

    AlignedVec64<std::int64_t> src_i64(n), dec_i64(n);
    AlignedVec64<std::uint64_t> enc_i64(n);
    std::int64_t i64_cur = -10'000'000LL;
    for (std::size_t i = 0; i < n; ++i) {
        i64_cur += static_cast<std::int64_t>((i % 11) - 5);
        src_i64[i] = i64_cur;
    }
    dfh::compression::encode_delta_zig_zag_i64(src_i64.data(), enc_i64.data(), n, -10'000'000LL);
    dfh::compression::decode_delta_zig_zag_i64(enc_i64.data(), dec_i64.data(), n, -10'000'000LL);
    assert(src_i64 == dec_i64);
}

void test_explicit_scalar_backends() {
    constexpr std::size_t n = 2051;

    AlignedVec64<std::uint32_t> src_u32(n), enc_u32(n), dec_u32(n);
    for (std::size_t i = 0; i < n; ++i) src_u32[i] = static_cast<std::uint32_t>(100 + i * 2);
    [[maybe_unused]] const bool ok_u32 =
            dfh::compression::encode_delta_zig_zag_scalar_u32(src_u32.data(), enc_u32.data(), n, 100u);
    dfh::compression::decode_delta_zig_zag_scalar_u32(enc_u32.data(), dec_u32.data(), n, 100u);
    assert(src_u32 == dec_u32);

    AlignedVec64<std::int32_t> src_i32(n), dec_i32(n);
    AlignedVec64<std::uint32_t> enc_i32(n);
    for (std::size_t i = 0; i < n; ++i) src_i32[i] = static_cast<std::int32_t>(-1000 + static_cast<int>(i % 17));
    [[maybe_unused]] const bool ok_i32 =
            dfh::compression::encode_delta_zig_zag_scalar_i32(src_i32.data(), enc_i32.data(), n, static_cast<std::uint32_t>(-1000));
    dfh::compression::decode_delta_zig_zag_scalar_i32(enc_i32.data(), dec_i32.data(), n, -1000);
    assert(src_i32 == dec_i32);

    AlignedVec64<std::uint64_t> src_u64(n), enc_u64(n), dec_u64(n);
    for (std::size_t i = 0; i < n; ++i) src_u64[i] = 9000000ULL + i * 4ULL;
    dfh::compression::encode_delta_zig_zag_scalar_u64(src_u64.data(), enc_u64.data(), n, 9000000ULL);
    dfh::compression::decode_delta_zig_zag_scalar_u64(enc_u64.data(), dec_u64.data(), n, 9000000LL);
    assert(src_u64 == dec_u64);

    AlignedVec64<std::int64_t> src_i64(n), dec_i64(n);
    AlignedVec64<std::uint64_t> enc_i64(n);
    for (std::size_t i = 0; i < n; ++i) src_i64[i] = -7000000LL + static_cast<std::int64_t>(i % 13);
    dfh::compression::encode_delta_zig_zag_scalar_i64(src_i64.data(), enc_i64.data(), n, -7000000LL);
    dfh::compression::decode_delta_zig_zag_scalar_i64(enc_i64.data(), dec_i64.data(), n, -7000000LL);
    assert(src_i64 == dec_i64);
}

#if defined(__SSE2__)
void test_explicit_sse2_backends() {
    constexpr std::size_t n = 2051;

    AlignedVec64<std::uint32_t> src_u32(n), enc_u32(n), dec_u32(n);
    for (std::size_t i = 0; i < n; ++i) src_u32[i] = static_cast<std::uint32_t>(100 + i * 3);
    [[maybe_unused]] const bool ok_u32 =
            dfh::compression::encode_delta_zig_zag_sse2_u32(src_u32.data(), enc_u32.data(), n, 100u);
    dfh::compression::decode_delta_zig_zag_sse2_u32(enc_u32.data(), dec_u32.data(), n, 100u);
    assert(src_u32 == dec_u32);

    AlignedVec64<std::int32_t> src_i32(n), dec_i32(n);
    AlignedVec64<std::uint32_t> enc_i32(n);
    for (std::size_t i = 0; i < n; ++i) src_i32[i] = -3000 + static_cast<std::int32_t>(i % 7);
    [[maybe_unused]] const bool ok_i32 =
            dfh::compression::encode_delta_zig_zag_sse2_i32(src_i32.data(), enc_i32.data(), n, static_cast<std::uint32_t>(-3000));
    dfh::compression::decode_delta_zig_zag_sse2_i32(enc_i32.data(), dec_i32.data(), n, -3000);
    assert(src_i32 == dec_i32);

    AlignedVec64<std::uint64_t> src_u64(n), enc_u64(n), dec_u64(n);
    for (std::size_t i = 0; i < n; ++i) src_u64[i] = 500000ULL + i * 5ULL;
    dfh::compression::encode_delta_zig_zag_sse2_u64(src_u64.data(), enc_u64.data(), n, 500000ULL);
    dfh::compression::decode_delta_zig_zag_sse2_u64(enc_u64.data(), dec_u64.data(), n, 500000LL);
    assert(src_u64 == dec_u64);

    AlignedVec64<std::int64_t> src_i64(n), dec_i64(n);
    AlignedVec64<std::uint64_t> enc_i64(n);
    for (std::size_t i = 0; i < n; ++i) src_i64[i] = -500000LL + static_cast<std::int64_t>(i % 9);
    dfh::compression::encode_delta_zig_zag_sse2_i64(src_i64.data(), enc_i64.data(), n, -500000LL);
    dfh::compression::decode_delta_zig_zag_sse2_i64(enc_i64.data(), dec_i64.data(), n, -500000LL);
    assert(src_i64 == dec_i64);
}
#endif

#if defined(__AVX2__)
void test_explicit_avx2_backends() {
    constexpr std::size_t n = 4099;

    AlignedVec64<std::uint32_t> src_u32(n), enc_u32(n), dec_u32(n);
    for (std::size_t i = 0; i < n; ++i) src_u32[i] = static_cast<std::uint32_t>(1000 + i * 4);
    [[maybe_unused]] const bool ok_u32 =
            dfh::compression::encode_delta_zig_zag_avx2_u32(src_u32.data(), enc_u32.data(), n, 1000u);
    dfh::compression::decode_delta_zig_zag_avx2_u32(enc_u32.data(), dec_u32.data(), n, 1000u);
    assert(src_u32 == dec_u32);

    AlignedVec64<std::int32_t> src_i32(n), dec_i32(n);
    AlignedVec64<std::uint32_t> enc_i32(n);
    for (std::size_t i = 0; i < n; ++i) src_i32[i] = -7000 + static_cast<std::int32_t>(i % 15);
    [[maybe_unused]] const bool ok_i32 =
            dfh::compression::encode_delta_zig_zag_avx2_i32(src_i32.data(), enc_i32.data(), n, static_cast<std::uint32_t>(-7000));
    dfh::compression::decode_delta_zig_zag_avx2_i32(enc_i32.data(), dec_i32.data(), n, -7000);
    assert(src_i32 == dec_i32);

    AlignedVec64<std::uint64_t> src_u64(n), enc_u64(n), dec_u64(n);
    for (std::size_t i = 0; i < n; ++i) src_u64[i] = 700000ULL + i * 8ULL;
    dfh::compression::encode_delta_zig_zag_avx2_u64(src_u64.data(), enc_u64.data(), n, 700000ULL);
    dfh::compression::decode_delta_zig_zag_avx2_u64(enc_u64.data(), dec_u64.data(), n, 700000LL);
    assert(src_u64 == dec_u64);

    AlignedVec64<std::int64_t> src_i64(n), dec_i64(n);
    AlignedVec64<std::uint64_t> enc_i64(n);
    for (std::size_t i = 0; i < n; ++i) src_i64[i] = -700000LL + static_cast<std::int64_t>(i % 19);
    dfh::compression::encode_delta_zig_zag_avx2_i64(src_i64.data(), enc_i64.data(), n, -700000LL);
    dfh::compression::decode_delta_zig_zag_avx2_i64(enc_i64.data(), dec_i64.data(), n, -700000LL);
    assert(src_i64 == dec_i64);
}
#endif

#if defined(__AVX512F__)
void test_explicit_avx512_backends() {
    constexpr std::size_t n = 4099;

    AlignedVec64<std::uint32_t> src_u32(n), enc_u32(n), dec_u32(n);
    for (std::size_t i = 0; i < n; ++i) src_u32[i] = static_cast<std::uint32_t>(500 + i * 6);
    [[maybe_unused]] const bool ok_u32 =
            dfh::compression::encode_delta_zig_zag_avx512_u32(src_u32.data(), enc_u32.data(), n, 500u);
    dfh::compression::decode_delta_zig_zag_avx512_u32(enc_u32.data(), dec_u32.data(), n, 500u);
    assert(src_u32 == dec_u32);

    AlignedVec64<std::int32_t> src_i32(n), dec_i32(n);
    AlignedVec64<std::uint32_t> enc_i32(n);
    for (std::size_t i = 0; i < n; ++i) src_i32[i] = -11000 + static_cast<std::int32_t>(i % 31);
    [[maybe_unused]] const bool ok_i32 =
            dfh::compression::encode_delta_zig_zag_avx512_i32(src_i32.data(), enc_i32.data(), n, static_cast<std::uint32_t>(-11000));
    dfh::compression::decode_delta_zig_zag_avx512_i32(enc_i32.data(), dec_i32.data(), n, -11000);
    assert(src_i32 == dec_i32);

    AlignedVec64<std::uint64_t> src_u64(n), enc_u64(n), dec_u64(n);
    for (std::size_t i = 0; i < n; ++i) src_u64[i] = 900000ULL + i * 10ULL;
    dfh::compression::encode_delta_zig_zag_avx512_u64(src_u64.data(), enc_u64.data(), n, 900000ULL);
    dfh::compression::decode_delta_zig_zag_avx512_u64(enc_u64.data(), dec_u64.data(), n, 900000LL);
    assert(src_u64 == dec_u64);

    AlignedVec64<std::int64_t> src_i64(n), dec_i64(n);
    AlignedVec64<std::uint64_t> enc_i64(n);
    for (std::size_t i = 0; i < n; ++i) src_i64[i] = -900000LL + static_cast<std::int64_t>(i % 23);
    dfh::compression::encode_delta_zig_zag_avx512_i64(src_i64.data(), enc_i64.data(), n, -900000LL);
    dfh::compression::decode_delta_zig_zag_avx512_i64(enc_i64.data(), dec_i64.data(), n, -900000LL);
    assert(src_i64 == dec_i64);
}
#endif

} // namespace

int main() {
    test_delta_sorted_roundtrip_u32_long();
    test_time_id_and_price_delta_apis();
    test_generic_delta_zigzag_dispatchers_long();
    test_missing_apis_roundtrip_and_backends();

    test_explicit_scalar_backends();
#if defined(__SSE2__)
    test_explicit_sse2_backends();
#endif
#if defined(__AVX2__)
    test_explicit_avx2_backends();
#endif
#if defined(__AVX512F__)
    test_explicit_avx512_backends();
#endif

    return 0;
}
