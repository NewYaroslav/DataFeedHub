#pragma once
#ifndef _DTH_DATA_BARS_ENUMS_HPP_INCLUDED
#define _DTH_DATA_BARS_ENUMS_HPP_INCLUDED

/// \file enums.hpp
/// \brief Defines enums for timeframes and related data intervals.

namespace dfh {

    /// \enum TimeFrame
    /// \brief Enumerates standard timeframes for bar data, stored as seconds.
    enum class TimeFrame : uint32_t {
        UNKNOWN = 0,     ///< Unknown timeframe
        S1     = 1,      ///< 1 second
        S3     = 3,      ///< 3 seconds
        S5     = 5,      ///< 5 seconds
        S15    = 15,     ///< 15 seconds
        M1     = 60,     ///< 1 minute
        M5     = 300,    ///< 5 minutes
        M15    = 900,    ///< 15 minutes
        M30    = 1800,   ///< 30 minutes
        H1     = 3600,   ///< 1 hour
        H4     = 14400,  ///< 4 hours
        D1     = 86400,  ///< 1 day
        W1     = 604800, ///< 1 week (7 days)
        MN1    = 2592000 ///< 1 month (30 days)
    };

    /// \brief Converts a TimeFrame enum value to milliseconds.
    /// \param tf TimeFrame value to convert.
    /// \return Duration in milliseconds
    constexpr uint64_t to_ms(TimeFrame tf) {
        return static_cast<uint64_t>(tf) * 1000ULL;
    }

    /// \brief Converts number of seconds to TimeFrame enum.
    /// \param sec Timeframe duration in seconds.
    /// \return Corresponding TimeFrame value, or TimeFrame::UNKNOWN if not matched.
    inline dfh::TimeFrame to_timeframe(uint32_t sec) {
        switch (sec) {
            case 1:       return dfh::TimeFrame::S1;
            case 3:       return dfh::TimeFrame::S3;
            case 5:       return dfh::TimeFrame::S5;
            case 15:      return dfh::TimeFrame::S15;
            case 60:      return dfh::TimeFrame::M1;
            case 300:     return dfh::TimeFrame::M5;
            case 900:     return dfh::TimeFrame::M15;
            case 1800:    return dfh::TimeFrame::M30;
            case 3600:    return dfh::TimeFrame::H1;
            case 14400:   return dfh::TimeFrame::H4;
            case 86400:   return dfh::TimeFrame::D1;
            case 604800:  return dfh::TimeFrame::W1;
            case 2592000: return dfh::TimeFrame::MN1;
            default:      return dfh::TimeFrame::UNKNOWN;
        }
    }

    /// \brief Converts number of milliseconds to TimeFrame enum.
    /// \param ms Timeframe duration in milliseconds.
    /// \return Corresponding TimeFrame value, or TimeFrame::UNKNOWN if not matched.
    inline dfh::TimeFrame to_timeframe_ms(uint64_t ms) {
        return to_timeframe(static_cast<uint32_t>(ms / 1000));
    }

    /// \brief Returns the recommended segment duration in seconds for a given timeframe.
    /// \param tf Timeframe enum value.
    /// \return Segment duration in seconds.
    /// \throws std::invalid_argument If the timeframe is unknown or unsupported.
    constexpr uint64_t get_segment_duration_sec(TimeFrame tf) {
        switch (tf) {
            case TimeFrame::S1:
            case TimeFrame::S3:
            case TimeFrame::S5:
            case TimeFrame::S15:
                return 3600ULL; // 1 hour
            case TimeFrame::M1:
            case TimeFrame::M5:
            case TimeFrame::M15:
            case TimeFrame::M30:
            case TimeFrame::H1:
                return 86400ULL; // 1 day
            case TimeFrame::H4:
            case TimeFrame::D1:
                return 604800ULL; // 1 week
            default:
                throw std::invalid_argument("Unsupported or unknown timeframe.");
        }
    }

    /// \brief Returns the recommended segment duration in milliseconds for a given timeframe.
    /// \param tf Timeframe enum value.
    /// \return Segment duration in milliseconds.
    /// \throws std::invalid_argument If the timeframe is unknown or unsupported.
    constexpr uint64_t get_segment_duration_ms(TimeFrame tf) {
        switch (tf) {
            case TimeFrame::S1:
            case TimeFrame::S3:
            case TimeFrame::S5:
            case TimeFrame::S15:
                return 3600000ULL; // 1 hour
            case TimeFrame::M1:
            case TimeFrame::M5:
            case TimeFrame::M15:
            case TimeFrame::M30:
            case TimeFrame::H1:
                return 86400000ULL; // 1 day
            case TimeFrame::H4:
            case TimeFrame::D1:
                return 604800000ULL; // 1 week
            default:
                throw std::invalid_argument("Unsupported or unknown timeframe.");
        }
    }

    /// \brief Returns the recommended segment duration in seconds for a given bar interval in milliseconds.
    /// \param bar_interval_ms Duration of the bar in milliseconds.
    /// \return Recommended segment duration in seconds.
    /// \throws std::invalid_argument If the interval is unknown or unsupported.
    constexpr uint64_t get_segment_duration_sec(uint64_t bar_interval_ms) {
        switch (bar_interval_ms) {
            case 1000ULL:
            case 3000ULL:
            case 5000ULL:
            case 15000ULL:
                return 3600ULL; // 1 hour
            case 60000ULL:
            case 300000ULL:
            case 900000ULL:
            case 1800000ULL:
            case 3600000ULL:
                return 86400ULL; // 1 day
            case 14400000ULL:
            case 86400000ULL:
                return 604800ULL; // 1 week
            default:
                throw std::invalid_argument("Unsupported or unknown bar interval (ms) for segment duration in seconds.");
        }
    }

