/// \file test_zigzag_delta.cpp
/// \brief Demonstrates and tests the DFH library's zig-zag and delta encoding/decoding functions.

#include <iostream>
#include <vector>
#include <random>
#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <chrono>
#include <memory>
#include <DataFeedHub/src/compression/utils/zig_zag.hpp>
#include <DataFeedHub/src/compression/utils/zig_zag_delta.hpp>

#if defined(_WIN32)
#include <malloc.h>
#elif __APPLE__
#include <stdlib.h>
#else
#include <cstdlib>
#endif

namespace ha::alignment {

//-----------------------------------------------------------------------------
/*
 * https://en.cppreference.com/w/cpp/named_req/Allocator
 * https://en.cppreference.com/w/cpp/memory/allocator
 * https://github.com/boostorg/align/tree/develop/include/boost/align/detail
 */
//-----------------------------------------------------------------------------
template <typename T, std::size_t BYTE_ALIGNMENT>
struct aligned_allocator
{
    typedef T value_type;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef std::true_type propagate_on_container_move_assignment;
    typedef std::true_type is_always_equal;

    template <class U>
    struct rebind
    {
        typedef aligned_allocator<U, BYTE_ALIGNMENT> other;
    };

    aligned_allocator() = default;
    template <class U>
    constexpr aligned_allocator(const aligned_allocator<U, BYTE_ALIGNMENT>&) noexcept
    {
    }

    [[nodiscard]] T* allocate(std::size_t n)
    {
        static_assert(BYTE_ALIGNMENT == 8 || BYTE_ALIGNMENT == 16 || BYTE_ALIGNMENT == 32 ||
                      BYTE_ALIGNMENT == 64);

        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
            throw std::bad_alloc();
#if defined(_WIN32)
        if (auto p = static_cast<T*>(_aligned_malloc(n * sizeof(T), BYTE_ALIGNMENT)))
#elif __APPLE__
        void* pT = nullptr;
        if (::posix_memalign(&pT, BYTE_ALIGNMENT, n * sizeof(T)) == 0)
#else
        if (auto p = static_cast<T*>(std::aligned_alloc(BYTE_ALIGNMENT, n * sizeof(T))))
#endif
        {
#if defined(__APPLE__)
            auto p = static_cast<T*>(pT);
#endif
#if REPORT_ALLOCATIONS
            report(p, n);
#endif
            return p;
        }

        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
#if REPORT_ALLOCATIONS
        report(p, n, 0);
#endif
#if defined(_WIN32)
        _aligned_free(p);
#elif __APPLE__
        ::free(p);
#else
        std::free(p);
#endif
    }

private:
    void report(T* p, std::size_t n, bool alloc = true) const
    {
        std::cout << (alloc ? "Alloc: " : "Dealloc: ") << sizeof(T) * n << " bytes at " << std::hex
                  << std::showbase << reinterpret_cast<void*>(p) << std::dec << '\n';
    }
};

template <class T, class U, std::size_t BYTE_ALIGNMENT>
bool operator==(const aligned_allocator<T, BYTE_ALIGNMENT>&,
                const aligned_allocator<U, BYTE_ALIGNMENT>&)
{
    return true;
}
template <class T, class U, std::size_t BYTE_ALIGNMENT>
bool operator!=(const aligned_allocator<T, BYTE_ALIGNMENT>&,
                const aligned_allocator<U, BYTE_ALIGNMENT>&)
{
    return false;
}

//-----------------------------------------------------------------------------
} // namespace ha::alignment

/// \brief Generates a random int32_t array.
/// \param size Number of elements to generate.
/// \param min_value Minimum possible integer value.
/// \param max_value Maximum possible integer value.
/// \return A std::vector<int32_t> with random values in [min_value, max_value].
std::vector<int32_t, ha::alignment::aligned_allocator<int32_t, 16>> generate_random_int32_array(size_t size, int32_t min_value, int32_t max_value) {
    static std::mt19937_64 rng(12345ULL); // фиксированный seed для воспроизводимости
    std::uniform_int_distribution<int32_t> dist(min_value, max_value);

    std::vector<int32_t, ha::alignment::aligned_allocator<int32_t, 16>> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = dist(rng);
    }
    return data;
}

std::vector<uint32_t, ha::alignment::aligned_allocator<uint32_t, 16>> generate_random_uint32_array(size_t size, uint32_t min_value, uint32_t max_value) {
    static std::mt19937_64 rng(12345ULL); // фиксированный seed для воспроизводимости
    std::uniform_int_distribution<uint32_t> dist(min_value, max_value);

    std::vector<uint32_t, ha::alignment::aligned_allocator<uint32_t, 16>> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = dist(rng);
    }
    return data;
}

/// \brief Generates a random int64_t array.
/// \param size Number of elements to generate.
/// \param min_value Minimum possible integer value.
/// \param max_value Maximum possible integer value.
/// \return A std::vector<int64_t> with random values in [min_value, max_value].
std::vector<int64_t, ha::alignment::aligned_allocator<int64_t, 16>> generate_random_int64_array(size_t size, int64_t min_value, int64_t max_value) {
    static std::mt19937_64 rng(67890ULL); // другой seed
    std::uniform_int_distribution<int64_t> dist(min_value, max_value);

    std::vector<int64_t, ha::alignment::aligned_allocator<int64_t, 16>> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = dist(rng);
    }
    return data;
}

