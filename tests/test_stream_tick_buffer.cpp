#define DFH_TEST_MODE
#include <iostream>
#include <vector>
#include <filesystem>
#include <cassert>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <random>
#include <ctime>
#include <gzip/config.hpp>
#include <gzip/decompress.hpp>
#include <gzip/utils.hpp>
#include <gtest/gtest.h>
#include <time_shield_cpp/time_shield.hpp>
#include <fast_double_parser.h>
#include <DataFeedHub/src/structures/ticks/flags.hpp>
#include <DataFeedHub/src/structures/ticks/MarketTick.hpp>
#include <DataFeedHub/src/structures/ticks/ValueTick.hpp>
#include <DataFeedHub/src/structures/ticks/SimpleTick.hpp>
#include <DataFeedHub/src/structures/ticks/TickSpan.hpp>
#include <DataFeedHub/src/structures/ticks/TickCodecConfig.hpp>
#include <DataFeedHub/src/structures/ticks/BidAskRestoreConfig.hpp>
#include <DataFeedHub/src/structures/ticks/TickSequence.hpp>
#include <DataFeedHub/src/utils/math_utils.hpp>
#include <DataFeedHub/src/utils/fixed_point.hpp>
#include <DataFeedHub/src/utils/aligned_allocator.hpp>
#include <DataFeedHub/src/utils/sse_double_int64_utils.hpp>
#include <DataFeedHub/src/utils/vbyte.hpp>
#include <DataFeedHub/src/utils/simdcomp.hpp>
#include <DataFeedHub/src/utils/zip_utils.hpp>
#include <DataFeedHub/src/utils/bybit_parser.hpp>
#include <DataFeedHub/src/utils/binance_parser.hpp>
#include <DataFeedHub/src/utils/string_utils.hpp>
#include <DataFeedHub/src/compression/utils/zstd_utils.hpp>
#include <DataFeedHub/src/compression/utils/frequency_encoding.hpp>
#include <DataFeedHub/src/compression/utils/repeat_encoding.hpp>
#include <DataFeedHub/src/compression/utils/volume_scaling.hpp>
#include <DataFeedHub/src/compression/utils/zig_zag.hpp>
#include <DataFeedHub/src/compression/utils/zig_zag_delta.hpp>
#include <DataFeedHub/src/compression/ticks/TickCompressorV1/TickCompressionContextV1.hpp>
#include <DataFeedHub/src/compression/ticks/TickCompressorV1/TickEncoderV1.hpp>
#include <DataFeedHub/src/compression/ticks/TickCompressorV1/TickDecoderV1.hpp>
#include <DataFeedHub/src/compression/ticks/TickCompressorV1.hpp>
#include <DataFeedHub/src/structures/ticks/flags.hpp>
#include <DataFeedHub/src/core/IMarketDataSource.hpp>
#include <DataFeedHub/src/core/MarketDataBuffer/StreamTickBuffer.hpp>
#include <algorithm>

using namespace dfh::core;
using namespace dfh;

/// \brief Mock loader function for generating ticks.
/// \param ticks Vector to load ticks into.
/// \param config TickCodecConfig for mock behavior.
void mock_loader(std::vector<MarketTick>& ticks, TickCodecConfig& config) {
    for (uint64_t i = 0; i < 3600; ++i) {
        MarketTick tick, tick2;
        tick.time_ms = i * 1000;
        tick.last = static_cast<double>(i);
        tick.set_flag(i % 2 == 0 ? TickUpdateFlags::TICK_FROM_BUY : TickUpdateFlags::TICK_FROM_SELL);
        tick.set_flag(TickUpdateFlags::LAST_UPDATED);
        ticks.push_back(tick);

        tick2.time_ms = i * 1000 + 500;
        tick2.last = static_cast<double>(i);
        tick2.set_flag(i % 2 == 0 ? TickUpdateFlags::TICK_FROM_BUY : TickUpdateFlags::TICK_FROM_SELL);
        ticks.push_back(tick2);
    }
    config.price_digits = 0;
    config.volume_digits = 0;
}

