#pragma once
#ifndef _DFH_COMPRESSION_TICK_TRADE_ID_CODEC_HPP_INCLUDED
#define _DFH_COMPRESSION_TICK_TRADE_ID_CODEC_HPP_INCLUDED

/// \file TradeIdCodec.hpp
/// \brief Encodes and decodes delta-encoded trade identifiers.

#include "DataFeedHub/utils/vbyte.hpp"
#include "DataFeedHub/compression/utils/zig_zag_delta.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dfh::compression {

    /// \brief Кодирует trade_id как последовательность дельт с zig-zag и vbyte.
    /// \details Формат: для каждого trade_id вычисляется signed delta от предыдущего
    /// (первый от 0), затем применяется zig-zag для int64 и vbyte для uint64.
    /// Предположения: порядок trade_id сохраняется; поддерживаются нули и повторы
    /// (дельта может быть 0 или отрицательной).
    inline void encode_trade_id_deltas(std::vector<uint8_t>& buffer, const std::vector<uint64_t>& trade_ids) {
        if (trade_ids.empty()) return;

        std::vector<int64_t> deltas;
        deltas.reserve(trade_ids.size());
        int64_t prev = 0;
        for (std::size_t i = 0; i < trade_ids.size(); ++i) {
            const int64_t current = static_cast<int64_t>(trade_ids[i]);
            deltas.push_back(current - prev);
            prev = current;
        }

        std::vector<uint64_t> zigzag(trade_ids.size());
        encode_delta_zig_zag_int64(deltas.data(), zigzag.data(), zigzag.size(), 0);
        dfh::utils::append_vbyte<uint64_t>(buffer, zigzag.data(), zigzag.size());
    }

    /// \brief Декодирует trade_id из vbyte и восстанавливает дельты zig-zag.
    /// \details Считывает ровно count значений vbyte и смещает offset независимо
    /// от output. Если output == nullptr, значения просто пропускаются.
    inline void decode_trade_id_deltas(const uint8_t* data, size_t& offset, size_t count, std::vector<uint64_t>* output) {
        if (count == 0) return;

        std::vector<uint64_t> zigzag;
        zigzag.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            zigzag.push_back(dfh::utils::extract_vbyte<uint64_t>(data, offset));
        }

        if (!output) return;
        std::vector<int64_t> decoded(count);
        decode_delta_zig_zag_int64(zigzag.data(), decoded.data(), count, 0);
        output->resize(count);
        for (size_t i = 0; i < count; ++i) {
            (*output)[i] = static_cast<uint64_t>(decoded[i]);
        }
    }

} // namespace dfh::compression

#endif // _DFH_COMPRESSION_TICK_TRADE_ID_CODEC_HPP_INCLUDED