std::vector<uint64_t, ha::alignment::aligned_allocator<uint64_t, 16>> generate_random_uint64_array(size_t size, uint64_t min_value, uint64_t max_value) {
    static std::mt19937_64 rng(67890ULL); // другой seed
    std::uniform_int_distribution<uint64_t> dist(min_value, max_value);

    std::vector<uint64_t, ha::alignment::aligned_allocator<uint64_t, 16>> data(size);
    for (size_t i = 0; i < size; ++i) {
        data[i] = dist(rng);
    }
    return data;
}

/// \brief Checks if two arrays of int32_t are equal.
/// \param a The first array.
/// \param b The second array.
/// \return true if they have the same size and elements, false otherwise.
template<class T>
bool arrays_equal(const T& a, const T& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) {
            std::cout << "Input[" << i << "]: " << a[i] << ", Output: " << b[i] << std::endl;
            return false;
        }
    }
    return true;
}

/// \brief Checks if two arrays of int64_t are equal.
/// \param a The first array.
/// \param b The second array.
/// \return true if they have the same size and elements, false otherwise.
template<class T>
bool arrays_equal64(const T& a, const T& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) {
            std::cout << "Input[" << i << "]: " << a[i] << ", Output: " << b[i] << std::endl;
            return false;
        }
    }
    return true;
}

/// \brief Simple test routine for 32-bit zig-zag (encode/decode).
void test_zigzag32(size_t size) {
    std::cout << "[Test ZigZag-32] size = " << size << "\n";

    // Generate random data
    auto original = generate_random_int32_array(size, -100000, 100000);

    // Encode
    std::vector<uint32_t, ha::alignment::aligned_allocator<uint32_t, 16>> encoded(size);
    dfh::compression::encode_zig_zag_int32(original.data(), encoded.data(), size);

    // Decode
    std::vector<int32_t, ha::alignment::aligned_allocator<int32_t, 16>> decoded(size);
    dfh::compression::decode_zig_zag_int32(encoded.data(), decoded.data(), size);

    // Compare
    bool ok = arrays_equal(original, decoded);
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}

/// \brief Simple test routine for 64-bit zig-zag (encode/decode).
void test_zigzag64(size_t size) {
    std::cout << "[Test ZigZag-64] size = " << size << "\n";

    // Generate random data
    auto original = generate_random_int64_array(size, -1000000LL, 1000000LL);

    // Encode
    std::vector<uint64_t, ha::alignment::aligned_allocator<uint64_t, 16>> encoded(size);
    dfh::compression::encode_zig_zag_int64(original.data(), encoded.data(), size);

    // Decode
    std::vector<int64_t, ha::alignment::aligned_allocator<int64_t, 16>> decoded(size);
    dfh::compression::decode_zig_zag_int64(encoded.data(), decoded.data(), size);

    // Compare
    bool ok = arrays_equal64(original, decoded);
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}

/// \brief Tests classic delta + zig-zag encoding/decoding for 32-bit with a given initial value.
void test_delta_zigzag32(size_t size, int32_t initial_value) {
    std::cout << "[Test DeltaZigZag-32] size = " << size
              << ", initial_value = " << initial_value << "\n";

    // Generate random data
    auto original = generate_random_int32_array(size, -100000, 100000);

    // Encode
    std::vector<uint32_t, ha::alignment::aligned_allocator<uint32_t, 16>> encoded(size);
    dfh::compression::encode_delta_zig_zag_int32(original.data(), encoded.data(), size, initial_value);

    // Decode
    std::vector<int32_t, ha::alignment::aligned_allocator<int32_t, 16>> decoded(size);
    dfh::compression::decode_delta_zig_zag_int32(encoded.data(), decoded.data(), size, initial_value);

    // Compare
    bool ok = arrays_equal(original, decoded);
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}

void test_delta_zigzag_u32(size_t size, uint32_t initial_value) {
    std::cout << "[Test DeltaZigZag-u32] size = " << size
              << ", initial_value = " << initial_value << "\n";

    // Generate random data
    auto original = generate_random_uint32_array(size, 0, 100000);

    // Encode
    std::vector<uint32_t, ha::alignment::aligned_allocator<uint32_t, 16>> encoded(size);
    dfh::compression::encode_delta_zig_zag_int32(original.data(), encoded.data(), size, initial_value);

    // Decode
    std::vector<uint32_t, ha::alignment::aligned_allocator<uint32_t, 16>> decoded(size);
    dfh::compression::decode_delta_zig_zag_int32(encoded.data(), decoded.data(), size, initial_value);

    // Compare
    bool ok = arrays_equal(original, decoded);
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}

