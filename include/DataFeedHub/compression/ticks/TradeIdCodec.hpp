#pragma once
#ifndef _DFH_COMPRESSION_TICK_TRADE_ID_CODEC_HPP_INCLUDED
#define _DFH_COMPRESSION_TICK_TRADE_ID_CODEC_HPP_INCLUDED

/// \file TradeIdCodec.hpp
/// \brief Прокси для существующего tick-трейд-кодека.

#include "DataFeedHub/compression/ticks/TickCompressorV1/TickCompressionContextV1.hpp"
#include "DataFeedHub/compression/ticks/TickCompressorV1/TickDecoderV1.hpp"
#include "DataFeedHub/compression/ticks/TickCompressorV1/TickEncoderV1.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dfh::compression {

    /// \brief Проксирует передачу trade_id в TickEncoderV1.
    /// \details Записывает encoded_count (uint32) и SIMD-сжатые значения.
    inline void encode_trade_id_deltas(std::vector<uint8_t>& buffer, const std::vector<uint64_t>& trade_ids) {
        if (trade_ids.empty()) return;
        TickCompressionContextV1 context;
        TickEncoderV1 encoder(context);
        encoder.encode_trade_id(buffer, trade_ids);
    }

    /// \brief Проксирует восстановление trade_id через TickDecoderV1.
    /// \details Читает encoded_count (uint32) и двигает offset независимо от output.
    inline void decode_trade_id_deltas(const uint8_t* data, size_t& offset, size_t count, std::vector<uint64_t>* output) {
        TickCompressionContextV1 context;
        TickDecoderV1 decoder(context);
        decoder.decode_trade_id(data, offset, count, output);
    }

} // namespace dfh::compression

#endif // _DFH_COMPRESSION_TICK_TRADE_ID_CODEC_HPP_INCLUDED
