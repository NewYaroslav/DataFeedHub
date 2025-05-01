#pragma once
#ifndef _DFH_TRANSFORM_CROP_BY_TIME_HPP_INCLUDED
#define _DFH_TRANSFORM_CROP_BY_TIME_HPP_INCLUDED

/// \file crop_by_time.hpp
/// \brief Generic utilities for cropping vectors of objects by time ranges.

#include <algorithm>
#include <vector>

namespace dfh::transform {

    /// \brief Removes elements with time_ms less than start_time_ms (inclusive crop start).
    /// \tparam T Type of the elements (must have `time_ms` field).
    /// \param items Vector of items to crop.
    /// \param start_time_ms Inclusive lower time bound (milliseconds).
    template<typename T>
    inline void crop_before(std::vector<T>& items, uint64_t start_time_ms) {
        items.erase(
            std::remove_if(items.begin(), items.end(), [start_time_ms](const T& item) {
                return item.time_ms < start_time_ms;
            }),
            items.end()
        );
    }

    /// \brief Removes elements with time_ms greater than or equal to end_time_ms (exclusive crop end).
    /// \tparam T Type of the elements (must have `time_ms` field).
    /// \param items Vector of items to crop.
    /// \param end_time_ms Exclusive upper time bound (milliseconds).
    template<typename T>
    inline void crop_after(std::vector<T>& items, uint64_t end_time_ms) {
        items.erase(
            std::remove_if(items.begin(), items.end(), [end_time_ms](const T& item) {
                return item.time_ms >= end_time_ms;
            }),
            items.end()
        );
    }

} // namespace dfh::transform

#endif // _DFH_TRANSFORM_CROP_BY_TIME_HPP_INCLUDED