/// \brief Tests classic delta + zig-zag encoding/decoding for 64-bit with a given initial value.
void test_delta_zigzag64(size_t size, int64_t initial_value) {
    std::cout << "[Test DeltaZigZag-64] size = " << size
              << ", initial_value = " << initial_value << "\n";

    // Generate random data
    auto original = generate_random_int64_array(size, -100000, 100000);

    // Encode
    std::vector<uint64_t, ha::alignment::aligned_allocator<uint64_t, 16>> encoded(size);
    dfh::compression::encode_delta_zig_zag_int64(original.data(), encoded.data(), size, initial_value);

    // Decode
    std::vector<int64_t, ha::alignment::aligned_allocator<int64_t, 16>> decoded(size);
    dfh::compression::decode_delta_zig_zag_int64(encoded.data(), decoded.data(), size, initial_value);

    // Compare
    bool ok = arrays_equal64(original, decoded);
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}

void test_delta_zigzag_u64(size_t size, uint64_t initial_value) {
    std::cout << "[Test DeltaZigZag-u64] size = " << size
              << ", initial_value = " << initial_value << "\n";

    // Generate random data
    auto original = generate_random_uint64_array(size, 0, 100000);

    // Encode
    std::vector<uint64_t, ha::alignment::aligned_allocator<uint64_t, 16>> encoded(size);
    dfh::compression::encode_delta_zig_zag_int64(original.data(), encoded.data(), size, initial_value);

    // Decode
    std::vector<uint64_t, ha::alignment::aligned_allocator<uint64_t, 16>> decoded(size);
    dfh::compression::decode_delta_zig_zag_int64(encoded.data(), decoded.data(), size, initial_value);

    // Compare
    bool ok = arrays_equal(original, decoded);
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}

/// \brief Tests chunk-based delta + zig-zag encoding/decoding for 32-bit with a given initial value.
void test_delta_zigzag32_chunked(size_t size, int32_t initial_value) {
    std::cout << "[Test DeltaZigZag-32 Chunked] size = " << size
              << ", initial_value = " << initial_value << "\n";

    // Generate random data
    auto original = generate_random_int32_array(size, -200000, 200000);

    // Encode (chunked)
    std::vector<uint32_t, ha::alignment::aligned_allocator<uint32_t, 16>> encoded(size);
    dfh::compression::encode_delta_zig_zag_chunked4_int32(original.data(), encoded.data(), size, initial_value);

    // Decode (chunked)
    std::vector<int32_t, ha::alignment::aligned_allocator<int32_t, 16>> decoded(size);
    dfh::compression::decode_delta_zig_zag_chunked4_int32(encoded.data(), decoded.data(), size, initial_value);

    // Compare
    bool ok = arrays_equal(original, decoded);
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}

/// \brief Tests chunk-based delta + zig-zag encoding/decoding for 64-bit with a given initial value.
void test_delta_zigzag64_chunked(size_t size, int64_t initial_value) {
    std::cout << "[Test DeltaZigZag-64 Chunked-4] size = " << size
              << ", initial_value = " << initial_value << "\n";

    // Generate random data
    auto original = generate_random_int64_array(size, -100000000LL, 100000000LL);

    // Encode (chunked, 64-bit)
    std::vector<uint64_t, ha::alignment::aligned_allocator<uint64_t, 16>> encoded(size);
    dfh::compression::encode_delta_zig_zag_chunked4_int64(original.data(), encoded.data(), size, initial_value);

    // Decode (chunked, 64-bit)
    std::vector<int64_t, ha::alignment::aligned_allocator<int64_t, 16>> decoded(size);
    dfh::compression::decode_delta_zig_zag_chunked4_int64(encoded.data(), decoded.data(), size, initial_value);

    // Compare
    bool ok = arrays_equal64(original, decoded);
    std::cout << "  => " << (ok ? "OK" : "FAIL") << "\n\n";
    assert(ok);
}

namespace dfh::compression {

    // Скалярные реализации для сравнения
    void scalar_encode_delta_zig_zag_int32(const int32_t* input, uint32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;
        int32_t prev = initial_value;
        for (size_t i = 0; i < size; ++i) {
            int32_t delta = input[i] - prev;
            output[i] = (delta << 1) ^ (delta >> 31);
            prev = input[i];
        }
    }

    void scalar_decode_delta_zig_zag_int32(const uint32_t* input, int32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;
        int32_t prev = initial_value;
        for (size_t i = 0; i < size; ++i) {
            int32_t zigzag = (input[i] >> 1) ^ -(input[i] & 1);
            output[i] = prev + zigzag;
            prev = output[i];
        }
    }

    void scalar_encode_delta_zig_zag_int64(const int64_t* input, uint64_t* output, size_t size, int64_t initial_value) {
        if (size == 0) return;
        int64_t prev = initial_value;
        for (size_t i = 0; i < size; ++i) {
            int64_t delta = input[i] - prev;
            output[i] = (static_cast<uint64_t>(delta) << 1) ^ static_cast<uint64_t>(delta >> 63);
            prev = input[i];
        }
    }

    void scalar_decode_delta_zig_zag_int64(const uint64_t* input, int64_t* output, size_t size, int64_t initial_value) {
        if (size == 0) return;
        int64_t prev = initial_value;
        for (size_t i = 0; i < size; ++i) {
            int64_t zigzag = (input[i] >> 1) ^ -(input[i] & 1);
            output[i] = prev + zigzag;
            prev = output[i];
        }
    }