void mock_loader_short(std::vector<MarketTick>& ticks, TickCodecConfig& config) {
    for (uint64_t i = 0; i < 3000; ++i) {
        MarketTick tick, tick2;
        tick.time_ms = i * 1000;  // One tick per second
        tick.last = static_cast<double>(i);
        tick.set_flag(i % 2 == 0 ? TickUpdateFlags::TICK_FROM_BUY : TickUpdateFlags::TICK_FROM_SELL);
        tick.set_flag(TickUpdateFlags::LAST_UPDATED);
        ticks.push_back(tick);

        tick2.time_ms = i * 1000 + 500;
        tick2.last = static_cast<double>(i);
        tick2.set_flag(i % 2 == 0 ? TickUpdateFlags::TICK_FROM_BUY : TickUpdateFlags::TICK_FROM_SELL);
        ticks.push_back(tick2);
    }
    config.price_digits = 0;
    config.volume_digits = 0;
}

void mock_loader_dynamic_spread(std::vector<MarketTick>& ticks, TickCodecConfig& config) {
    for (uint64_t i = 0; i < 3600; ++i) {
        MarketTick tick, tick2, tick3;
        tick.time_ms = i * 1000;  // One tick per second
        tick.last = 100.0;
        tick.set_flag(i % 2 == 0 ? TickUpdateFlags::TICK_FROM_BUY : TickUpdateFlags::TICK_FROM_SELL);
        tick.set_flag(TickUpdateFlags::LAST_UPDATED);
        ticks.push_back(tick);

        tick2.time_ms = i * 1000 + 250;
        tick2.last = 100.0;
        tick2.set_flag(i % 2 == 0 ? TickUpdateFlags::TICK_FROM_BUY : TickUpdateFlags::TICK_FROM_SELL);
        ticks.push_back(tick2);

        tick3.time_ms = i * 1000 + 500;
        tick3.last = 101.0;
        tick3.set_flag(i % 2 == 0 ? TickUpdateFlags::TICK_FROM_BUY : TickUpdateFlags::TICK_FROM_SELL);
        tick3.set_flag(TickUpdateFlags::LAST_UPDATED);
        ticks.push_back(tick3);
    }
    config.price_digits = 0;
    config.volume_digits = 0;
}

void mock_loader_real(uint64_t time_ms, std::vector<MarketTick>& ticks, TickCodecConfig& config) {
    uint64_t counter = 0;
    uint64_t start_time_ms = time_shield::start_of_hour_ms(time_ms);
    for (uint64_t t = start_time_ms; t < (start_time_ms + time_shield::MS_PER_HOUR); t += 250) {
        MarketTick tick;
        tick.time_ms = t;
        tick.last = static_cast<double>(counter) + 100.0;
        tick.set_flag(t % 500 == 0 ? TickUpdateFlags::TICK_FROM_BUY : TickUpdateFlags::TICK_FROM_SELL);
        tick.set_flag(TickUpdateFlags::LAST_UPDATED);
        ticks.push_back(tick);
        ++counter;
        if (counter >= 10) counter = 0;
    }
    config.price_digits = 0;
    config.volume_digits = 0;
}

