#pragma once
#ifndef _DFH_COMPRESSION_TICK_COMPRESSOR_V1_TICK_DECODER_V1_HPP_INCLUDED
#define _DFH_COMPRESSION_TICK_COMPRESSOR_V1_TICK_DECODER_V1_HPP_INCLUDED

/// \file TickDecoderV1.hpp
/// \brief Defines the decoder for tick data compression in the TickCompressorV1 system.

namespace dfh::compression {

    /// \class TickDecoderV1
    /// \brief Decodes compressed tick data.
    class TickDecoderV1 {
    public:
        /// \brief Constructs a TickDecoderV1 with a given compression context.
        /// \param context The compression context used for intermediate data during decoding.
        explicit TickDecoderV1(TickCompressionContextV1& context)
            : m_context(context) {}

        /// \brief Decodes the compressed price data.
        /// \param ticks The array to store decompressed tick data.
        /// \param binary The binary data buffer containing compressed data.
        /// \param offset The current offset in the binary buffer, updated after decoding.
        /// \param num_ticks The number of ticks to decode.
        /// \param price_scale The scaling factor for price precision.
        /// \param initial_price The initial price used for delta calculations.
        void decode_price_last(
                MarketTick* ticks,
                const uint8_t* binary,
                size_t& offset,
                size_t num_ticks,
                double price_scale,
                int64_t initial_price) {
            auto &deltas_u32 = m_context.deltas_u32;
            auto &deltas_u64= m_context.deltas_u64;
            auto &values_u32 = m_context.values_u32;
            auto &values_u64 = m_context.values_u64;
            auto &rle_u32 = m_context.rle_u32;
            auto &code_to_value_u32 = m_context.code_to_value_u32;
            auto &code_to_value_u64 = m_context.code_to_value_u64;
            auto &index_map_u32 = m_context.index_map_u32;

            uint32_t values_length = dfh::utils::extract_vbyte<uint32_t>(binary, offset);
            bool requires_int64 = static_cast<bool>(values_length & 0x1);
            values_length >>= 1;

            if (requires_int64) {
                values_u64.resize(values_length);
                index_map_u32.resize(values_length);
                dfh::utils::extract_vbyte(binary, offset, values_u64.data(), values_length);
                dfh::utils::extract_simdcomp(binary, offset, index_map_u32.data(), values_length);

                size_t deltas_size = dfh::utils::extract_vbyte<uint32_t>(binary, offset);
                deltas_u32.resize(num_ticks);
                dfh::utils::extract_simdcomp(binary, offset, deltas_u32.data(), deltas_size);

                decode_delta_sorted<uint64_t, uint64_t>(values_u64.data(), values_u64.data(), values_length, 0);
                decode_delta_zig_zag_int32(index_map_u32.data(), index_map_u32.data(), values_length, 0);

                rle_u32.resize(num_ticks);
                size_t repeats_size = 0;
                decode_zero_with_repeats(deltas_u32.data(), deltas_size, rle_u32.data(), repeats_size);

                deltas_u64.resize(num_ticks);
                code_to_value_u64.resize(values_length);
                decode_frequency(rle_u32.data(), deltas_u64.data(), num_ticks, code_to_value_u64.data(), values_u64.data(), index_map_u32.data(), values_length);
                decode_last_delta_zig_zag_int64(deltas_u64.data(), ticks, num_ticks, price_scale, initial_price);
            } else {
                values_u32.resize(values_length);
                index_map_u32.resize(values_length);
                dfh::utils::extract_simdcomp(binary, offset, values_u32.data(), values_length);
                dfh::utils::extract_simdcomp(binary, offset, index_map_u32.data(), values_length);

                size_t deltas_size = dfh::utils::extract_vbyte<uint32_t>(binary, offset);
                deltas_u32.resize(num_ticks);
                dfh::utils::extract_simdcomp(binary, offset, deltas_u32.data(), deltas_size);

                decode_delta_sorted<uint32_t, uint32_t>(values_u32.data(), values_u32.data(), values_length, 0);
                decode_delta_zig_zag_int32(index_map_u32.data(), index_map_u32.data(), values_length, 0);

                rle_u32.resize(num_ticks);
                size_t repeats_size = 0;
                decode_zero_with_repeats(deltas_u32.data(), deltas_size, rle_u32.data(), repeats_size);

                code_to_value_u32.resize(values_length);
                decode_frequency(rle_u32.data(), rle_u32.data(), num_ticks, code_to_value_u32.data(), values_u32.data(), index_map_u32.data(), values_length);
                decode_last_delta_zig_zag_int32(rle_u32.data(), ticks, num_ticks, price_scale, initial_price);
            }
        }