    void scalar_encode_delta_zig_zag_chunked8_int32(const int32_t* input, uint32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 8;
        const size_t aligned_size = size - (size % simd_width);

        int32_t delta_value;
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                delta_value = input[j] - initial_value;
                output[j] = (delta_value << 1) ^ (delta_value >> 31);
            }
            initial_value = input[j_max - 1];
        }
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
        }
    }

    void scalar_decode_delta_zig_zag_chunked8_int32(const uint32_t* input, int32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 8;
        const size_t aligned_size = size - (size % simd_width);

        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                output[j] = initial_value + ((input[j] >> 1) ^ -(input[j] & 1));
                initial_value = output[j];
            }
        }
        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
            initial_value = output[i];
        }
    }

    void scalar_encode_delta_zig_zag_chunked4_int32(const int32_t* input, uint32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 4;
        const size_t aligned_size = size - (size % simd_width);

        int32_t delta_value;
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                delta_value = input[j] - initial_value;
                output[j] = (delta_value << 1) ^ (delta_value >> 31);
            }
            initial_value = input[j_max - 1];
        }
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
        }
    }

    void scalar_decode_delta_zig_zag_chunked4_int32(const uint32_t* input, int32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 4;
        const size_t aligned_size = size - (size % simd_width);

        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                output[j] = initial_value + ((input[j] >> 1) ^ -(input[j] & 1));
                initial_value = output[j];
            }
        }
        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
            initial_value = output[i];
        }
    }

    void scalar_encode_delta_zig_zag_chunked4_int64(const int64_t* input, uint64_t* output, size_t size, int64_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 4;
        const size_t aligned_size = size - (size % simd_width);

        int64_t delta_value;
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                delta_value = input[j] - initial_value;
                output[j] = (delta_value << 1) ^ (delta_value >> 63);
            }
            initial_value = input[j_max - 1];
        }
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 63);
        }
    }

    void scalar_decode_delta_zig_zag_chunked4_int64(const uint64_t* input, int64_t* output, size_t size, int64_t initial_value) {
        if (size == 0) return;

        constexpr size_t chunk_width = 4;
        const size_t aligned_size = size - (size % chunk_width);

        for (size_t i = 0; i < aligned_size; i += chunk_width) {
            size_t j_max = i + chunk_width;
            for (size_t j = i; j < j_max; ++j) {
                output[j] = initial_value + ((input[j] >> 1) ^ -(input[j] & 1));
                initial_value = output[j];
            }
            initial_value = output[j_max - 1];
        }
        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
            initial_value = output[i];
        }
    }

} // namespace dfh::compression

namespace dfh::compression {

    void simd_encode_delta_zig_zag_int32(const int32_t* input, uint32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;
        int32_t prev = initial_value, delta_value;
#       if defined(__SSE2__)
        constexpr size_t simd_width = 4;
        const size_t aligned_size = size - ((size - 1) % simd_width);
        __m128i delta;

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            if (aligned_size > 1) {
                for (size_t i = 0; i < simd_width; ++i) {
                    delta_value = input[i] - prev;
                    output[i] = (delta_value << 1) ^ (delta_value >> 31);
                    prev = input[i];
                }
            }
            for (size_t i = simd_width; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i - 1])));

                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1),
                        _mm_srai_epi32(delta, 31)));
            }
        } else {
            for (size_t i = 1; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i - 1])));

                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1),
                        _mm_srai_epi32(delta, 31)));
            }
        }

        delta_value = input[0] - initial_value;
        output[0] = (delta_value << 1) ^ (delta_value >> 31);
        prev = aligned_size > 0 ? input[aligned_size - 1] : initial_value;
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - prev;
            output[i] = (static_cast<uint32_t>(delta_value) << 1) ^ static_cast<uint32_t>(delta_value >> 31);
            prev = input[i];
        }
#       else
        prev = initial_value;
        for (size_t i = 0; i < size; ++i) {
            delta_value = input[i] - prev;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
            prev = input[i];
        }
#       endif
    }

    void simd_decode_delta_zig_zag_int32(const uint32_t* input, int32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;
#       if defined(__SSE2__)
        constexpr size_t simd_width = 4; // SSE2 обрабатывает 4 int32 за раз
        const size_t aligned_size = size - (size % simd_width);

        const __m128i zero = _mm_setzero_si128();
        const __m128i one = _mm_set1_epi32(1);
        __m128i vec;

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                vec = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_srli_epi32(vec, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(vec, one))));
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_srli_epi32(vec, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(vec, one))));
            }
        }

        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = (input[i] >> 1) ^ -(input[i] & 1);
        }

        output[0] = initial_value + output[0];
        for (size_t i = 1; i < size; ++i) {
            output[i] += output[i - 1];
        }
#       else
        int32_t zigzag = (input[0] >> 1) ^ -(input[0] & 1);
        output[0] = initial_value + zigzag;
        for (size_t i = 1; i < size; ++i) {
            zigzag = (input[i] >> 1) ^ -(input[i] & 1);
            output[i] = output[i - 1] + zigzag;
        }
