#pragma once
#ifndef _DFH_COMPRESSION_TICK_TRADE_ID_CODEC_HPP_INCLUDED
#define _DFH_COMPRESSION_TICK_TRADE_ID_CODEC_HPP_INCLUDED

/// \file TradeIdCodec.hpp
/// \brief Encodes and decodes delta-encoded trade identifiers.

#include "../../utils/vbyte.hpp"
#include "../utils/zig_zag_delta.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace dfh::compression {

    /// \brief Encodes trade identifiers using zig-zag delta encoding.
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

    /// \brief Decodes zig-zag delta-encoded trade identifiers.
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
