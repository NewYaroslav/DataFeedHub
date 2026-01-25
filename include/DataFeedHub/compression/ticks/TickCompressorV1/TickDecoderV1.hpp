#pragma once
#ifndef _DFH_COMPRESSION_TICK_COMPRESSOR_V1_TICK_DECODER_V1_HPP_INCLUDED
#define _DFH_COMPRESSION_TICK_COMPRESSOR_V1_TICK_DECODER_V1_HPP_INCLUDED

/// \file TickDecoderV1.hpp
/// \brief Defines the decoder for tick data compression in the TickCompressorV1 system.

#include "DataFeedHub/compression/utils/repeat_encoding.hpp"
#include "DataFeedHub/utils/simdcomp.hpp"
#include "DataFeedHub/utils/vbyte.hpp"
#include <stdexcept>

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
        /// \tparam TickType Tick structure type.
        /// \tparam PriceMember Pointer-to-member selecting the price field (double TickType::*).
        /// \param ticks Array to store decompressed tick data.
        /// \param binary Binary data buffer containing compressed data.
        /// \param offset Current offset in the binary buffer, updated after decoding.
        /// \param num_ticks Number of ticks to decode.
        /// \param price_scale Scaling factor for price precision.
        /// \param initial_price Initial price used for delta calculations.
        template<class TickType, double TickType::* PriceMember>
        void decode_price(
                TickType* ticks,
                const std::uint8_t* binary,
                std::size_t& offset,
                std::size_t num_ticks,
                double price_scale,
                std::int64_t initial_price) {
            auto &deltas_u32 = m_context.deltas_u32;
            auto &deltas_u64= m_context.deltas_u64;
            auto &dict_values_u32 = m_context.values_u32;
            auto &dict_values_u64 = m_context.values_u64;
            auto &rle_u32 = m_context.rle_u32;

            std::uint32_t dict_length = dfh::utils::extract_vbyte<std::uint32_t>(binary, offset);
            bool requires_int64 = static_cast<bool>(dict_length & 0x1);
            dict_length >>= 1;

            if (requires_int64) {
                dict_values_u64.resize(dict_length);
                dfh::utils::extract_vbyte(binary, offset, dict_values_u64.data(), dict_length);

                deltas_u32.resize(num_ticks);
                std::size_t deltas_size = dfh::utils::extract_vbyte<std::uint32_t>(binary, offset);
                dfh::utils::extract_simdcomp(binary, offset, deltas_u32.data(), deltas_size);

                decode_delta_zig_zag_u64(dict_values_u64.data(), dict_values_u64.data(), dict_length, 0);

                rle_u32.resize(num_ticks);
                std::size_t repeats_size = 0;
                decode_zero_with_repeats(deltas_u32.data(), deltas_size, rle_u32.data(), repeats_size);

                deltas_u64.resize(num_ticks);
                decode_frequency(rle_u32.data(), deltas_u64.data(), num_ticks, dict_values_u64.data());
                decode_price_delta_zig_zag_u64<TickType, PriceMember>(deltas_u64.data(), ticks, num_ticks, price_scale, initial_price);
            } else {
                dict_values_u32.resize(dict_length);
                dfh::utils::extract_simdcomp(binary, offset, dict_values_u32.data(), dict_length);

                std::size_t deltas_size = dfh::utils::extract_vbyte<std::uint32_t>(binary, offset);
                deltas_u32.resize(num_ticks);
                dfh::utils::extract_simdcomp(binary, offset, deltas_u32.data(), deltas_size);

                decode_delta_zig_zag_u32(dict_values_u32.data(), dict_values_u32.data(), dict_length, 0);

                rle_u32.resize(num_ticks);
                std::size_t repeats_size = 0;
                decode_zero_with_repeats(deltas_u32.data(), deltas_size, rle_u32.data(), repeats_size);

                decode_frequency(rle_u32.data(), rle_u32.data(), num_ticks, dict_values_u32.data());
                decode_price_delta_zig_zag_u32(rle_u32.data(), ticks, num_ticks, price_scale, initial_price);
            }
        }

        /// \brief Decodes the compressed volume data.
        /// \tparam TickType Tick structure type.
        /// \param ticks The array to store decompressed tick data.
        /// \param binary The binary data buffer containing compressed data.
        /// \param offset The current offset in the binary buffer, updated after decoding.
        /// \param num_ticks The number of ticks to decode.
        /// \param volume_scale The scaling factor for volume precision.
        template<class TickType>
        void decode_volume(
                TickType* ticks,
                const std::uint8_t* binary,
                std::size_t& offset,
                std::size_t num_ticks,
                double volume_scale) {
            auto &deltas_u32 = m_context.deltas_u32;
            auto &deltas_u64= m_context.deltas_u64;
            auto &dict_values_u32 = m_context.values_u32;
            auto &dict_values_u64 = m_context.values_u64;
            auto &rle_u32 = m_context.rle_u32;

            std::size_t dict_length = dfh::utils::extract_vbyte<std::uint32_t>(binary, offset);
            bool requires_int64 = static_cast<bool>(dict_length & 0x1);
            dict_length >>= 1;

            if (requires_int64) {
                dict_values_u64.resize(dict_length);
                dfh::utils::extract_vbyte(binary, offset, dict_values_u64.data(), dict_length);

                deltas_u32.resize(num_ticks);
                std::size_t deltas_size = dfh::utils::extract_vbyte<std::uint32_t>(binary, offset);
                dfh::utils::extract_vbyte(binary, offset, deltas_u32.data(), deltas_size);

                decode_delta_zig_zag_u64(dict_values_u64.data(), dict_values_u64.data(), dict_length, 0);

                rle_u32.resize(num_ticks);
                decode_zero_with_repeats(deltas_u32.data(), deltas_size, rle_u32.data());

                deltas_u64.resize(num_ticks);
                decode_frequency(rle_u32.data(), deltas_u64.data(), num_ticks, dict_values_u64.data());
                unscale_volume<TickType>(deltas_u64.data(), ticks, num_ticks, volume_scale);
            } else {
                dict_values_u32.resize(dict_length);
                dfh::utils::extract_simdcomp(binary, offset, dict_values_u32.data(), dict_length);

                deltas_u32.resize(num_ticks);
                std::size_t deltas_size = dfh::utils::extract_vbyte<std::uint32_t>(binary, offset);
                dfh::utils::extract_vbyte(binary, offset, deltas_u32.data(), deltas_size);

                decode_delta_zig_zag_u32(dict_values_u32.data(), dict_values_u32.data(), dict_length, 0);

                rle_u32.resize(num_ticks);
                decode_zero_with_repeats(deltas_u32.data(), deltas_size, rle_u32.data());

                decode_frequency(rle_u32.data(), rle_u32.data(), num_ticks, dict_values_u32.data());
                unscale_volume<TickType>(rle_u32.data(), ticks, num_ticks, volume_scale);
            }
        }

        /// \brief Decodes the compressed timestamp data.
        /// \tparam TickType Tick structure type.
        /// \param ticks The array to store decompressed tick data.
        /// \param binary The binary data buffer containing compressed data.
        /// \param offset The current offset in the binary buffer, updated after decoding.
        /// \param num_ticks The number of ticks to decode.
        /// \param base_time The base time used for delta calculations.
        template<class TickType>
        void decode_time(
                TickType* ticks,
                const std::uint8_t* binary,
                std::size_t& offset,
                std::size_t num_ticks,
                std::uint64_t base_time) {
            auto &deltas_u32 = m_context.deltas_u32;
            auto &dict_values_u32 = m_context.values_u32;
            auto &rle_u32 = m_context.rle_u32;

            std::size_t values_length = dfh::utils::extract_vbyte<std::uint32_t>(binary, offset);
            dict_values_u32.resize(values_length);
            dfh::utils::extract_simdcomp(binary, offset, dict_values_u32.data(), values_length);

            deltas_u32.resize(num_ticks);
            std::size_t deltas_size = dfh::utils::extract_vbyte<std::uint32_t>(binary, offset);
            dfh::utils::extract_vbyte(binary, offset, deltas_u32.data(), deltas_size);

            decode_delta_zig_zag_u32(dict_values_u32.data(), dict_values_u32.data(), values_length, 0);

            rle_u32.resize(num_ticks);
            decode_zero_with_repeats(deltas_u32.data(), deltas_size, rle_u32.data());

            decode_frequency(rle_u32.data(), rle_u32.data(), num_ticks, dict_values_u32.data());
            decode_time_delta<TickType>(rle_u32.data(), ticks, num_ticks, base_time);
        }

        /// \brief Декодирует trade_id: simdcomp -> RLE нулей -> zig-zag -> +1 -> накопление.
        /// \param binary Двоичный буфер со сжатыми данными.
        /// \param offset Текущий оффсет в буфере; обновляется при чтении.
        /// \param num_ticks Ожидаемое число идентификаторов на выходе.
        /// \param output Вектор для результата (может быть nullptr).
        /// \throws std::runtime_error Если количество восстановленных значений не совпадает с num_ticks.
        void decode_trade_id(
                const std::uint8_t* binary,
                std::size_t& offset,
                std::size_t num_ticks,
                std::vector<std::uint64_t>* output) {
            const std::size_t encoded_size = dfh::utils::extract_vbyte<std::uint32_t>(binary, offset);
            auto &deltas_u32 = m_context.deltas_u32;
            auto &rle_u32 = m_context.rle_u32;

            deltas_u32.resize(encoded_size);
            dfh::utils::extract_simdcomp(binary, offset, deltas_u32.data(), encoded_size);

            if (!output) {
                return;
            }

            rle_u32.resize(num_ticks);
            std::size_t decoded_size = 0;
            decode_zero_with_repeats(deltas_u32.data(), encoded_size, rle_u32.data(), decoded_size);
            if (decoded_size != num_ticks) {
                throw std::runtime_error("decode_trade_id: decoded size does not match tick count");
            }

            output->resize(num_ticks);
            std::int64_t prev = 0;
            for (std::size_t i = 0; i < num_ticks; ++i) {
                const std::uint32_t zigzag = rle_u32[i];
                const std::int32_t delta_adj = static_cast<std::int32_t>((zigzag >> 1) ^ -(zigzag & 1));
                const std::int64_t delta = static_cast<std::int64_t>(delta_adj) + 1;
                const std::int64_t current = prev + delta;
                (*output)[i] = static_cast<std::uint64_t>(current);
                prev = current;
            }
        }

        /// \brief Decodes the compressed side flags indicating trade direction.
        /// \param ticks The array to store decompressed tick data.
        /// \param binary The binary data buffer containing compressed data.
        /// \param offset The current offset in the binary buffer, updated after decoding.
        /// \param num_ticks The number of ticks to decode.
        void decode_side_flags(
                MarketTick* ticks,
                const std::uint8_t* binary,
                std::size_t& offset,
                std::size_t num_ticks) {
            constexpr std::size_t chunk_width = sizeof(std::uint8_t);
            constexpr std::size_t bit_flag_buy  = 4;
            constexpr std::size_t bit_flag_sell = 5;
            constexpr std::uint64_t flag_mask = ~(
                static_cast<std::uint64_t>(TickUpdateFlags::TICK_FROM_BUY) |
                static_cast<std::uint64_t>(TickUpdateFlags::TICK_FROM_SELL));
            const std::size_t aligned_size = num_ticks - (num_ticks % chunk_width);

            std::uint64_t byte, value;
            std::size_t max_j, byte_index = offset;
            for (std::size_t i = 0; i < aligned_size; i += chunk_width, ++byte_index) {
                max_j = i + chunk_width;
                byte = binary[byte_index];
                value = (byte & 0x1);
                ticks[i].flags &= flag_mask;
                ticks[i].flags |= value << bit_flag_buy;
                ticks[i].flags |= !value << bit_flag_sell;
                for (std::size_t j = i + 1; j < max_j; ++j) {
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
                for (std::size_t i = aligned_size + 1; i < num_ticks; ++i) {
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