#       endif
    }

    void simd_encode_delta_zig_zag_int64(const int64_t* input, uint64_t* output, size_t size, int64_t initial_value) {
        if (size == 0) return;
        int64_t prev = initial_value, delta_value;
#       if defined(__SSE2__)
        constexpr size_t simd_width = 2; // SSE2 обрабатывает 2 int64 за раз
        const size_t aligned_size = size - ((size - 1) % simd_width);
        __m128i delta;

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            if (aligned_size > 1) {
                for (size_t i = 0; i < size; ++i) {
                    delta_value = input[i] - prev;
                    output[i] = (delta_value << 1) ^ (delta_value >> 63);
                    prev = input[i];
                }
            }
            for (size_t i = simd_width; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi64(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i - 1])));

                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))
                    ));
            }
        } else {
            for (size_t i = 1; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi64(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i - 1])));

                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))
                    ));
            }
        }

        delta_value = input[0] - initial_value;
        output[0] = (delta_value << 1) ^ (delta_value >> 63);
        prev = aligned_size > 0 ? input[aligned_size - 1] : initial_value;
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - prev;
            output[i] = (delta_value << 1) ^ (delta_value >> 63);
            prev = input[i];
        }
#       else
        for (size_t i = 0; i < size; ++i) {
            delta_value = input[i] - prev;
            output[i] = (delta_value << 1) ^ (delta_value >> 63);
            prev = input[i];
        }
#       endif
    }

    void simd_decode_delta_zig_zag_int64(const uint64_t* input, int64_t* output, size_t size, int64_t initial_value) {
        if (size == 0) return;
#       if defined(__SSE2__)
        constexpr size_t simd_width = 2; // SSE2 обрабатывает 2 int64 за раз
        const size_t aligned_size = size - (size % simd_width);

        const __m128i zero = _mm_setzero_si128();
        const __m128i one = _mm_set1_epi64x(1);
        __m128i vec;

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                vec = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_store_si128(
                    reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(
                        _mm_srli_epi64(vec, 1),
                        _mm_sub_epi64(zero, _mm_and_si128(vec, one))));
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_storeu_si128(
                    reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(
                        _mm_srli_epi64(vec, 1),
                        _mm_sub_epi64(zero, _mm_and_si128(vec, one))));
            }
        }

        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = (input[i] >> 1) ^ -(input[i] & 1);
        }
        output[0] = initial_value + output[0];
        for (size_t i = 1; i < size; ++i) {
            output[i] += output[i - 1];
        }
#       else
        int64_t zigzag = (input[0] >> 1) ^ -(input[0] & 1);
        output[0] = initial_value + zigzag;
        for (size_t i = 1; i < size; ++i) {
            zigzag = (input[i] >> 1) ^ -(input[i] & 1);
            output[i] = output[i - 1] + zigzag;
        }
#       endif
    }

    void simd_encode_delta_zig_zag_chunked8_int32(const int32_t* input, uint32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 8;
        const size_t aligned_size = size - (size % simd_width);

#       if defined(__SSE2__)
        constexpr size_t sse_simd_width = 4;

        __m128i base = _mm_set1_epi32(initial_value);
        __m128i delta;

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                delta = _mm_sub_epi32(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width])),
                    base);
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                base = _mm_set1_epi32(input[i + simd_width - 1]);
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                delta = _mm_sub_epi32(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width])),
                    base);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                base = _mm_set1_epi32(input[i + simd_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = input[aligned_size - 1];
        }

        int32_t delta_value;
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
        }
#       else
        int32_t delta_value;
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                delta_value = input[j] - initial_value;
                output[j] = (delta_value << 1) ^ (delta_value >> 31);
            }
            initial_value = input[j_max - 1];
        }
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
        }
#       endif
    }

    void simd_decode_delta_zig_zag_chunked8_int32(const uint32_t* input, int32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 8;
        const size_t aligned_size = size - (size % simd_width);

#       if defined(__SSE2__)
        constexpr size_t sse_simd_width = 4;

        __m128i base = _mm_set1_epi32(initial_value);
        __m128i delta;

        const __m128i one = _mm_set1_epi32(1);
        const __m128i zero = _mm_setzero_si128();

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                delta = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width]));
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi32(output[i + simd_width - 1]);
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                delta = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width]));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi32(output[i + simd_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = output[aligned_size - 1];
        }

        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
        }
#       else
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                output[j] = initial_value + ((input[j] >> 1) ^ -(input[j] & 1));
                initial_value = output[j];
            }
        }
        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
            initial_value = output[i];
        }
#       endif
    }

    void simd_encode_delta_zig_zag_chunked4_int32(const int32_t* input, uint32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 4;
        const size_t aligned_size = size - (size % simd_width);

#       if defined(__SSE2__)
        __m128i base = _mm_set1_epi32(initial_value);
        __m128i delta;

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                base = _mm_set1_epi32(input[i + simd_width - 1]);
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                base = _mm_set1_epi32(input[i + simd_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = input[aligned_size - 1];
        }

        int32_t delta_value;
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
        }
#       else
        int32_t delta_value;
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                delta_value = input[j] - initial_value;
                output[j] = (delta_value << 1) ^ (delta_value >> 31);
            }
            initial_value = input[j_max - 1];
        }
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
        }
