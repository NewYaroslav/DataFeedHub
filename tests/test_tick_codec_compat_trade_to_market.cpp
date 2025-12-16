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

int run_tick_codec_compat_trade_to_market() {
#ifndef _MSC_VER
    std::vector<dfh::TradeTick> trades;
    constexpr std::uint64_t base_time = 1700000000000ULL;
    const std::array<dfh::TradeSide, 3> sides = {
        dfh::TradeSide::Buy,
        dfh::TradeSide::Sell,
        dfh::TradeSide::Unknown
    };

    for (std::uint64_t i = 0; i < 10; ++i) {
        dfh::TradeTick trade{};
        trade.price = 100.0 + static_cast<double>(i) * 0.125;
        trade.volume = 1.0 + static_cast<double>(i) * 0.5;
        trade.time_ms = base_time + i * 231ULL;
        trade.set_trade(static_cast<std::uint64_t>(5000 + i), sides[i % sides.size()]);
        trades.push_back(trade);
    }

    dfh::TickCodecConfig config{};
    config.price_digits = 6;
    config.volume_digits = 4;
    config.flags = dfh::TickStorageFlags::NONE;
    config.set_flag(dfh::TickStorageFlags::ENABLE_TICK_FLAGS, true);
    config.set_flag(dfh::TickStorageFlags::ENABLE_RECV_TIME, false);
    config.set_flag(dfh::TickStorageFlags::ENABLE_VOLUME, false);
    config.set_flag(dfh::TickStorageFlags::TRADE_BASED, true);

    dfh::compression::TickCompressorV1 compressor;
    compressor.set_codec_config(config);

    std::vector<std::uint8_t> buffer;
    compressor.serialize(trades, buffer);

    std::vector<dfh::MarketTick> decoded;
    dfh::TickCodecConfig decoded_config{};
    compressor.deserialize(buffer, decoded, decoded_config);

    assert(decoded.size() == trades.size());
    for (std::size_t index = 0; index < trades.size(); ++index) {
        const auto& expected = trades[index];
        const auto& actual = decoded[index];
        assert(expected.time_ms == actual.time_ms);
        assert(almost_equal(expected.price, actual.last));
        assert(almost_equal(expected.volume, actual.volume));
        assert(actual.ask == 0.0);
        assert(actual.bid == 0.0);
        assert(actual.received_ms == 0);
        assert(actual.flags == dfh::TickUpdateFlags::NONE);
    }

    assert(decoded_config.has_flag(dfh::TickStorageFlags::TRADE_BASED));
#else
    (void)almost_equal;
#endif

    return 0;
}

} // namespace

int main() {
    return run_tick_codec_compat_trade_to_market();
}
