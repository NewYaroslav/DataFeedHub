#include <array>
#include <cstdint>
#include <string>
#include <system_error>

#include <zlib.h>
#include <mz.h>
#include <mz_strm.h>
#include <mz_zip.h>
#include <mz_zip_rw.h>
#include <simdcomp.h>
#include <vbyte.h>
#include <zstd.h>
#include <mdbx.h>
#include <time_shield_cpp/time_shield.hpp>
#include <gzip/compress.hpp>
#include <nlohmann/json.hpp>
#include <fast_double_parser.h>
#include <fast_float/fast_float.h>

int main() {
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

    std::array<uint8_t, 128> encoded{};
    vbyte_compress_unsorted32(values.data(), encoded.data(), values.size());

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
    auto now = time_shield::timestamp();
    auto iso = time_shield::to_iso8601(now);
    gzip::Compressor gzip_compressor;

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

    bool ok = !iso.empty() && json_value == 42.0 && has_version_info;
    (void)gzip_compressor;
    return ok ? 0 : 1;
}
