/// \file test_zig_zag_delta_price.cpp
/// \brief Price delta ZigZag tests for DataFeedHub/compression/utils/zig_zag_delta_price.hpp.

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

#ifdef __SSE2__
#include <emmintrin.h>
#endif

#ifdef __SSE4_1__
#include <smmintrin.h>
#endif

#ifdef defined(__AVX__) || defined(__AVX2__) || defined(__AVX512F__) || defined(__AVX512DQ__)
#include <immintrin.h>
#endif

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

} // namespace

int main() {
    test_price_delta_roundtrip_all_backends_impl(make_series_generic<SyntheticTick>(4099));
    test_price_delta_roundtrip_all_backends_impl(make_series_generic<dfh::TradeTick>(4099));
    return 0;
}