        /// \brief Decodes the compressed volume data.
        /// \param ticks The array to store decompressed tick data.
        /// \param binary The binary data buffer containing compressed data.
        /// \param offset The current offset in the binary buffer, updated after decoding.
        /// \param num_ticks The number of ticks to decode.
        /// \param volume_scale The scaling factor for volume precision.
        void decode_volume(
                MarketTick* ticks,
                const uint8_t* binary,
                size_t& offset,
                size_t num_ticks,
                double volume_scale) {
            auto &deltas_u32 = m_context.deltas_u32;
            auto &deltas_u64= m_context.deltas_u64;
            auto &values_u32 = m_context.values_u32;
            auto &values_u64 = m_context.values_u64;
            auto &rle_u32 = m_context.rle_u32;
            auto &code_to_value_u32 = m_context.code_to_value_u32;
            auto &code_to_value_u64 = m_context.code_to_value_u64;
            auto &index_map_u32 = m_context.index_map_u32;

            size_t values_length = dfh::utils::extract_vbyte<uint32_t>(binary, offset);
            bool requires_int64 = static_cast<bool>(values_length & 0x1);
            values_length >>= 1;

            index_map_u32.resize(values_length);

            if (requires_int64) {
                values_u64.resize(values_length);
                values_u32.resize(values_length);
                dfh::utils::extract_vbyte(binary, offset, values_u64.data(), values_length);
                dfh::utils::extract_simdcomp(binary, offset, index_map_u32.data(), values_length);

                size_t deltas_size = dfh::utils::extract_vbyte<uint32_t>(binary, offset);
                deltas_u32.resize(num_ticks);
                dfh::utils::extract_vbyte(binary, offset, deltas_u32.data(), deltas_size);

                decode_delta_sorted<uint64_t, uint64_t>(values_u64.data(), values_u64.data(), values_length, 0);
                decode_delta_zig_zag_int32(index_map_u32.data(), index_map_u32.data(), values_length, 0);

                rle_u32.resize(num_ticks);
                size_t repeats_size = 0;
                decode_zero_with_repeats(deltas_u32.data(), deltas_size, rle_u32.data(), repeats_size);

                deltas_u64.resize(num_ticks);
                code_to_value_u64.resize(values_length);
                decode_frequency(rle_u32.data(), deltas_u64.data(), num_ticks, code_to_value_u64.data(), values_u64.data(), index_map_u32.data(), values_length);
                scale_volume<uint64_t, MarketTick>(deltas_u64.data(), ticks, num_ticks, volume_scale);
            } else {
                values_u32.resize(values_length);
                dfh::utils::extract_simdcomp(binary, offset, values_u32.data(), values_length);
                dfh::utils::extract_simdcomp(binary, offset, index_map_u32.data(), values_length);

                deltas_u32.resize(num_ticks);
                size_t deltas_size = dfh::utils::extract_vbyte<uint32_t>(binary, offset);
                dfh::utils::extract_vbyte(binary, offset, deltas_u32.data(), deltas_size);

                decode_delta_sorted<uint32_t, uint32_t>(values_u32.data(), values_u32.data(), values_length, 0);
                decode_delta_zig_zag_int32(index_map_u32.data(), index_map_u32.data(), values_length, 0);

                rle_u32.resize(num_ticks);
                size_t repeats_size = 0;
                decode_zero_with_repeats(deltas_u32.data(), deltas_size, rle_u32.data(), repeats_size);

                code_to_value_u32.resize(values_length);
                decode_frequency(rle_u32.data(), rle_u32.data(), num_ticks, code_to_value_u32.data(), values_u32.data(), index_map_u32.data(), values_length);
                scale_volume<uint32_t, MarketTick>(rle_u32.data(), ticks, num_ticks, volume_scale);
            }
        }

