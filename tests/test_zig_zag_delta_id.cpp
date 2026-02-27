/// \file test_zig_zag_delta_id.cpp
/// \brief Trade ID delta tests for DataFeedHub/compression/utils/zig_zag_delta_id.hpp.

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

} // namespace

int main() {
    test_id_delta_roundtrip_all_backends_impl(make_series_generic<SyntheticTick>(4099));
    test_id_delta_roundtrip_all_backends_impl(make_series_generic<dfh::TradeTick>(4099));
    return 0;
}
