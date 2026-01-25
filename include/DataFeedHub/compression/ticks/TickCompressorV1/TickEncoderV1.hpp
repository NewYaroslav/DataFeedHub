#pragma once
#ifndef _DFH_COMPRESSION_TICK_COMPRESSOR_V1_TICK_ENCODER_V1_HPP_INCLUDED
#define _DFH_COMPRESSION_TICK_COMPRESSOR_V1_TICK_ENCODER_V1_HPP_INCLUDED

/// \file TickEncoderV1.hpp
/// \brief Defines the encoder for tick data compression in the TickCompressorV1 system.

#include "DataFeedHub/compression/utils/repeat_encoding.hpp"
#include "DataFeedHub/utils/simdcomp.hpp"
#include "DataFeedHub/utils/vbyte.hpp"
#include <limits>
#include <stdexcept>

namespace dfh::compression {

    /// \class TickEncoderV1
    /// \brief Encodes tick data for compression.
    class TickEncoderV1 {
    public:

        /// \brief Constructs a TickEncoderV1 with a given compression context.
        /// \param context The compression context used for intermediate data.
        explicit TickEncoderV1(TickCompressionContextV1& context)
            : m_context(context) {}

        /// \brief Encodes the last trade price as delta values.
        /// \param output Buffer where encoded data will be written.
        /// \param ticks Array of market ticks to encode.
        /// \param num_ticks Number of ticks to encode.
        /// \param price_scale Scaling factor for price precision.
        /// \param initial_price Initial price for delta calculations.
		/// \param expected_uniques Expected number of unique values (used for reserve()).
		template<class TickType, double TickType::* PriceMember>
        void encode_price(
                std::vector<uint8_t>& output,
                const TickType* ticks,
                size_t num_ticks,
                double price_scale,
                int64_t initial_price,
				std::size_t expected_uniques = 16384) {
            auto &deltas_u32 = m_context.deltas_u32;
            auto &deltas_u64 = m_context.deltas_u64;
            auto &dict_values_u32 = m_context.values_u32;
            auto &dict_values_u64 = m_context.values_u64;

			deltas_u32.resize(num_ticks);
            if (encode_price_delta_zig_zag_u32<TickType, PriceMember>(ticks, deltas_u32.data(), num_ticks, price_scale, initial_price)) {
                encode_frequency(deltas_u32.data(), deltas_u32.data(), num_ticks, dict_values_u32, expected_uniques);

                size_t repeats_size = 0;
                encode_zero_with_repeats(deltas_u32.data(), deltas_u32.size(), deltas_u32.data(), repeats_size);
                deltas_u32.resize(repeats_size);

                encode_delta_zig_zag_u32(dict_values_u32.data(), dict_values_u32.data(), dict_values_u32.size(), 0);

                uint32_t dict_length = dict_values_u32.size();
                dict_length = (dict_length << 1);

                dfh::utils::append_vbyte<uint32_t>(output, dict_length);
                dfh::utils::append_simdcomp(output, dict_values_u32.data(), dict_values_u32.size());

                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.size());
                dfh::utils::append_simdcomp(output, deltas_u32.data(), deltas_u32.size());
            } else {
                // Обработка ошибки encode_last_delta_zig_zag_u32
                deltas_u64.resize(num_ticks);
                encode_price_delta_zig_zag_u64<TickType, PriceMember>(ticks, deltas_u64.data(), num_ticks, price_scale, initial_price);
                encode_frequency(deltas_u64.data(), deltas_u32.data(), num_ticks, dict_values_u64, expected_uniques);

                size_t repeats_size = 0;
                encode_zero_with_repeats(deltas_u32.data(), deltas_u32.size(), deltas_u32.data(), repeats_size);
                deltas_u32.resize(repeats_size);

                encode_delta_zig_zag_u64(dict_values_u64.data(), dict_values_u64.data(), dict_values_u64.size(), 0);

                uint32_t dict_length = dict_values_u64.size();
                dict_length = (dict_length << 1) | 0x1;

                dfh::utils::append_vbyte<uint32_t>(output, dict_length);
                dfh::utils::append_vbyte<uint64_t>(output, dict_values_u64.data(), dict_values_u64.size());

                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.size());
                dfh::utils::append_simdcomp(output, deltas_u32.data(), deltas_u32.size());
            }
        }

        /// \brief Encodes the trade volume as delta values.
		
        /// \param output Buffer where encoded data will be written.
        /// \param ticks Array of market ticks to encode.
        /// \param num_ticks Number of ticks to encode.
        /// \param volume_scale Scaling factor for volume precision.
		/// \param expected_uniques Expected number of unique values (used for reserve()).
		template<class TickType>
        void encode_volume(
                std::vector<uint8_t>& output,
                const TickType* ticks,
                size_t num_ticks,
                double volume_scale,
				std::size_t expected_uniques = 16384) {
            auto &deltas_u32 = m_context.deltas_u32;
            auto &deltas_u64= m_context.deltas_u64;
            auto &dict_values_u32 = m_context.values_u32;
            auto &dict_values_u64 = m_context.values_u64;
			
			deltas_u32.resize(num_ticks);
			if (scale_volume_int32<TickType>(ticks, deltas_u32.data(), num_ticks, volume_scale)) {
                encode_frequency(deltas_u32.data(), deltas_u32.data(), num_ticks, dict_values_u32, expected_uniques);

                size_t repeats_size = 0;
                encode_zero_with_repeats(deltas_u32.data(), deltas_u32.size(), deltas_u32.data(), repeats_size);
                deltas_u32.resize(repeats_size);

                encode_delta_zig_zag_int32(dict_values_u32.data(), dict_values_u32.data(), dict_values_u32.size(), 0);

                uint32_t dict_length = dict_values_u32.size();
                dict_length = (dict_length << 1);

                dfh::utils::append_vbyte<uint32_t>(output, dict_length);
                dfh::utils::append_simdcomp(output, dict_values_u32.data(), dict_values_u32.size());
                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.size());
                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.data(), deltas_u32.size());
            } else {
                // Обработка ошибки scale_volume_int32, encode_frequency
                deltas_u64.resize(num_ticks);

                scale_volume_int64<TickType>(ticks, deltas_u64.data(), num_ticks, volume_scale);
                encode_frequency(deltas_u64.data(), deltas_u32.data(), num_ticks, dict_values_u64, expected_uniques);

                size_t repeats_size = 0;
                encode_zero_with_repeats(deltas_u32.data(), deltas_u32.size(), deltas_u32.data(), repeats_size);
                deltas_u32.resize(repeats_size);

                encode_delta_zig_zag_int64(dict_values_u64.data(), dict_values_u64.data(), dict_values_u64.size(), 0);

                uint32_t dict_length = dict_values_u64.size();
                dict_length = (dict_length << 1) | 0x1;

                dfh::utils::append_vbyte<uint32_t>(output, dict_length);
                dfh::utils::append_vbyte<uint64_t>(output, dict_values_u64.data(), dict_values_u64.size());
                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.size());
                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.data(), deltas_u32.size());
            }
        }

        /// \brief Encodes the timestamp as delta values.
        /// \param output Buffer where encoded data will be written.
        /// \param ticks Array of market ticks to encode.
        /// \param num_ticks Number of ticks to encode.
        /// \param initial_time Initial timestamp for delta calculations.
		/// \param expected_uniques Expected number of unique values (used for reserve()).
		template<class TickType>
        void encode_time(
                std::vector<std::uint8_t>& output,
                const TickType* ticks,
                std::size_t num_ticks,
                std::int64_t initial_time,
				std::size_t expected_uniques = 16384) {
            auto &deltas_u32 = m_context.deltas_u32;
            auto &dict_values_u32 = m_context.values_u32;
 
            deltas_u32.resize(num_ticks);
            encode_time_delta<TickType>(ticks, deltas_u32.data(), num_ticks, initial_time);
            encode_frequency(deltas_u32.data(), deltas_u32.data(), num_ticks, dict_values_u32, expected_uniques);

            size_t repeats_size = 0;
            encode_zero_with_repeats(deltas_u32.data(), deltas_u32.size(), deltas_u32.data(), repeats_size);
            deltas_u32.resize(repeats_size);

            encode_delta_zig_zag_u32(dict_values_u32.data(), dict_values_u32.data(), dict_values_u32.size(), 0);

            dfh::utils::append_vbyte<uint32_t>(output, dict_values_u32.size());
            dfh::utils::append_simdcomp(output, dict_values_u32.data(), dict_values_u32.size());

            dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.size());
            dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.data(), deltas_u32.size());
        }

        /// \brief Кодирует trade_id: delta-1 -> zig-zag -> RLE нулей -> simdcomp.
        /// \param output Буфер для записи закодированных данных.
        /// \param trade_ids Массив идентификаторов сделок.
        /// \throws std::overflow_error Если delta_adj выходит за пределы int32_t.
		template<class TickType>
        void encode_trade_id(
                std::vector<uint8_t>& output,
				const TickType* ticks,
				std::size_t num_ticks,
				std::int64_t initial_id) {
            

            auto &deltas_u32 = m_context.deltas_u32;
            const size_t count = trade_ids.size();
            deltas_u32.resize(count);

            constexpr int64_t min_val = static_cast<int64_t>(std::numeric_limits<int32_t>::min());
            constexpr int64_t max_val = static_cast<int64_t>(std::numeric_limits<int32_t>::max());

            int64_t prev = 0;
            for (size_t i = 0; i < count; ++i) {
                const int64_t current = static_cast<int64_t>(trade_ids[i]);
                const int64_t delta = current - prev;
                const int64_t delta_adj = delta - 1;
                if (delta_adj < min_val || delta_adj > max_val) {
                    throw std::overflow_error("encode_trade_id: delta_adj out of int32 range");
                }
                const int32_t delta_adj32 = static_cast<int32_t>(delta_adj);
                deltas_u32[i] = static_cast<uint32_t>((delta_adj32 << 1) ^ (delta_adj32 >> 31));
                prev = current;
            }

            size_t repeats_size = 0;
            encode_zero_with_repeats(deltas_u32.data(), deltas_u32.size(), deltas_u32.data(), repeats_size);
            deltas_u32.resize(repeats_size);

            dfh::utils::append_vbyte<uint32_t>(output, static_cast<uint32_t>(repeats_size));
            dfh::utils::append_simdcomp(output, deltas_u32.data(), deltas_u32.size());
        }

        /// \brief Encodes the side flags indicating the direction of the trade.
        /// \param output Buffer where encoded data will be written.
        /// \param ticks Array of market ticks to encode.
        /// \param num_ticks Number of ticks to encode.
        void encode_side_flags(
                std::vector<uint8_t>& output,
                const MarketTick* ticks,
                size_t num_ticks) {
            constexpr size_t chunk_width = sizeof(uint8_t);
            constexpr size_t bif_offset = 4;
            const size_t start_offset = output.size();
            const size_t aligned_size = num_ticks - (num_ticks % chunk_width);

            output.resize(start_offset + ((num_ticks + (chunk_width - 1)) / chunk_width));

            size_t max_j, byte_index = start_offset;
            for (size_t i = 0; i < aligned_size; i += chunk_width, ++byte_index) {
                max_j = i + chunk_width;
                output[byte_index] = (static_cast<uint64_t>(ticks[i].flags) >> bif_offset) & 0x1;
                for (size_t j = i + 1; j < max_j; ++j) {
                    output[byte_index] <<= 1;
                    output[byte_index] |= (static_cast<uint64_t>(ticks[j].flags) >> bif_offset) & 0x1;
                }
            }

            for (size_t bit_index = 0, i = aligned_size; i < num_ticks; ++i, ++bit_index) {
                output[byte_index] |= ((static_cast<uint64_t>(ticks[i].flags) >> bif_offset) & 0x1) << bit_index;
            }
        }

    private:
        TickCompressionContextV1& m_context; ///< Reference to the compression context for intermediate data.
    };

}; // namespace dfh::compression

#endif // _DFH_COMPRESSION_TICK_COMPRESSOR_V1_TICK_ENCODER_V1_HPP_INCLUDED
