#pragma once
#ifndef _DFH_COMPRESSION_TICK_COMPRESSOR_V1_TICK_COMPRESSION_CONTEXT_V1_HPP_INCLUDED
#define _DFH_COMPRESSION_TICK_COMPRESSOR_V1_TICK_COMPRESSION_CONTEXT_V1_HPP_INCLUDED

/// \file TickCompressionContextV1.hpp
/// \brief Defines the context used for tick data compression and decompression.

namespace dfh::compression {

    /// \class TickCompressionContextV1
    /// \brief Shared context for intermediate buffers used by tick compression/decompression.
    ///
    /// Stores reusable buffers for stages such as delta encoding, ZigZag, frequency encoding,
    /// and RLE processing.
    ///
    /// \note Buffers are allocated with 32-byte alignment to support AVX2 aligned loads/stores
    /// (e.g., _mm256_load_si256/_mm256_store_si256) in hot loops.
    class TickCompressionContextV1 {
    public:
        std::vector<std::uint32_t, dfh::utils::aligned_allocator<std::uint32_t, 32>> deltas_u32; ///< Stores 32-byte aligned 32-bit delta values.
        std::vector<std::uint64_t, dfh::utils::aligned_allocator<std::uint64_t, 32>> deltas_u64; ///< Stores 32-byte aligned 64-bit delta values.
        std::vector<std::uint32_t, dfh::utils::aligned_allocator<std::uint32_t, 32>> values_u32; ///< Stores 32-byte aligned unique 32-bit values used in frequency encoding.
        std::vector<std::uint64_t, dfh::utils::aligned_allocator<std::uint64_t, 32>> values_u64; ///< Stores 32-byte aligned unique 64-bit values used in frequency encoding.
        std::vector<std::uint32_t, dfh::utils::aligned_allocator<std::uint32_t, 32>> rle_u32;    ///< Stores 32-byte aligned 32-bit flagged RLE values.
        std::vector<std::uint8_t> processing_buffer; ///< General-purpose buffer for processing intermediate data.

        TickCompressionContextV1() = default;

        /// \brief Resets all buffers in the context.
        /// Prepares the context for a new compression/decompression operation.
        void reset() {
            deltas_u32.clear();
            deltas_u64.clear();
            values_u32.clear();
            values_u64.clear();
            rle_u32.clear();
            processing_buffer.clear();
        }
    };

}; // namespace dfh::compression

#endif // _DFH_COMPRESSION_TICK_COMPRESSOR_V1_TICK_COMPRESSION_CONTEXT_V1_HPP_INCLUDED