        /// \brief Decodes the compressed timestamp data.
        /// \param ticks The array to store decompressed tick data.
        /// \param binary The binary data buffer containing compressed data.
        /// \param offset The current offset in the binary buffer, updated after decoding.
        /// \param num_ticks The number of ticks to decode.
        /// \param base_time The base time used for delta calculations.
        void decode_time(
                MarketTick* ticks,
                const uint8_t* binary,
                size_t& offset,
                size_t num_ticks,
                uint64_t base_time) {
            auto &deltas_u32 = m_context.deltas_u32;
            auto &values_u32 = m_context.values_u32;
            auto &rle_u32 = m_context.rle_u32;
            auto &code_to_value_u32 = m_context.code_to_value_u32;
            auto &index_map_u32 = m_context.index_map_u32;

            size_t values_length = dfh::utils::extract_vbyte<uint32_t>(binary, offset);
            values_u32.resize(values_length);
            index_map_u32.resize(values_length);
            dfh::utils::extract_simdcomp(binary, offset, values_u32.data(), values_length);
            dfh::utils::extract_simdcomp(binary, offset, index_map_u32.data(), values_length);

            deltas_u32.resize(num_ticks);
            size_t deltas_size = dfh::utils::extract_vbyte<uint32_t>(binary, offset);
            dfh::utils::extract_vbyte(binary, offset, deltas_u32.data(), deltas_size);

            decode_delta_sorted<uint32_t, uint32_t>(values_u32.data(), values_u32.data(), values_length, 0);
            decode_delta_zig_zag_int32(index_map_u32.data(), index_map_u32.data(), values_length, 0);

            rle_u32.resize(num_ticks);
            size_t repeats_size = 0;
            decode_zero_with_repeats(deltas_u32.data(), deltas_size, rle_u32.data(), repeats_size);

            code_to_value_u32.resize(values_length);
            decode_frequency(rle_u32.data(), rle_u32.data(), num_ticks, code_to_value_u32.data(), values_u32.data(), index_map_u32.data(), values_length);
            decode_time_delta(rle_u32.data(), ticks, num_ticks, base_time);
        }

        /// \brief Decodes the compressed side flags indicating trade direction.
        /// \param ticks The array to store decompressed tick data.
        /// \param binary The binary data buffer containing compressed data.
        /// \param offset The current offset in the binary buffer, updated after decoding.
        /// \param num_ticks The number of ticks to decode.
        void decode_side_flags(
                MarketTick* ticks,
                const uint8_t* binary,
                size_t& offset,
                size_t num_ticks) {
            constexpr size_t chunk_width = sizeof(uint8_t);
            constexpr size_t bit_flag_buy  = 4;
            constexpr size_t bit_flag_sell = 5;
            constexpr uint64_t flag_mask = ~(
                static_cast<uint64_t>(TickUpdateFlags::TICK_FROM_BUY) |
                static_cast<uint64_t>(TickUpdateFlags::TICK_FROM_SELL));
            const size_t aligned_size = num_ticks - (num_ticks % chunk_width);

            uint64_t byte, value;
            size_t max_j, byte_index = offset;
            for (size_t i = 0; i < aligned_size; i += chunk_width, ++byte_index) {
                max_j = i + chunk_width;
                byte = binary[byte_index];
                value = (byte & 0x1);
                ticks[i].flags &= flag_mask;
                ticks[i].flags |= value << bit_flag_buy;
                ticks[i].flags |= !value << bit_flag_sell;
                for (size_t j = i + 1; j < max_j; ++j) {
                    byte >>= 1;
                    value = (byte & 0x1);
                    ticks[j].flags &= flag_mask;
                    ticks[j].flags |= value << bit_flag_buy;
                    ticks[j].flags |= !value << bit_flag_sell;
                }
            }

            if (aligned_size < num_ticks) {
                byte = binary[byte_index];
                value = (byte & 0x1);
                ticks[aligned_size].flags &= flag_mask;
                ticks[aligned_size].flags |= value << bit_flag_buy;
                ticks[aligned_size].flags |= !value << bit_flag_sell;
                for (size_t i = aligned_size + 1; i < num_ticks; ++i) {
                    byte >>= 1;
                    value = (byte & 0x1);
                    ticks[i].flags &= flag_mask;
                    ticks[i].flags |= value << bit_flag_buy;
                    ticks[i].flags |= !value << bit_flag_sell;
                }
            }

            offset += (num_ticks + 7) / 8;
        }

    private:
        TickCompressionContextV1& m_context; ///< Reference to the compression context for intermediate data.

    }; // TickDecoderV1

}; // namespace dfh::compression

#endif // _DFH_COMPRESSION_TICK_COMPRESSOR_V1_TICK_DECODER_V1_HPP_INCLUDED