TEST(StreamTickBufferTest, TestBasicLoadingAndAccess) {
    StreamTickBuffer buffer;
    StreamTickBuffer buffer_short;

    // Configure the buffer for fixed spread mode.
    BidAskRestoreConfig restore_config;
    restore_config.mode = BidAskModel::FIXED_SPREAD;
    restore_config.fixed_spread = 2;
    restore_config.price_digits = 0;
    buffer.set_bidask_config(restore_config);
    buffer_short.set_bidask_config(restore_config);

    // Load ticks into the buffer.
    buffer.test_load_ticks(0, mock_loader);
    buffer_short.test_load_ticks(0, mock_loader_short);

    // Check the tick count.
    EXPECT_EQ(buffer.tick_count(), 7200);
    EXPECT_EQ(buffer_short.tick_count(), 6000);

    // Verify the first tick.
    {
        buffer.set_tick_span(0, 1000);
        const MarketTick* first_tick = buffer.get_tick_span().data;
        ASSERT_NE(first_tick, nullptr);
        EXPECT_EQ(buffer.get_tick_span().size, 2);
        EXPECT_EQ(first_tick->time_ms, 0);
        EXPECT_EQ(first_tick->last, 0.0);
        EXPECT_EQ(first_tick->ask, first_tick->last);
        EXPECT_EQ(first_tick->bid, first_tick->last - 2.0);
    }

    {
        buffer.set_tick_span(500, 5000);
        const MarketTick* first_tick = buffer.get_tick_span().data;
        ASSERT_NE(first_tick, nullptr);
        EXPECT_EQ(buffer.get_tick_span().size, 9);
        EXPECT_EQ(first_tick->time_ms, 500);
        EXPECT_EQ(first_tick->last, 0.0);
        EXPECT_EQ(first_tick->ask, first_tick->last);
        EXPECT_EQ(first_tick->bid, first_tick->last - 2.0);
    }

    {
        buffer.set_tick_span(500, 5100);
        const MarketTick* first_tick = buffer.get_tick_span().data;
        ASSERT_NE(first_tick, nullptr);
        EXPECT_EQ(buffer.get_tick_span().size, 10);
        EXPECT_EQ(first_tick->time_ms, 500);
        EXPECT_EQ(first_tick->last, 0.0);
        EXPECT_EQ(first_tick->ask, first_tick->last);
        EXPECT_EQ(first_tick->bid, first_tick->last - 2.0);
    }

    {
        buffer_short.set_tick_span(0, 1000);
        const MarketTick* first_tick = buffer_short.get_tick_span().data;
        ASSERT_NE(first_tick, nullptr);
        EXPECT_EQ(buffer_short.get_tick_span().size, 2);
        EXPECT_EQ(first_tick->time_ms, 0);
        EXPECT_EQ(first_tick->last, 0.0);
        EXPECT_EQ(first_tick->ask, first_tick->last);
        EXPECT_EQ(first_tick->bid, first_tick->last - 2.0);
    }

    // Verify the last tick.
    {
        buffer.set_tick_span((3600 - 1) * 1000, 3600 * 1000);
        const MarketTick* last_tick = buffer.get_latest_tick();
        ASSERT_NE(last_tick, nullptr);
        EXPECT_EQ(buffer.get_tick_span().size, 2);
        EXPECT_EQ(last_tick->time_ms, 3599500);
        EXPECT_EQ(last_tick->last, 3599.0);
        EXPECT_EQ(last_tick->ask, last_tick->last + 2.0);
        EXPECT_EQ(last_tick->bid, last_tick->last);
    }

    {
        buffer_short.set_tick_span(2999500, 3000000);
        const MarketTick* last_tick = buffer_short.get_latest_tick();
        ASSERT_NE(last_tick, nullptr);
        EXPECT_EQ(buffer_short.get_tick_span().size, 1);
        EXPECT_EQ(last_tick->time_ms, 2999500);
        EXPECT_EQ(last_tick->last, 2999.0);
        EXPECT_EQ(last_tick->ask, last_tick->last + 2.0);
        EXPECT_EQ(last_tick->bid, last_tick->last);
    }

    //
    {
        buffer_short.set_tick_span(2999500, 2999500);
        const MarketTick* last_tick = buffer_short.get_latest_tick();
        EXPECT_EQ(last_tick, nullptr);
        EXPECT_EQ(buffer_short.get_tick_span().size, 0);
    }

    {
        buffer_short.set_tick_span(2999500, 2999501);
        const MarketTick* last_tick = buffer_short.get_latest_tick();
        ASSERT_NE(last_tick, nullptr);
        EXPECT_EQ(buffer_short.get_tick_span().size, 1);
        EXPECT_EQ(last_tick->time_ms, 2999500);
        EXPECT_EQ(last_tick->last, 2999.0);
        EXPECT_EQ(last_tick->ask, last_tick->last + 2.0);
        EXPECT_EQ(last_tick->bid, last_tick->last);
    }

    {
        buffer_short.set_tick_span(3000 * 1000, 3001 * 1000);
        const MarketTick* last_tick = buffer_short.get_latest_tick();
        EXPECT_EQ(last_tick, nullptr);
        EXPECT_EQ(buffer_short.get_tick_span().size, 0);
    }

    {
        buffer_short.set_tick_span(3000 * 1000, 3600 * 1000);
        const MarketTick* last_tick5 = buffer_short.get_latest_tick();
        EXPECT_EQ(last_tick5, nullptr);
        EXPECT_EQ(buffer_short.get_tick_span().size, 0);
    }
}

