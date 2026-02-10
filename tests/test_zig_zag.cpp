/// \file test_zig_zag.cpp
/// \brief Coverage for functions from DataFeedHub/compression/utils/zig_zag.hpp.

#include <cassert>
#include <cstdint>
#include <limits>
#include <vector>
#include <immintrin.h>

#include "DataFeedHub/utils/aligned_allocator.hpp"
#include "DataFeedHub/compression/utils/zig_zag.hpp"

namespace {

template <typename T>
using AlignedVec64 = std::vector<T, dfh::utils::aligned_allocator<T, 64>>;

void test_scalar_primitives_u32_u64() {
    using dfh::compression::zigzag_decode_u32;
    using dfh::compression::zigzag_decode_u64;
    using dfh::compression::zigzag_encode_u32;
    using dfh::compression::zigzag_encode_u64;

    assert(zigzag_encode_u32(0) == 0u);
    assert(zigzag_encode_u32(-1) == 1u);
    assert(zigzag_encode_u32(1) == 2u);
    assert(zigzag_encode_u32(std::numeric_limits<std::int32_t>::max()) == 0xFFFFFFFEu);
    assert(zigzag_encode_u32(std::numeric_limits<std::int32_t>::min()) == 0xFFFFFFFFu);

    assert(zigzag_decode_u32(0u) == 0);
    assert(zigzag_decode_u32(1u) == -1);
    assert(zigzag_decode_u32(2u) == 1);
    assert(zigzag_decode_u32(0xFFFFFFFEu) == std::numeric_limits<std::int32_t>::max());
    assert(zigzag_decode_u32(0xFFFFFFFFu) == std::numeric_limits<std::int32_t>::min());

    assert(zigzag_encode_u64(0) == 0ull);
    assert(zigzag_encode_u64(-1) == 1ull);
    assert(zigzag_encode_u64(1) == 2ull);
    assert(zigzag_encode_u64(std::numeric_limits<std::int64_t>::max()) == 0xFFFFFFFFFFFFFFFEull);
    assert(zigzag_encode_u64(std::numeric_limits<std::int64_t>::min()) == 0xFFFFFFFFFFFFFFFFull);

    assert(zigzag_decode_u64(0ull) == 0);
    assert(zigzag_decode_u64(1ull) == -1);
    assert(zigzag_decode_u64(2ull) == 1);
    assert(zigzag_decode_u64(0xFFFFFFFFFFFFFFFEull) == std::numeric_limits<std::int64_t>::max());
    assert(zigzag_decode_u64(0xFFFFFFFFFFFFFFFFull) == std::numeric_limits<std::int64_t>::min());
}

void test_scalar_array_codecs_long_sequences() {
    constexpr std::size_t n = 4097;
    AlignedVec64<std::int32_t> src32(n);
    AlignedVec64<std::uint32_t> enc32(n);
    AlignedVec64<std::int32_t> dec32(n);

    for (std::size_t i = 0; i < n; ++i) {
        src32[i] = (i % 2 == 0) ? static_cast<std::int32_t>(i * 17) : -static_cast<std::int32_t>(i * 19);
    }

    dfh::compression::encode_zig_zag_scalar_u32(src32.data(), enc32.data(), src32.size());
    dfh::compression::decode_zig_zag_scalar_u32(enc32.data(), dec32.data(), dec32.size());
    assert(src32 == dec32);

    AlignedVec64<std::int64_t> src64(n);
    AlignedVec64<std::uint64_t> enc64(n);
    AlignedVec64<std::int64_t> dec64(n);

    for (std::size_t i = 0; i < n; ++i) {
        src64[i] = (i % 2 == 0) ? static_cast<std::int64_t>(i) * 9876543LL : -static_cast<std::int64_t>(i) * 1234567LL;
    }

    dfh::compression::encode_zig_zag_scalar_u64(src64.data(), enc64.data(), src64.size());
    dfh::compression::decode_zig_zag_scalar_u64(enc64.data(), dec64.data(), dec64.size());
    assert(src64 == dec64);
}

void test_dispatch_array_codecs_long_sequences() {
    constexpr std::size_t n = 4097;
    AlignedVec64<std::int32_t> src32(n);
    AlignedVec64<std::uint32_t> enc32(n);
    AlignedVec64<std::int32_t> dec32(n);

    for (std::size_t i = 0; i < n; ++i) {
        src32[i] = (i % 3 == 0) ? static_cast<std::int32_t>(i * 3) : -static_cast<std::int32_t>(i * 5);
    }

    dfh::compression::encode_zig_zag_u32(src32.data(), enc32.data(), n);
    dfh::compression::decode_zig_zag_u32(enc32.data(), dec32.data(), n);
    assert(src32 == dec32);

    AlignedVec64<std::int64_t> src64(n);
    AlignedVec64<std::uint64_t> enc64(n);
    AlignedVec64<std::int64_t> dec64(n);

    for (std::size_t i = 0; i < n; ++i) {
        src64[i] = (i % 2 == 0) ? static_cast<std::int64_t>(i) * 111111111LL
                                : -static_cast<std::int64_t>(i) * 222222223LL;
    }

    dfh::compression::encode_zig_zag_u64(src64.data(), enc64.data(), n);
    dfh::compression::decode_zig_zag_u64(enc64.data(), dec64.data(), n);
    assert(src64 == dec64);
}

#if defined(__SSE2__)
void test_sse2_helpers() {
    alignas(16) std::int64_t src64[2] = {-10, 25};
    const __m128i packed64 = _mm_load_si128(reinterpret_cast<const __m128i*>(src64));
    const __m128i enc64 = dfh::compression::zigzag_encode_u64_sse2(packed64);
    const __m128i dec64 = dfh::compression::zigzag_decode_u64_sse2(enc64);

    alignas(16) std::int64_t out64[2] = {};
    _mm_store_si128(reinterpret_cast<__m128i*>(out64), dec64);
    assert(out64[0] == src64[0]);
    assert(out64[1] == src64[1]);

    alignas(16) std::int32_t src32[4] = {-10, 25, -1000, 2048};
    const __m128i packed32_in = _mm_load_si128(reinterpret_cast<const __m128i*>(src32));
    const __m128i enc32 = dfh::compression::zigzag_encode_u32_sse2(packed32_in);
    const __m128i dec32 = dfh::compression::zigzag_decode_u32_sse2(enc32);

    alignas(16) std::int32_t out32[4] = {};
    _mm_store_si128(reinterpret_cast<__m128i*>(out32), dec32);
    for (std::size_t i = 0; i < 4; ++i) {
        assert(out32[i] == src32[i]);
    }
}
#endif

#if defined(__AVX2__)
void test_avx2_helpers_and_arrays() {
    alignas(32) std::int64_t src64[4] = {-10, 25, -1000, 2048};
    const __m256i packed64 = _mm256_load_si256(reinterpret_cast<const __m256i*>(src64));
    const __m256i enc64 = dfh::compression::zigzag_encode_u64_avx2(packed64);
    const __m256i dec64 = dfh::compression::zigzag_decode_u64_avx2(enc64);

    alignas(32) std::int64_t out64[4] = {};
    _mm256_store_si256(reinterpret_cast<__m256i*>(out64), dec64);
    for (std::size_t i = 0; i < 4; ++i) {
        assert(out64[i] == src64[i]);
    }

    alignas(32) std::int32_t src32[8] = {-1, 0, 1, -2, 2, -3, 3, std::numeric_limits<std::int32_t>::min()};
    const __m256i packed32_in = _mm256_load_si256(reinterpret_cast<const __m256i*>(src32));
    const __m256i enc32 = dfh::compression::zigzag_encode_u32_avx2(packed32_in);
    const __m256i dec32 = dfh::compression::zigzag_decode_u32_avx2(enc32);
    alignas(32) std::int32_t out32[8] = {};
    _mm256_store_si256(reinterpret_cast<__m256i*>(out32), dec32);

    for (std::size_t i = 0; i < 8; ++i) {
        assert(out32[i] == src32[i]);
    }

    constexpr std::size_t size32 = 4097;
    AlignedVec64<std::int32_t> arr_src32(size32);
    AlignedVec64<std::uint32_t> arr_enc32(size32);
    AlignedVec64<std::int32_t> arr_dec32(size32);
    for (std::size_t i = 0; i < size32; ++i) {
        arr_src32[i] = (i % 2 == 0) ? static_cast<std::int32_t>(i * 11) : -static_cast<std::int32_t>(i * 13);
    }

    dfh::compression::encode_zig_zag_avx2_u32(arr_src32.data(), arr_enc32.data(), size32);
    dfh::compression::decode_zig_zag_avx2_u32(arr_enc32.data(), arr_dec32.data(), size32);
    assert(arr_src32 == arr_dec32);

    constexpr std::size_t size64 = 4099;
    AlignedVec64<std::int64_t> arr_src64(size64);
    AlignedVec64<std::uint64_t> arr_enc64(size64);
    AlignedVec64<std::int64_t> arr_dec64(size64);
    for (std::size_t i = 0; i < size64; ++i) {
        arr_src64[i] = (i % 2 == 0) ? static_cast<std::int64_t>(i * 1111111) : -static_cast<std::int64_t>(i * 2222222);
    }

    dfh::compression::encode_zig_zag_avx2_u64(arr_src64.data(), arr_enc64.data(), size64);
    dfh::compression::decode_zig_zag_avx2_u64(arr_enc64.data(), arr_dec64.data(), size64);
    assert(arr_src64 == arr_dec64);
}
#endif

#if defined(__AVX512F__)
void test_avx512_helpers() {
    alignas(64) std::int64_t src64[8] = {-1, 0, 1, -2, 2, -1234567, 1234567, std::numeric_limits<std::int64_t>::min()};
    const __m512i packed64 = _mm512_load_si512(reinterpret_cast<const __m512i*>(src64));
    const __m512i enc64 = dfh::compression::zigzag_encode_u64_avx512(packed64);
    const __m512i dec64 = dfh::compression::zigzag_decode_u64_avx512(enc64);

    alignas(64) std::int64_t out64[8] = {};
    _mm512_store_si512(reinterpret_cast<__m512i*>(out64), dec64);
    for (std::size_t i = 0; i < 8; ++i) {
        assert(out64[i] == src64[i]);
    }

    alignas(64) std::int32_t src32[16] = {-8,-7,-6,-5,-4,-3,-2,-1,0,1,2,3,4,5,6,7};
    const __m512i packed32_in = _mm512_load_si512(reinterpret_cast<const __m512i*>(src32));
    const __m512i enc32 = dfh::compression::zigzag_encode_u32_avx512(packed32_in);
    const __m512i dec32 = dfh::compression::zigzag_decode_u32_avx512(enc32);
    alignas(64) std::int32_t out32[16] = {};
    _mm512_store_si512(reinterpret_cast<__m512i*>(out32), dec32);
    for (std::size_t i = 0; i < 16; ++i) {
        assert(out32[i] == src32[i]);
    }
}
#endif

} // namespace

int main() {
    test_scalar_primitives_u32_u64();
    test_scalar_array_codecs_long_sequences();
    test_dispatch_array_codecs_long_sequences();

#if defined(__SSE2__)
    test_sse2_helpers();
#endif
#if defined(__AVX2__)
    test_avx2_helpers_and_arrays();
#endif
#if defined(__AVX512F__)
    test_avx512_helpers();
#endif

    return 0;
}