#       endif
    }

    void simd_decode_delta_zig_zag_chunked4_int32(const uint32_t* input, int32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 4;
        const size_t aligned_size = size - (size % simd_width);

#       if defined(__SSE2__)
        __m128i base = _mm_set1_epi32(initial_value);
        __m128i delta;

        const __m128i one = _mm_set1_epi32(1);
        const __m128i zero = _mm_setzero_si128();

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi32(output[i + simd_width - 1]);
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi32(output[i + simd_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = output[aligned_size - 1];
        }

        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
        }
#       else
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                output[j] = initial_value + ((input[j] >> 1) ^ -(input[j] & 1));
                initial_value = output[j];
            }
        }
        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
            initial_value = output[i];
        }
#       endif
    }

    void simd_encode_delta_zig_zag_chunked4_int64(const int64_t* input, uint64_t* output, size_t size, int64_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 4;
        const size_t aligned_size = size - (size % simd_width);

#       if defined(__SSE2__)
        constexpr size_t sse_simd_width = 2;

        __m128i base = _mm_set1_epi64x(initial_value);
        __m128i delta;

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi64(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))));


                delta = _mm_sub_epi64(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width])),
                    base);
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))));

                base = _mm_set1_epi64x(input[i + simd_width - 1]);
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi64(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))));


                delta = _mm_sub_epi64(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width])),
                    base);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))));

                base = _mm_set1_epi64x(input[i + simd_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = input[aligned_size - 1];
        }

        int64_t delta_value;
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 63);
        }
#       else
        int64_t delta_value;
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                delta_value = input[j] - initial_value;
                output[j] = (delta_value << 1) ^ (delta_value >> 63);
            }
            initial_value = input[j_max - 1];
        }
        for (size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 63);
        }
#       endif
    }

    void simd_decode_delta_zig_zag_chunked4_int64(const uint64_t* input, int64_t* output, size_t size, int64_t initial_value) {
        if (size == 0) return;

        constexpr size_t chunk_width = 4;
        const size_t aligned_size = size - (size % chunk_width);

#       if defined(__SSE2__)
        constexpr size_t sse_simd_width = 2;
        __m128i base = _mm_set1_epi64x(initial_value);
        __m128i delta;

        const __m128i one = _mm_set1_epi64x(1);
        const __m128i zero = _mm_setzero_si128();

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += chunk_width) {
                delta = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i]));

                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi64(_mm_xor_si128(
                        _mm_srli_epi64(delta, 1),
                        _mm_sub_epi64(zero, _mm_and_si128(delta, one))), base));

                delta = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width]));

                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_add_epi64(_mm_xor_si128(
                        _mm_srli_epi64(delta, 1),
                        _mm_sub_epi64(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi64x(output[i + chunk_width - 1]);
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += chunk_width) {
                delta = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i]));

                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi64(_mm_xor_si128(
                        _mm_srli_epi64(delta, 1),
                        _mm_sub_epi64(zero, _mm_and_si128(delta, one))), base));

                delta = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width]));

                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_add_epi64(_mm_xor_si128(
                        _mm_srli_epi64(delta, 1),
                        _mm_sub_epi64(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi64x(output[i + chunk_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = output[aligned_size - 1];
        }

        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
        }
#       else
        for (size_t i = 0; i < aligned_size; i += chunk_width) {
            size_t j_max = i + chunk_width;
            for (size_t j = i; j < j_max; ++j) {
                output[j] = initial_value + ((input[j] >> 1) ^ -(input[j] & 1));
                initial_value = output[j];
            }
            initial_value = output[j_max - 1];
        }
        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
            initial_value = output[i];
        }
#       endif
    }

} // namespace dfh::compression

namespace dfh::compression {

    void simd_encode_zig_zag_int32(const int32_t* input, uint32_t* output, size_t size) {
#       if defined(__SSE2__)
        constexpr size_t simd_width = 4; // SSE2 обрабатывает 4 int32 за раз
        const size_t aligned_size = size - (size % simd_width);

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                __m128i vec = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(vec, 1), _mm_srai_epi32(vec, 31)));
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                __m128i vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(vec, 1), _mm_srai_epi32(vec, 31)));
            }
        }

        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = (input[i] << 1) ^ (input[i] >> 31);
        }
#       else
        for (size_t i = 0; i < size; ++i) {
            output[i] = (input[i] << 1) ^ (input[i] >> 31);
        }
#       endif
    }

    void simd_decode_zig_zag_int32(const uint32_t* input, int32_t* output, size_t size) {
#       if defined(__SSE2__)
        constexpr size_t simd_width = 4; // SSE2 обрабатывает 4 int32 за раз
        const size_t aligned_size = size - (size % simd_width);

        const __m128i zero = _mm_setzero_si128();
        const __m128i one = _mm_set1_epi32(1);

        if (reinterpret_cast<uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                __m128i vec = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_srli_epi32(vec, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(vec, one))));
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                __m128i vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_srli_epi32(vec, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(vec, one))));
            }
        }

        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = (input[i] >> 1) ^ -(input[i] & 1);
        }
