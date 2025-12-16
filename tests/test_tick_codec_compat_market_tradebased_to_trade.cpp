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

int run_tick_codec_compat_market_tradebased_to_trade() {
#ifndef _MSC_VER
    std::vector<dfh::MarketTick> ticks;
    constexpr std::uint64_t base_time = 1700000000000ULL;
    for (std::uint64_t i = 0; i < 10; ++i) {
        dfh::MarketTick tick{};
        tick.time_ms = base_time + i * 111ULL;
        tick.last = 99.5 + static_cast<double>(i) * 0.25;
        tick.volume = 2.0 + static_cast<double>(i) * 0.3;
        tick.ask = 0.0;
        tick.bid = 0.0;
        tick.received_ms = 0;
        tick.flags = dfh::TickUpdateFlags::NONE;
        ticks.push_back(tick);
    }

    dfh::TickCodecConfig config{};
    config.price_digits = 6;
    config.volume_digits = 3;
    config.flags = dfh::TickStorageFlags::NONE;
    config.set_flag(dfh::TickStorageFlags::ENABLE_TICK_FLAGS, true);
    config.set_flag(dfh::TickStorageFlags::ENABLE_RECV_TIME, false);
    config.set_flag(dfh::TickStorageFlags::ENABLE_VOLUME, true);
    config.set_flag(dfh::TickStorageFlags::TRADE_BASED, true);

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
        assert(almost_equal(expected.last, actual.price));
        assert(almost_equal(expected.volume, actual.volume));
        assert(actual.trade_id() == 0);
        assert(actual.trade_side() == dfh::TradeSide::Unknown);
    }

    assert(decoded_config.has_flag(dfh::TickStorageFlags::TRADE_BASED));
#else
    (void)almost_equal;
#endif

    return 0;
}

} // namespace

int main() {
    return run_tick_codec_compat_market_tradebased_to_trade();
}
