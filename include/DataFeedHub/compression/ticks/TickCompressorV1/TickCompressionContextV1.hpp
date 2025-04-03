#pragma once
#ifndef _DFH_COMPRESSION_TICK_COMPRESSOR_V1_TICK_COMPRESSION_CONTEXT_V1_HPP_INCLUDED
#define _DFH_COMPRESSION_TICK_COMPRESSOR_V1_TICK_COMPRESSION_CONTEXT_V1_HPP_INCLUDED

/// \file TickCompressionContextV1.hpp
/// \brief Defines the context used for tick data compression and decompression.

namespace dfh::compression {

    /// \class TickCompressionContextV1
    /// \brief Provides a shared context for intermediate data during tick compression and decompression.
    ///
    /// This class manages several buffers required for operations such as encoding,
    /// decoding, and frequency analysis. Each buffer is allocated with alignment for
    /// efficient SIMD operations.
    class TickCompressionContextV1 {
    public:
        std::vector<uint32_t, dfh::utils::aligned_allocator<uint32_t, 16>> deltas_u32; ///< Stores 32-bit delta values.
        std::vector<uint64_t, dfh::utils::aligned_allocator<uint64_t, 16>> deltas_u64; ///< Stores 64-bit delta values.
        std::vector<uint32_t, dfh::utils::aligned_allocator<uint32_t, 16>> values_u32; ///< Stores unique 32-bit values extracted during frequency encoding.
        std::vector<uint64_t, dfh::utils::aligned_allocator<uint64_t, 16>> values_u64; ///< Stores unique 64-bit values extracted during frequency encoding.
        std::vector<uint32_t, dfh::utils::aligned_allocator<uint32_t, 16>> rle_u32;    ///< Stores 32-bit run-length encoded (RLE) values. For flagged Run-Length Encoding (Flagged RLE)
        std::vector<uint32_t, dfh::utils::aligned_allocator<uint32_t, 16>> code_to_value_u32; ///< Maps encoded 32-bit values to their original values.
        std::vector<uint64_t, dfh::utils::aligned_allocator<uint64_t, 16>> code_to_value_u64; ///< Maps encoded 64-bit values to their original values.
        std::vector<uint32_t, dfh::utils::aligned_allocator<uint32_t, 16>> index_map_u32;     ///< Stores index mappings for 32-bit frequency-encoded values.
        std::vector<uint8_t> processing_buffer; ///< General-purpose buffer for processing intermediate data.

        TickCompressionContextV1() = default;

        /// \brief Resets all buffers in the context.
        /// Clears all data stored in the buffers, effectively resetting the state of the context.
        /// Use this method to prepare the context for a new compression or decompression operation.
        void reset() {
            deltas_u32.clear();
            deltas_u64.clear();
            values_u32.clear();
            values_u64.clear();
            rle_u32.clear();
            code_to_value_u32.clear();
            code_to_value_u64.clear();
            index_map_u32.clear();
            processing_buffer.clear();
        }
    };

}; // namespace dfh::compression

#endif // _DFH_COMPRESSION_TICK_COMPRESSOR_V1_TICK_COMPRESSION_CONTEXT_V1_HPP_INCLUDED
