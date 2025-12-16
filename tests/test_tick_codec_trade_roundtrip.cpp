#include <cassert>
#include <cmath>
#include <cstdint>
#include <vector>

#include <nlohmann/json.hpp>

#ifndef _MSC_VER
#define DFH_USE_JSON 1
#define DFH_USE_NLOHMANN_JSON 1
#include <DataFeedHub/compression.hpp>
#include <DataFeedHub/data/ticks/enums.hpp>
#endif

namespace {

bool almost_equal(double lhs, double rhs, double eps = 1e-12) {
    return std::fabs(lhs - rhs) <= eps;
}

int run_tick_codec_trade_roundtrip() {
#ifndef _MSC_VER
    std::vector<dfh::TradeTick> ticks;
    constexpr std::uint64_t base_time = 1700000000000ULL;
    const std::array<dfh::TradeSide, 3> sides = {
        dfh::TradeSide::Buy,
        dfh::TradeSide::Sell,
        dfh::TradeSide::Unknown
    };

    for (std::uint64_t i = 0; i < 10; ++i) {
        const dfh::TradeSide side = sides[i % sides.size()];
        dfh::TradeTick tick{};
        tick.price = 100.25 + static_cast<double>(i) * 0.15;
        tick.volume = 1.5 + static_cast<double>(i) * 0.1;
        tick.time_ms = base_time + i * 137ULL;
        tick.set_trade(static_cast<std::uint64_t>(1000 + i), side);
        ticks.push_back(tick);
    }

    dfh::TickCodecConfig config{};
    config.price_digits = 6;
    config.volume_digits = 3;
    config.flags = dfh::TickStorageFlags::NONE;
    config.set_flag(dfh::TickStorageFlags::ENABLE_TICK_FLAGS, true);
    config.set_flag(dfh::TickStorageFlags::ENABLE_RECV_TIME, false);
    config.set_flag(dfh::TickStorageFlags::ENABLE_VOLUME, false);
    config.set_flag(dfh::TickStorageFlags::TRADE_BASED, true);
    config.set_flag(dfh::TickStorageFlags::STORE_RAW_BINARY, false);

    dfh::compression::TickCompressorV1 compressor;
    compressor.set_codec_config(config);

    std::vector<std::uint8_t> buffer;
    compressor.serialize(ticks, buffer);

    std::vector<dfh::TradeTick> decoded;
    dfh::TickCodecConfig decoded_config{};
    compressor.deserialize(buffer, decoded, decoded_config);

    assert(decoded.size() == ticks.size());
    for (std::size_t index = 0; index < ticks.size(); ++index) {
        const auto& expected = ticks[index];
        const auto& actual = decoded[index];
        assert(expected.time_ms == actual.time_ms);
        assert(almost_equal(expected.price, actual.price));
        assert(almost_equal(expected.volume, actual.volume));
        assert(expected.trade_id() == actual.trade_id());
        assert(expected.trade_side() == actual.trade_side());
    }

    assert(decoded_config.has_flag(dfh::TickStorageFlags::TRADE_BASED));
    assert(decoded_config.has_flag(dfh::TickStorageFlags::ENABLE_TICK_FLAGS));
#else
    (void)almost_equal;
#endif

    return 0;
}

} // namespace

int main() {
    return run_tick_codec_trade_roundtrip();
}
