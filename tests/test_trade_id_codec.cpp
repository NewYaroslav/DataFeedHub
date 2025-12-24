#include <array>
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

#ifndef _MSC_VER
std::vector<dfh::TradeTick> build_trade_ticks(const std::vector<std::uint64_t>& trade_ids) {
    std::vector<dfh::TradeTick> ticks;
    ticks.reserve(trade_ids.size());
    constexpr std::uint64_t base_time = 1700000000000ULL;
    const std::array<dfh::TradeSide, 3> sides = {
        dfh::TradeSide::Buy,
        dfh::TradeSide::Sell,
        dfh::TradeSide::Unknown
    };

    for (std::size_t i = 0; i < trade_ids.size(); ++i) {
        dfh::TradeTick tick{};
        tick.price = 100.25 + static_cast<double>(i) * 0.01;
        tick.volume = 1.0 + static_cast<double>(i % 10) * 0.1;
        tick.time_ms = base_time + static_cast<std::uint64_t>(i) * 13ULL;
        tick.set_trade(trade_ids[i], sides[i % sides.size()]);
        ticks.push_back(tick);
    }
    return ticks;
}

void assert_trade_ticks_equal(const std::vector<dfh::TradeTick>& expected,
                              const std::vector<dfh::TradeTick>& actual) {
    assert(actual.size() == expected.size());
    for (std::size_t i = 0; i < expected.size(); ++i) {
        assert(expected[i].time_ms == actual[i].time_ms);
        assert(almost_equal(expected[i].price, actual[i].price));
        assert(almost_equal(expected[i].volume, actual[i].volume));
        assert(expected[i].trade_id() == actual[i].trade_id());
        assert(expected[i].trade_side() == actual[i].trade_side());
    }
}

std::vector<std::uint8_t> roundtrip_compressor(const std::vector<std::uint64_t>& trade_ids) {
    // Проверяем полный цикл через TickCompressorV1.
    const auto ticks = build_trade_ticks(trade_ids);

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
    compressor.deserialize(buffer, decoded);

    assert_trade_ticks_equal(ticks, decoded);
    return buffer;
}

void roundtrip_serializer(const std::vector<std::uint64_t>& trade_ids) {
    // Проверяем совместимость с TickBinarySerializerV1.
    const auto ticks = build_trade_ticks(trade_ids);

    dfh::TickCodecConfig config{};
    config.price_digits = 6;
    config.volume_digits = 3;
    config.flags = dfh::TickStorageFlags::NONE;
    config.set_flag(dfh::TickStorageFlags::ENABLE_TICK_FLAGS, false);
    config.set_flag(dfh::TickStorageFlags::ENABLE_RECV_TIME, false);
    config.set_flag(dfh::TickStorageFlags::ENABLE_VOLUME, false);
    config.set_flag(dfh::TickStorageFlags::TRADE_BASED, true);
    config.set_flag(dfh::TickStorageFlags::STORE_RAW_BINARY, true);

    dfh::compression::TickBinarySerializerV1 serializer;
    serializer.set_codec_config(config);

    std::vector<std::uint8_t> buffer;
    serializer.serialize(ticks, buffer);

    std::vector<dfh::TradeTick> decoded;
    serializer.deserialize(buffer, decoded);

    assert_trade_ticks_equal(ticks, decoded);
}
#endif

int run_trade_id_codec_tests() {
#ifndef _MSC_VER
    std::vector<std::uint64_t> sequential_ids;
    sequential_ids.reserve(2000);
    for (std::uint64_t i = 0; i < 2000; ++i) {
        sequential_ids.push_back(1'000'000ULL + i);
    }
    const auto seq_buffer = roundtrip_compressor(sequential_ids);
    roundtrip_serializer(sequential_ids);

    std::vector<std::uint64_t> gap_ids;
    gap_ids.reserve(2000);
    std::uint64_t current = 1'000'000ULL;
    for (std::uint64_t i = 0; i < 2000; ++i) {
        if (i > 0 && (i % 50 == 0)) {
            current += 1000;
        } else {
            current += 1;
        }
        gap_ids.push_back(current);
    }
    const auto gap_buffer = roundtrip_compressor(gap_ids);

    // sanity-check: последовательные id должны сжиматься не хуже, чем с разрывами.
    assert(seq_buffer.size() < gap_buffer.size());

    const std::vector<std::uint64_t> negative_delta_ids = {
        100, 101, 102, 90, 91, 92, 93, 94, 95, 96
    };
    roundtrip_compressor(negative_delta_ids);
#else
    (void)almost_equal(0.0, 0.0);
#endif
    return 0;
}

} // namespace

int main() {
    return run_trade_id_codec_tests();
}