#       else
        for (size_t i = 0; i < size; ++i) {
            output[i] = (input[i] >> 1) ^ -(input[i] & 1);
        }
#       endif
    }

    void scalar_encode_zig_zag_int32(const int32_t* input, uint32_t* output, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            output[i] = (input[i] << 1) ^ (input[i] >> 31);
        }
    }

    void scalar_decode_zig_zag_int32(const uint32_t* input, int32_t* output, size_t size) {
        for (size_t i = 0; i < size; ++i) {
            output[i] = (input[i] >> 1) ^ -(input[i] & 1);
        }
    }

} // namespace dfh::compression

// Функция для измерения времени выполнения
template <typename Func>
double measure_time(Func func, size_t iterations = 10) {
    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < iterations; ++i) {
        func();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    return elapsed.count() / iterations;
}

template <typename Func1, typename Func2>
void measure_time(Func1 func1, Func2 func2, size_t iterations = 10) {
    double total_time1 = 0.0;
    double total_time2 = 0.0;
    for (size_t i = 0; i < iterations; ++i) {
        auto start_time1 = std::chrono::high_resolution_clock::now();
        func1();
        auto end_time1 = std::chrono::high_resolution_clock::now();

        auto start_time2 = std::chrono::high_resolution_clock::now();
        func2();
        auto end_time2 = std::chrono::high_resolution_clock::now();

        auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end_time1 - start_time1).count();
        auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end_time2 - start_time2).count();

        total_time1 += static_cast<double>(duration1);
        total_time2 += static_cast<double>(duration2);
    }

    double average_time1 = total_time1 / iterations;
    double average_time2 = total_time2 / iterations;

    std::cout << "Avg time 1/2: "
        << average_time1 << " us; "
        << average_time2 << " us; "
        << iterations << " iterations." << std::endl;
}

// Тестирование скорости выполнения
void test_speed_zig_zag() {
    std::cout << "Test speed zig zag\n";
    const size_t size = 1000000; // Размер массива
    const size_t iterations = 1000; // Количество повторений для замеров

    auto data32 = generate_random_int32_array(size, -100000, 100000);
    auto data64 = generate_random_int64_array(size, -100000LL, 100000LL);

    std::vector<uint32_t, ha::alignment::aligned_allocator<uint32_t, 16>> encoded32(size);
    std::vector<int32_t, ha::alignment::aligned_allocator<int32_t, 16>> decoded32(size);
    std::vector<uint64_t, ha::alignment::aligned_allocator<uint64_t, 16>> encoded64(size);
    std::vector<int64_t, ha::alignment::aligned_allocator<int64_t, 16>> decoded64(size);

    // Тест 32-бит
    std::cout << "32-bit Encode:\n";
    measure_time([&]() {
        dfh::compression::simd_encode_zig_zag_int32(data32.data(), encoded32.data(), size);
    }, [&]() {
        dfh::compression::scalar_encode_zig_zag_int32(data32.data(), encoded32.data(), size);
    }, iterations);

    std::cout << "32-bit Decode:\n";
    measure_time([&]() {
        dfh::compression::simd_decode_zig_zag_int32(encoded32.data(), decoded32.data(), size);
    }, [&]() {
        dfh::compression::scalar_decode_zig_zag_int32(encoded32.data(), decoded32.data(), size);
    }, iterations);

    std::cout << "\n";
}

void test_speed_delta_zig_zag() {
    std::cout << "Test speed delta zig zag\n";
    const size_t size = 1000000; // Размер массива
    const size_t iterations = 1000; // Количество повторений для замеров
    const int32_t initial_value_32 = 1000;
    const int64_t initial_value_64 = 1000LL;

    auto data32 = generate_random_int32_array(size, -100000, 100000);
    auto data64 = generate_random_int64_array(size, -100000LL, 100000LL);

    std::vector<uint32_t> encoded32(size);
    std::vector<int32_t> decoded32(size);
    std::vector<uint64_t> encoded64(size);
    std::vector<int64_t> decoded64(size);

    // Тест 32-бит
    std::cout << "32-bit Encode:\n";
    measure_time([&]() {
        dfh::compression::simd_encode_delta_zig_zag_int32(data32.data(), encoded32.data(), size, initial_value_32);
    }, [&]() {
        dfh::compression::scalar_encode_delta_zig_zag_int32(data32.data(), encoded32.data(), size, initial_value_32);
    }, iterations);

    std::cout << "32-bit Decode:\n";
    measure_time([&]() {
        dfh::compression::simd_decode_delta_zig_zag_int32(encoded32.data(), decoded32.data(), size, initial_value_32);
    }, [&]() {
        dfh::compression::scalar_decode_delta_zig_zag_int32(encoded32.data(), decoded32.data(), size, initial_value_32);
    }, iterations);

    // Тест 64-бит
    std::cout << "64-bit Encode:\n";
    measure_time([&]() {
        dfh::compression::simd_encode_delta_zig_zag_int64(data64.data(), encoded64.data(), size, initial_value_64);
    }, [&]() {
        dfh::compression::scalar_encode_delta_zig_zag_int64(data64.data(), encoded64.data(), size, initial_value_64);
    }, iterations);

    std::cout << "64-bit Decode:\n";
    measure_time([&]() {
        dfh::compression::simd_decode_delta_zig_zag_int64(encoded64.data(), decoded64.data(), size, initial_value_64);
    }, [&]() {
        dfh::compression::scalar_decode_delta_zig_zag_int64(encoded64.data(), decoded64.data(), size, initial_value_64);
    }, iterations);

    std::cout << "\n";
}

