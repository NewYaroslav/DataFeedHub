#include <array>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

#include <zlib.h>
#include <mz.h>
#include <mz_strm.h>
#include <mz_zip.h>
#include <mz_zip_rw.h>
#include <simdcomp.h>
#ifndef _MSC_VER
#include <vbyte.h>
#endif
#include <zstd.h>
#include <mdbx.h>
#ifndef _MSC_VER
#include <time_shield_cpp/time_shield.hpp>
#include <gzip/compress.hpp>
#endif
#include <nlohmann/json.hpp>
#include <fast_double_parser.h>
#include <fast_float/fast_float.h>
#include <DataFeedHub/compression.hpp>

namespace {

bool almost_equal(double lhs, double rhs, double eps = 1e-12) {
    return std::fabs(lhs - rhs) <= eps;
}

void test_tick_codec_market_tick_roundtrip() {
    constexpr std::size_t tick_count = 32;
    constexpr std::uint64_t base_time_ms = 1700000000000ULL;

    std::vector<dfh::MarketTick> ticks;
    ticks.reserve(tick_count);
    for (std::size_t i = 0; i < tick_count; ++i) {
        dfh::MarketTick tick{};
        tick.time_ms = base_time_ms + static_cast<std::uint64_t>(i) * 100ULL;
        tick.ask = 100.25 + static_cast<double>(i) * 0.01;
        tick.bid = 100.20 + static_cast<double>(i) * 0.01;
        tick.last = 100.22 + static_cast<double>(i) * 0.01;
        tick.received_ms = 0;
        tick.volume = 0.0;
        tick.flags = dfh::TickUpdateFlags::NONE;
        ticks.push_back(tick);
    }

    dfh::TickCodecConfig codec_config{};
    codec_config.price_digits = 6;
    codec_config.volume_digits = 3;
    codec_config.tick_size = 0.0;
    codec_config.flags = dfh::TickStorageFlags::NONE;
    codec_config.set_flag(dfh::TickStorageFlags::ENABLE_TICK_FLAGS, false);
    codec_config.set_flag(dfh::TickStorageFlags::ENABLE_RECV_TIME, false);
    codec_config.set_flag(dfh::TickStorageFlags::ENABLE_VOLUME, false);
    codec_config.set_flag(dfh::TickStorageFlags::TRADE_BASED, false);
    // TickBinarySerializerV1 requires STORE_RAW_BINARY, other optional flags stay disabled.
    codec_config.set_flag(dfh::TickStorageFlags::STORE_RAW_BINARY, true);

    dfh::compression::TickBinarySerializerV1 serializer;
    serializer.set_codec_config(codec_config);

    std::vector<std::uint8_t> buffer;
    serializer.serialize(ticks, buffer);

    std::vector<dfh::MarketTick> decoded;
    dfh::TickCodecConfig decoded_config{};
    serializer.deserialize(buffer, decoded, decoded_config);

    assert(decoded.size() == ticks.size());
    for (std::size_t i = 0; i < ticks.size(); ++i) {
        const auto& expected = ticks[i];
        const auto& actual = decoded[i];
        assert(expected.time_ms == actual.time_ms);
        assert(actual.received_ms == 0);
        assert(almost_equal(expected.ask, actual.ask));
        assert(almost_equal(expected.bid, actual.bid));
        assert(almost_equal(expected.last, actual.last));
        assert(almost_equal(actual.volume, 0.0));
        assert(actual.flags == dfh::TickUpdateFlags::NONE);
    }

    assert(!decoded_config.has_flag(dfh::TickStorageFlags::ENABLE_TICK_FLAGS));
    assert(!decoded_config.has_flag(dfh::TickStorageFlags::ENABLE_RECV_TIME));
    assert(!decoded_config.has_flag(dfh::TickStorageFlags::ENABLE_VOLUME));
    assert(!decoded_config.has_flag(dfh::TickStorageFlags::TRADE_BASED));
    assert(decoded_config.has_flag(dfh::TickStorageFlags::STORE_RAW_BINARY));
}

} // namespace

int main() {
    test_tick_codec_market_tick_roundtrip();

    // zlib-ng
    z_stream stream{};
    if (deflateInit2(&stream, Z_BEST_SPEED, Z_DEFLATED, MAX_WBITS, 1, Z_DEFAULT_STRATEGY) != Z_OK) {
        return 1;
    }
    deflateEnd(&stream);

    // minizip-ng
    void* writer = mz_zip_writer_create();
    mz_zip_writer_set_compress_method(writer, MZ_COMPRESS_METHOD_DEFLATE);
    mz_zip_writer_delete(&writer);

    // simdcomp and vbyte
    std::array<uint32_t, 16> values{};
    for (uint32_t i = 0; i < values.size(); ++i) {
        values[i] = i * 7 + 3;
    }
    (void)simdmaxbitsd1_length(0, values.data(), static_cast<int>(values.size()));

    #ifndef _MSC_VER
    std::array<uint8_t, 128> encoded{};
    vbyte_compress_unsorted32(values.data(), encoded.data(), values.size());
    #endif

    // zstd
    std::array<uint8_t, 256> compressed{};
    size_t compressed_size = ZSTD_compress(
        compressed.data(),
        compressed.size(),
        values.data(),
        values.size() * sizeof(uint32_t),
        1);
    if (ZSTD_isError(compressed_size)) {
        return 1;
    }

    // mdbx
    const bool has_version_info = (mdbx_version.git.describe != nullptr);

    // Header-only deps
    #ifndef _MSC_VER
    auto now = time_shield::timestamp();
    auto iso = time_shield::to_iso8601(now);
    gzip::Compressor gzip_compressor;
    #endif

    nlohmann::json json = {{"value", 42}};
    double json_value = json["value"].get<double>();

    double parsed_double = 0.0;
    if (fast_double_parser::parse_number("3.14", &parsed_double) == nullptr) {
        return 1;
    }
    (void)parsed_double;

    const char ff_text[] = "2.71";
    double parsed_float = 0.0;
    auto fast_float_result = fast_float::from_chars(ff_text, ff_text + 4, parsed_float);
    if (fast_float_result.ec != std::errc()) {
        return 1;
    }
    (void)parsed_float;

    #ifndef _MSC_VER
    bool ok = !iso.empty() && json_value == 42.0 && has_version_info;
    (void)gzip_compressor;
    return ok ? 0 : 1;
    #endif
}