TEST(StreamTickBufferTest, TestBidAskModelNone) {
    StreamTickBuffer buffer;
    StreamTickBuffer buffer_short;

    // Configure the buffer for fixed spread mode.
    BidAskRestoreConfig restore_config;
    restore_config.mode = BidAskModel::NONE;
    restore_config.fixed_spread = 2;
    restore_config.price_digits = 0;
    buffer.set_bidask_config(restore_config);
    buffer_short.set_bidask_config(restore_config);

    // Load ticks into the buffer.
    buffer.test_load_ticks(0, mock_loader);
    buffer_short.test_load_ticks(0, mock_loader_short);

    // Check the tick count.
    EXPECT_EQ(buffer.tick_count(), 7200);
    EXPECT_EQ(buffer_short.tick_count(), 6000);

    // Verify the first tick.
    {
        buffer.set_tick_span(0, 1000);
        const MarketTick* first_tick = buffer.get_tick_span().data;
        ASSERT_NE(first_tick, nullptr);
        EXPECT_EQ(buffer.get_tick_span().size, 2);
        EXPECT_EQ(first_tick->time_ms, 0);
        EXPECT_EQ(first_tick->last, 0.0);
    }

    {
        buffer.set_tick_span(500, 5000);
        const MarketTick* first_tick = buffer.get_tick_span().data;
        ASSERT_NE(first_tick, nullptr);
        EXPECT_EQ(buffer.get_tick_span().size, 9);
        EXPECT_EQ(first_tick->time_ms, 500);
        EXPECT_EQ(first_tick->last, 0.0);
    }

    {
        buffer.set_tick_span(500, 5100);
        const MarketTick* first_tick = buffer.get_tick_span().data;
        ASSERT_NE(first_tick, nullptr);
        EXPECT_EQ(buffer.get_tick_span().size, 10);
        EXPECT_EQ(first_tick->time_ms, 500);
        EXPECT_EQ(first_tick->last, 0.0);
    }

    {
        buffer_short.set_tick_span(0, 1000);
        const MarketTick* first_tick = buffer_short.get_tick_span().data;
        ASSERT_NE(first_tick, nullptr);
        EXPECT_EQ(buffer_short.get_tick_span().size, 2);
        EXPECT_EQ(first_tick->time_ms, 0);
        EXPECT_EQ(first_tick->last, 0.0);
    }

    // Verify the last tick.
    {
        buffer.set_tick_span((3600 - 1) * 1000, 3600 * 1000);
        const MarketTick* last_tick = buffer.get_latest_tick();
        ASSERT_NE(last_tick, nullptr);
        EXPECT_EQ(buffer.get_tick_span().size, 2);
        EXPECT_EQ(last_tick->time_ms, 3599500);
        EXPECT_EQ(last_tick->last, 3599.0);
    }

    {
        buffer_short.set_tick_span(2999500, 3000000);
        const MarketTick* last_tick = buffer_short.get_latest_tick();
        ASSERT_NE(last_tick, nullptr);
        EXPECT_EQ(buffer_short.get_tick_span().size, 1);
        EXPECT_EQ(last_tick->time_ms, 2999500);
        EXPECT_EQ(last_tick->last, 2999.0);
    }

    //
    {
        buffer_short.set_tick_span(2999500, 2999500);
        const MarketTick* last_tick = buffer_short.get_latest_tick();
        EXPECT_EQ(last_tick, nullptr);
        EXPECT_EQ(buffer_short.get_tick_span().size, 0);
    }

    {
        buffer_short.set_tick_span(2999500, 2999501);
        const MarketTick* last_tick = buffer_short.get_latest_tick();
        ASSERT_NE(last_tick, nullptr);
        EXPECT_EQ(buffer_short.get_tick_span().size, 1);
        EXPECT_EQ(last_tick->time_ms, 2999500);
        EXPECT_EQ(last_tick->last, 2999.0);
    }

    {
        buffer_short.set_tick_span(3000 * 1000, 3001 * 1000);
        const MarketTick* last_tick = buffer_short.get_latest_tick();
        EXPECT_EQ(last_tick, nullptr);
        EXPECT_EQ(buffer_short.get_tick_span().size, 0);
    }

    {
        buffer_short.set_tick_span(3000 * 1000, 3600 * 1000);
        const MarketTick* last_tick5 = buffer_short.get_latest_tick();
        EXPECT_EQ(last_tick5, nullptr);
        EXPECT_EQ(buffer_short.get_tick_span().size, 0);
    }
}