    /// \brief Returns the recommended segment duration in milliseconds for a given bar interval in milliseconds.
    /// \param bar_interval_ms Duration of the bar in milliseconds.
    /// \return Recommended segment duration in milliseconds.
    /// \throws std::invalid_argument If the interval is unknown or unsupported.
    constexpr uint64_t get_segment_duration_ms(uint64_t bar_interval_ms) {
        switch (bar_interval_ms) {
            case 1000ULL:
            case 3000ULL:
            case 5000ULL:
            case 15000ULL:
                return 3600000ULL; // 1 hour
            case 60000ULL:
            case 300000ULL:
            case 900000ULL:
            case 1800000ULL:
            case 3600000ULL:
                return 86400000ULL; // 1 day
            case 14400000ULL:
            case 86400000ULL:
                return 604800000ULL; // 1 week
            default:
                throw std::invalid_argument("Unsupported or unknown bar interval (ms) for segment duration in milliseconds.");
        }
    }

    /// \brief Checks if the given timeframe is supported for data segmentation.
    /// \param tf TimeFrame enum value.
    /// \return True if the timeframe is supported for segmentation; false otherwise.
    constexpr bool is_segmentable_timeframe(TimeFrame tf) {
        switch (tf) {
            case TimeFrame::S1:
            case TimeFrame::S3:
            case TimeFrame::S5:
            case TimeFrame::S15:
            case TimeFrame::M1:
            case TimeFrame::M5:
            case TimeFrame::M15:
            case TimeFrame::M30:
            case TimeFrame::H1:
            case TimeFrame::H4:
            case TimeFrame::D1:
                return true;
            default:
                return false;
        }
    }

    /// \brief Checks if the given bar interval in milliseconds is supported for segmentation.
    /// \param bar_interval_ms Bar interval duration in milliseconds.
    /// \return True if the interval is a known segmentable timeframe; false otherwise.
    constexpr bool is_segmentable_bar_interval_ms(uint64_t bar_interval_ms) {
        switch (bar_interval_ms) {
            case 1000ULL:
            case 3000ULL:
            case 5000ULL:
            case 15000ULL:
            case 60000ULL:
            case 300000ULL:
            case 900000ULL:
            case 1800000ULL:
            case 3600000ULL:
            case 14400000ULL:
            case 86400000ULL:
                return true;
            default:
                return false;
        }
    }

    /// \brief Returns the next lower segmentable timeframe relative to the given one.
    /// \param tf TimeFrame enum value.
    /// \return The next lower timeframe supported for segmentation, or UNKNOWN if none.
    constexpr TimeFrame get_lower_timeframe(TimeFrame tf) {
        switch (tf) {
            case TimeFrame::D1:   return TimeFrame::H4;
            case TimeFrame::H4:   return TimeFrame::H1;
            case TimeFrame::H1:   return TimeFrame::M30;
            case TimeFrame::M30:  return TimeFrame::M15;
            case TimeFrame::M15:  return TimeFrame::M5;
            case TimeFrame::M5:   return TimeFrame::M1;
            case TimeFrame::M1:   return TimeFrame::S15;
            case TimeFrame::S15:  return TimeFrame::S5;
            case TimeFrame::S5:   return TimeFrame::S3;
            case TimeFrame::S3:   return TimeFrame::S1;
            case TimeFrame::S1:   return TimeFrame::UNKNOWN;
            default:              return TimeFrame::UNKNOWN;
        }
    }

    /// \brief Returns the next lower segmentable interval (in milliseconds) relative to the given one.
    /// \details The returned interval must be a known standard step and must divide the input interval exactly.
    /// \param bar_interval_ms Bar interval duration in milliseconds.
    /// \return Lower segmentable interval, or 0 if none is compatible.
    constexpr uint64_t get_lower_bar_interval_ms(uint64_t bar_interval_ms) {
        constexpr uint64_t known_intervals[] = {
            86400000ULL, // D1
            14400000ULL, // H4
            3600000ULL,  // H1
            1800000ULL,  // M30
            900000ULL,   // M15
            300000ULL,   // M5
            60000ULL,    // M1
            15000ULL,    // S15
            5000ULL,     // S5
            3000ULL,     // S3
            1000ULL      // S1
        };

        constexpr size_t num_intervals = 10;

        for (size_t i = 0; (i + 1) < num_intervals; ++i) {
            if (bar_interval_ms == known_intervals[i]) {
                uint64_t lower = known_intervals[i + 1];
                return (bar_interval_ms % lower == 0) ? lower : 0;
            }
        }

        // If bar_interval_ms is not in the known list, search for a compatible lower one
        for (size_t i = 0; i < num_intervals; ++i) {
            if (bar_interval_ms > known_intervals[i] &&
                bar_interval_ms % known_intervals[i] == 0) {
                return known_intervals[i];
            }
        }

        return 0;
    }

    /// \brief Returns true if time_ms aligns exactly with the start of a timeframe segment.
    inline bool is_tf_aligned(uint64_t time_ms, TimeFrame tf) {
        return (time_ms % static_cast<uint64_t>(tf)) == 0;
    }

} // namespace dfh

#endif // _DTH_DATA_BARS_ENUMS_HPP_INCLUDED
