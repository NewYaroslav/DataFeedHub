#pragma once
#ifndef _DFH_COMPRESSION_UTILS_VOLUME_SCALING_HPP_INCLUDED
#define _DFH_COMPRESSION_UTILS_VOLUME_SCALING_HPP_INCLUDED

/// \file volume_scaling.hpp
/// \brief

#include <cmath>

namespace dfh::compression {

    /// \brief
    ///
    /// \param ticks
    /// \param output
    /// \param size
    /// \param scale
    template<class TickType>
    void scale_volume_int32(
            const TickType* ticks,
            uint32_t* output,
            size_t size,
            double scale) {
        constexpr int64_t max_val = static_cast<int64_t>(std::numeric_limits<uint32_t>::max());
        int64_t volume;
        for (size_t i = 0; i < size; ++i) {
            volume = std::llround(ticks[i].volume * scale);
            if (volume > max_val) throw std::overflow_error("?");
            output[i] = static_cast<uint32_t>(volume);
        }
    }

    template<class TickType>
    void scale_volume_int64(
            const TickType* ticks,
            uint64_t* output,
            size_t size,
            double scale) {
        for (size_t i = 0; i < size; ++i) {
            output[i] = static_cast<uint64_t>(std::llround(ticks[i].volume * scale));
        }
    }

    template<class InputType, class TickType>
    void scale_volume(
            const InputType* input,
            TickType* ticks,
            size_t size,
            double scale) {
        const double inv_scale = 1.0 / scale;
        for (size_t i = 0; i < size; ++i) {
            ticks[i].volume = static_cast<double>(input[i]) * inv_scale;
        }
    }

}; // namespace dfh::compression

#endif // _DFH_COMPRESSION_UTILS_VOLUME_SCALING_HPP_INCLUDED