TEST(StreamTickBufferTest, TestTickSpan) {
    StreamTickBuffer buffer;

    // Configure the buffer for fixed spread mode.
    BidAskRestoreConfig restore_config;
    restore_config.mode = BidAskModel::FIXED_SPREAD;
    restore_config.fixed_spread = 2;
    restore_config.price_digits = 0;
    buffer.set_bidask_config(restore_config);

    // Load ticks into the buffer.
    buffer.test_load_ticks(0, mock_loader);

    // Set a tick span.
    buffer.set_tick_span(1000, 2000); // From 1 second to 2 seconds.
    const MarketTickSpan& span = buffer.get_tick_span();

    // Verify the span.
    EXPECT_EQ(span.size, 2);
    ASSERT_NE(span.data, nullptr);
    EXPECT_EQ(span.data[0].time_ms, 1000);
    EXPECT_EQ(span.data[0].last, 1.0);
    EXPECT_EQ(span.data[0].ask, 3.0);
    EXPECT_EQ(span.data[0].bid, 1.0);
}

TEST(StreamTickBufferTest, TestDynamicSpread) {
    StreamTickBuffer buffer;

    // Configure the buffer for dynamic spread mode.
    BidAskRestoreConfig restore_config;
    restore_config.mode = BidAskModel::DYNAMIC_SPREAD;
    restore_config.fixed_spread = 2;
    restore_config.price_digits = 0;
    buffer.set_bidask_config(restore_config);

    // Load ticks into the buffer.
    buffer.test_load_ticks(0, mock_loader_dynamic_spread);

    // Check the tick count.
    EXPECT_EQ(buffer.tick_count(), 3600 * 3);

    // Verify the first tick.
    buffer.set_tick_span(0, 2100);
    const MarketTickSpan& span = buffer.get_tick_span();
    ASSERT_NE(span.data, nullptr);
    EXPECT_EQ(span.size, 7);
    EXPECT_EQ(span.data[0].time_ms, 0);
    EXPECT_EQ(span.data[0].last, 100.0);
    EXPECT_EQ(span.data[0].ask,  100.0);
    EXPECT_EQ(span.data[0].bid,  98.0);

    EXPECT_EQ(span.data[1].time_ms, 250);
    EXPECT_EQ(span.data[1].last, 100.0);
    EXPECT_EQ(span.data[1].ask,  100.0);
    EXPECT_EQ(span.data[1].bid,  98.0);

    EXPECT_EQ(span.data[2].time_ms, 500);
    EXPECT_EQ(span.data[2].last, 101.0);
    EXPECT_EQ(span.data[2].ask,  101.0);
    EXPECT_EQ(span.data[2].bid,  99.0);

    EXPECT_EQ(span.data[3].time_ms, 1000);
    EXPECT_EQ(span.data[3].last, 100.0);
    EXPECT_EQ(span.data[3].ask,  101.0);
    EXPECT_EQ(span.data[3].bid,  100.0);

    EXPECT_EQ(span.data[4].time_ms, 1250);
    EXPECT_EQ(span.data[4].last, 100.0);
    EXPECT_EQ(span.data[4].ask,  101.0);
    EXPECT_EQ(span.data[4].bid,  100.0);

    EXPECT_EQ(span.data[5].time_ms, 1500);
    EXPECT_EQ(span.data[5].last, 101.0);
    EXPECT_EQ(span.data[5].ask,  102.0);
    EXPECT_EQ(span.data[5].bid,  101.0);

    EXPECT_EQ(span.data[6].time_ms, 2000);
    EXPECT_EQ(span.data[6].last, 100.0);
    EXPECT_EQ(span.data[6].ask,  100.0);
    EXPECT_EQ(span.data[6].bid,  99.0);

    // Verify bid/ask for dynamic spread.
    const MarketTick* last_tick = buffer.get_latest_tick();
    ASSERT_NE(last_tick, nullptr);
    EXPECT_GT(last_tick->ask, last_tick->bid);
}

