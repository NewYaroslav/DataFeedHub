#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

#include <nlohmann/json.hpp>

#ifndef _MSC_VER
#define DFH_USE_JSON 1
#define DFH_USE_NLOHMANN_JSON 1
#include <DataFeedHub/compression.hpp>
#endif

namespace {

bool almost_equal(double lhs, double rhs, double eps = 1e-12) {
    return std::fabs(lhs - rhs) <= eps;
}

int run_tick_codec_market_roundtrip() {
#ifndef _MSC_VER
    std::vector<dfh::MarketTick> ticks;
    constexpr std::uint64_t base_time = 1700000000000ULL;
    for (std::uint64_t i = 0; i < 10; ++i) {
        dfh::MarketTick tick{};
        tick.time_ms = base_time + i * 100ULL;
        tick.ask = 101.125 + static_cast<double>(i) * 0.125;
        tick.bid = 101.000 + static_cast<double>(i) * 0.125;
        tick.last = (tick.ask + tick.bid) * 0.5 + ((i % 3 == 0) ? 0.01 : -0.005);
        tick.volume = 0.0;
        tick.received_ms = 0;
        tick.flags = dfh::TickUpdateFlags::NONE;
        ticks.push_back(tick);
    }

    dfh::TickCodecConfig config{};
    config.price_digits = 6;
    config.volume_digits = 4;
    config.flags = dfh::TickStorageFlags::NONE;
    config.set_flag(dfh::TickStorageFlags::ENABLE_TICK_FLAGS, false);
    config.set_flag(dfh::TickStorageFlags::ENABLE_RECV_TIME, false);
    config.set_flag(dfh::TickStorageFlags::ENABLE_VOLUME, false);
    config.set_flag(dfh::TickStorageFlags::STORE_RAW_BINARY, true);
    config.set_flag(dfh::TickStorageFlags::TRADE_BASED, false);

    dfh::compression::TickBinarySerializerV1 serializer;
    serializer.set_codec_config(config);

    std::vector<std::uint8_t> buffer;
    serializer.serialize(ticks, buffer);

    std::vector<dfh::MarketTick> decoded;
    dfh::TickCodecConfig decoded_config{};
    serializer.deserialize(buffer, decoded, decoded_config);

    assert(decoded.size() == ticks.size());
    for (std::size_t index = 0; index < ticks.size(); ++index) {
        const auto& expected = ticks[index];
        const auto& actual = decoded[index];
        assert(expected.time_ms == actual.time_ms);
        assert(almost_equal(expected.ask, actual.ask));
        assert(almost_equal(expected.bid, actual.bid));
        assert(almost_equal(expected.last, actual.last));
        assert(actual.volume == 0.0);
        assert(actual.received_ms == 0);
        assert(actual.flags == dfh::TickUpdateFlags::NONE);
    }

    assert(!decoded_config.has_flag(dfh::TickStorageFlags::ENABLE_TICK_FLAGS));
    assert(!decoded_config.has_flag(dfh::TickStorageFlags::ENABLE_RECV_TIME));
    assert(!decoded_config.has_flag(dfh::TickStorageFlags::ENABLE_VOLUME));
    assert(decoded_config.has_flag(dfh::TickStorageFlags::STORE_RAW_BINARY));
    assert(!decoded_config.has_flag(dfh::TickStorageFlags::TRADE_BASED));
#else
    (void)almost_equal;
#endif

    return 0;
}

} // namespace

int main() {
    return run_tick_codec_market_roundtrip();
}