void test_speed_delta_zig_zag_chunked_speed() {
    std::cout << "Test speed delta zig zag chunked\n";
    const size_t size = 1000000; // Размер массива
    const size_t iterations = 1000; // Количество повторений для замеров
    const int32_t initial_value_32 = 1000;
    const int64_t initial_value_64 = 1000LL;

    auto data32 = generate_random_int32_array(size, -100000, 100000);
    auto data64 = generate_random_int64_array(size, -100000LL, 100000LL);

    std::vector<uint32_t> encoded32(size);
    std::vector<int32_t> decoded32(size);
    std::vector<uint64_t> encoded64(size);
    std::vector<int64_t> decoded64(size);

    // Тест 32-бит
    std::cout << "32-bit Encode:\n";
    measure_time([&]() {
        dfh::compression::simd_encode_delta_zig_zag_chunked4_int32(data32.data(), encoded32.data(), size, initial_value_32);
    }, [&]() {
        dfh::compression::scalar_encode_delta_zig_zag_chunked4_int32(data32.data(), encoded32.data(), size, initial_value_32);
    }, iterations);

    std::cout << "32-bit Decode:\n";
    measure_time([&]() {
        dfh::compression::simd_decode_delta_zig_zag_chunked4_int32(encoded32.data(), decoded32.data(), size, initial_value_32);
    }, [&]() {
        dfh::compression::scalar_decode_delta_zig_zag_chunked4_int32(encoded32.data(), decoded32.data(), size, initial_value_32);
    }, iterations);

    // Тест 64-бит
    std::cout << "64-bit Encode:\n";
    measure_time([&]() {
        dfh::compression::simd_encode_delta_zig_zag_chunked4_int64(data64.data(), encoded64.data(), size, initial_value_64);
    }, [&]() {
        dfh::compression::scalar_encode_delta_zig_zag_chunked4_int64(data64.data(), encoded64.data(), size, initial_value_64);
    }, iterations);

    std::cout << "64-bit Decode:\n";
    measure_time([&]() {
        dfh::compression::simd_decode_delta_zig_zag_chunked4_int64(encoded64.data(), decoded64.data(), size, initial_value_64);
    }, [&]() {
        dfh::compression::scalar_decode_delta_zig_zag_chunked4_int64(encoded64.data(), decoded64.data(), size, initial_value_64);
    }, iterations);

    std::cout << "\n";
}

/// \brief Entry point: run all tests with various sizes.
int main() {
    // 1) Тестируем ZigZag 32-бит и 64-бит
    test_zigzag32(1);       // Минимальный размер
    test_zigzag32(7);       // Не кратно 8
    test_zigzag32(16);      // Больше 8
    test_zigzag64(1);
    test_zigzag64(5);       // Не кратно 4
    test_zigzag64(12);      // Кратно 4

    // 2) Тестируем классическую дельту + ZigZag (32-бит)
    test_delta_zigzag32(1,  1000);
    test_delta_zigzag32(2,  1000);
    test_delta_zigzag32(4,  1000);
    test_delta_zigzag32(5,  1000);
    test_delta_zigzag32(10, -5000);
    test_delta_zigzag32(16,  0);
    test_delta_zigzag32(17,  0);
    test_delta_zigzag_u32(1,  1000);
    test_delta_zigzag_u32(2,  1000);
    test_delta_zigzag_u32(4,  1000);
    test_delta_zigzag_u32(5,  1000);
    test_delta_zigzag_u32(10, 5000);
    test_delta_zigzag_u32(16,  0);
    test_delta_zigzag_u32(17,  0);

    // 3) Тестируем классическую дельту + ZigZag (64-бит)
    test_delta_zigzag64(1,  1000);
    test_delta_zigzag64(5,  -5000);
    test_delta_zigzag64(12, 0);
    test_delta_zigzag_u64(1,  1000);
    test_delta_zigzag_u64(5,  5000);
    test_delta_zigzag_u64(12, 0);

    // 4) Тестируем блочные (chunked) 32-бит
    test_delta_zigzag32_chunked(1,   100);
    test_delta_zigzag32_chunked(14, -100);
    test_delta_zigzag32_chunked(24,  0);

    // 5) Тестируем блочные (chunked) 64-бит
    test_delta_zigzag64_chunked(1,  1000000LL);
    test_delta_zigzag64_chunked(6, -7777777LL);
    test_delta_zigzag64_chunked(12, 0LL);

    // 6) Тестирование скорости выполнения
    test_speed_zig_zag();
    test_speed_delta_zig_zag();
    test_speed_delta_zig_zag_chunked_speed();

    std::cout << "All tests passed!\n";
    return 0;
}