TEST(StreamTickBufferTest, TestRealLoader) {
    StreamTickBuffer buffer;

    // Configure the buffer for median spread mode.
    // This test uses the MEDIAN_SPREAD bid/ask model with a fixed spread value of 1.
    BidAskRestoreConfig restore_config;
    restore_config.mode = BidAskModel::MEDIAN_SPREAD;
    restore_config.fixed_spread = 1;
    restore_config.price_digits = 0;
    buffer.set_bidask_config(restore_config);

    // Load ticks into the buffer using a mock loader that simulates real-time data.
    buffer.test_load_ticks_2(86400000, mock_loader_real);

    // Verify that the buffer contains the expected number of ticks.
    // The tick count should match the expected size for the generated data.
    EXPECT_EQ(buffer.tick_count(), 3600 * 4);

    // Set a span of ticks between 86400000ms (00:00:00) and 86400000 + 2100ms.
    buffer.set_tick_span(86400000, 86400000 + 2100);
    const MarketTickSpan& span = buffer.get_tick_span();

    // Verify that the span is not empty and contains the expected number of ticks.
    ASSERT_NE(span.data, nullptr);
    EXPECT_EQ(span.size, 9);

    // Verify individual ticks within the span for correctness.
    EXPECT_EQ(span.data[0].time_ms, 86400000);
    EXPECT_EQ(span.data[0].ask - span.data[0].bid,  1.0);

    EXPECT_EQ(span.data[1].time_ms, 86400250);
    EXPECT_EQ(span.data[1].ask - span.data[1].bid,  1.0);

    EXPECT_EQ(span.data[2].time_ms, 86400500);
    EXPECT_EQ(span.data[2].ask - span.data[2].bid,  1.0);

    EXPECT_EQ(span.data[3].time_ms, 86400750);
    EXPECT_EQ(span.data[3].ask - span.data[3].bid,  1.0);

    EXPECT_EQ(span.data[8].time_ms, 86402000);
    EXPECT_EQ(span.data[8].ask - span.data[8].bid,  1.0);

    // Verify that the bid/ask values for the last tick are consistent.
    const MarketTick* last_tick = buffer.get_latest_tick();
    ASSERT_NE(last_tick, nullptr);

    // The ask price should always be greater than the bid price.
    EXPECT_GT(last_tick->ask, last_tick->bid);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
