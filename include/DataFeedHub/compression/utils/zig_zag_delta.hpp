#pragma once
#ifndef _DFH_COMPRESSION_UTILS_ZIG_ZAG_DELTA_HPP_INCLUDED
#define _DFH_COMPRESSION_UTILS_ZIG_ZAG_DELTA_HPP_INCLUDED

/// \file zig_zag_delta.hpp
/// \brief

namespace dfh::compression {
    
    /// \ingroup dfh_compression
    /// \brief Encodes a 32-bit signed integer to ZigZag-encoded uint32.
    /// \details ZigZag encoding maps negative numbers to positive and uses bit shifts to encode the sign.
    /// This is the reverse of the ZigZag decoding.
    /// \param value Signed integer to be encoded.
    /// \return The ZigZag-encoded unsigned integer.
    inline std::uint32_t zigzag_encode_u32(std::int32_t value) noexcept {
        return static_cast<std::uint32_t>((value << 1) ^ (value >> 31));
    }

    /// \ingroup dfh_compression
    /// \brief Encodes a 64-bit signed integer to ZigZag-encoded uint64.
    /// \details ZigZag encoding maps negative numbers to positive and uses bit shifts to encode the sign.
    /// This is the reverse of the ZigZag decoding.
    /// \param value Signed integer to be encoded.
    /// \return The ZigZag-encoded unsigned integer.
    inline std::uint64_t zigzag_encode_u64(std::int64_t value) noexcept {
        return static_cast<std::uint64_t>((value << 1) ^ (value >> 63));
    }

    /// \ingroup dfh_compression
    /// \brief Decodes a ZigZag-encoded uint32 to a signed 32-bit integer.
    /// \details The ZigZag decoding reverses the transformation that encodes negative numbers as positive values.
    /// This function takes the ZigZag-encoded value and converts it back to the original signed integer.
    /// \param z ZigZag-encoded unsigned integer to be decoded.
    /// \return The decoded signed integer.
    inline std::int32_t zigzag_decode_u32(std::uint32_t z) noexcept {
        return static_cast<std::int32_t>((z >> 1) ^ (0u - (z & 1u)));
    }

    /// \ingroup dfh_compression
    /// \brief Decodes a ZigZag-encoded uint64 to a signed 64-bit integer.
    /// \details The ZigZag decoding reverses the transformation that encodes negative numbers as positive values.
    /// This function takes the ZigZag-encoded value and converts it back to the original signed integer.
    /// \param z ZigZag-encoded unsigned integer to be decoded.
    /// \return The decoded signed integer.
    inline std::int64_t zigzag_decode_u64(std::uint64_t z) noexcept {
        return static_cast<std::int64_t>((z >> 1) ^ (0ull - (z & 1ull)));
    }
    
//------------------------------------------------------------------------------

    /// \brief Checks whether the mathematical difference (a - b) fits in int32_t.
    /// \details Does not evaluate signed (a - b). Uses uint64_t wraparound arithmetic (mod 2^64)
    ///          and checks membership in a 2^32-sized window corresponding to [INT32_MIN, INT32_MAX].
    [[nodiscard]] inline bool diff_fits_i32(std::int64_t a, std::int64_t b) noexcept {
        // Want: d = a - b in [INT32_MIN, INT32_MAX]
        // Equivalent: (d - INT32_MIN) in [0, 2^32 - 1]
        // Compute using uint64_t (mod 2^64) to avoid signed overflow/UB.
        std::uint64_t ua = static_cast<std::uint64_t>(a);
        std::uint64_t ub = static_cast<std::uint64_t>(b);

        std::uint64_t d  = ua - ub; // modulo 2^64
        // Shift by -INT32_MIN (= 2^31) and check that the result fits in 32 bits.
        std::uint64_t x  = d - static_cast<std::uint64_t>(static_cast<std::int64_t>(INT32_MIN)); // subtract (-2^31) => add 2^31

        return x <= 0xFFFFFFFFull; // fits in int32 range
    }

    /// \brief Checks whether the mathematical difference (a - b) fits in int64_t.
    /// \details Returns false if computing (a - b) in int64_t would overflow.
    [[nodiscard]] inline bool safe_diff_check_i64(std::int64_t a, std::int64_t b) noexcept {
#       if defined(__GNUC__) || defined(__clang__)
        std::int64_t out;
        return !__builtin_sub_overflow(a, b, &out);
#       else
        // Portable fallback for compilers without __builtin_sub_overflow.
        if (b > 0) return a >= (std::numeric_limits<std::int64_t>::min() + b);
        if (b < 0) return a <= (std::numeric_limits<std::int64_t>::max() + b);
        return true;
#       endif
    }

//------------------------------------------------------------------------------

    /// \brief Delta-encodes a non-decreasing (sorted) sequence.
    /// \details
    /// Produces `output[i] = input[i] - prev`, where `prev` is `initial_value` for i=0
    /// and `input[i-1]` for i>0.
    ///
    /// \tparam InputType  Input value type (e.g., uint32_t, uint64_t).
    /// \tparam OutputType Delta type (typically same as InputType).
    /// \param input          Input array of size \p size (must be non-decreasing).
    /// \param output         Output array of deltas of size \p size.
    /// \param size           Number of elements.
    /// \param initial_value  Base value for the first delta (usually input[0] of previous block).
    /// \warning Assumes `input[i] >= prev` for all i. If this is violated and the types are unsigned,
    ///          the subtraction will underflow (wrap). For signed types, overflow/underflow is UB.
    template<class InputType = std::uint32_t, class OutputType = std::uint32_t>
    void encode_delta_sorted(
            const InputType* input,
            OutputType* output,
            std::size_t size,
            InputType initial_value
        ) {
        if (size == 0) return;

        for (std::size_t i = 0; i < size; ++i) {
            // Compute in a widened unsigned type when possible to avoid surprises on casts.
            using Wide = std::conditional_t<
                (sizeof(InputType) > sizeof(OutputType)), InputType, OutputType>;

            const Wide prev = static_cast<Wide>(initial_value);
            const Wide cur  = static_cast<Wide>(input[i]);
            const Wide d    = cur - prev;

            output[i] = static_cast<OutputType>(d);
            initial_value = input[i];
        }
    }

    /// \brief Decodes a delta-encoded non-decreasing (sorted) sequence.
    /// \details
    /// Restores values as:
    /// - `output[0] = initial_value + input[0]`
    /// - `output[i] = output[i-1] + input[i]` for i>0
    ///
    /// \tparam InputType  Delta type (e.g., uint32_t, uint64_t).
    /// \tparam OutputType Output value type (typically same as InputType).
    /// \param input          Input array of deltas of size \p size.
    /// \param output         Output array of restored values of size \p size.
    /// \param size           Number of elements.
    /// \param initial_value  Base value for the first element (the same base used by encoder).
    /// \warning For unsigned types, overflow wraps around. For signed types, overflow is UB.
    template<class InputType = std::uint32_t, class OutputType = std::uint32_t>
    void decode_delta_sorted(
            const InputType* input,
            OutputType* output,
            std::size_t size,
            OutputType initial_value
        ) {
        if (size == 0) return;
        output[0] = static_cast<OutputType>(initial_value + static_cast<OutputType>(input[0]));
        for (std::size_t i = 1; i < size; ++i) {
            output[i] = static_cast<OutputType>(output[i - 1] + static_cast<OutputType>(input[i]));
        }
    }

//------------------------------------------------------------------------------

    /// \defgroup dfh_tick_time_delta Time delta codecs
    /// \ingroup dfh_compression
    /// \brief Encoding/decoding of tick timestamps as millisecond deltas.
    ///
    /// The encoder stores timestamps as non-negative deltas in milliseconds:
    /// - \c delta[0] = ticks[0].time_ms - base_time_ms
    /// - \c delta[i] = ticks[i].time_ms - ticks[i-1].time_ms, for i > 0
    ///
    /// The decoder reconstructs absolute timestamps:
    /// - \c ticks[0].time_ms = delta[0] + base_time_ms
    /// - \c ticks[i].time_ms = ticks[i-1].time_ms + delta[i]
    ///
    /// Requirements:
    /// - Input timestamps must be non-decreasing.
    /// - Deltas are stored as \c uint32_t, so callers must ensure each delta fits 32 bits.
    ///   Pick an appropriate \c base_time_ms and sampling interval.
    ///
    /// \par Example
    /// \code
    /// struct MarketTick { std::uint64_t time_ms; };
    ///
    /// std::vector<MarketTick> ticks = { {1000}, {1010}, {1050} };
    /// const std::int64_t base_time_ms = 900;
    ///
    /// std::vector<std::uint32_t> deltas(ticks.size());
    /// dfh::compression::encode_time_delta(ticks.data(), deltas.data(), ticks.size(), base_time_ms);
    ///
    /// std::vector<MarketTick> restored(ticks.size());
    /// dfh::compression::decode_time_delta(deltas.data(), restored.data(), restored.size(), base_time_ms);
    /// \endcode


    /// \ingroup dfh_tick_time_delta
    /// \brief Encodes \c time_ms into millisecond deltas stored in \c uint32_t.
    /// \details Deltas are guaranteed to fit into signed 32-bit range \c [0..INT32_MAX]
    ///          (SIMD-friendly invariant). The input timestamps must be non-decreasing.
    /// \tparam TickType Tick structure type. Must provide a readable member \c time_ms (recommended \c int64_t).
    /// \param ticks Pointer to the first tick.
    /// \param output Output buffer. Must contain at least \p size elements.
    /// \param size Number of ticks to encode.
    /// \param initial_time Base timestamp in milliseconds for the first delta (must be non-negative).
    /// \throw std::invalid_argument If \p initial_time is negative or if \c time_ms is not non-decreasing.
    /// \throw std::overflow_error If any delta exceeds \c INT32_MAX.
    template<class TickType>
    void encode_time_delta(
            const TickType* ticks,
            std::uint32_t* output,
            std::size_t size,
            std::int64_t initial_time) {
        if (size == 0) return;
        if (initial_time < 0) throw std::invalid_argument("encode_time_delta: initial_time must be >= 0");

        std::int64_t cur, delta;
        for (std::size_t i = 0; i < size; ++i) {
            cur = ticks[i].time_ms;
            if (cur < initial_time)  throw std::invalid_argument("encode_time_delta: timestamps must be non-decreasing");
            delta = cur - initial_time;
            if (delta > std::numeric_limits<std::int32_t>::max()) throw std::overflow_error("encode_time_delta: delta > int32_max");
            output[i] = static_cast<std::uint32_t>(delta);
            initial_time = cur;
        }
    }

    /// \ingroup dfh_tick_time_delta
    /// \brief Decodes millisecond deltas stored in \c uint32_t back into \c time_ms.
    /// \details Assumes deltas were produced by \ref encode_time_delta and therefore fit into
    ///          signed 32-bit range \c [0..INT32_MAX]. No validation is performed.
    /// \tparam TickType Tick structure type. Must provide a writable member \c time_ms (recommended \c int64_t).
    /// \param deltas Input deltas array. Must contain \p size elements.
    /// \param ticks Output tick array. Must contain \p size elements. Only \c time_ms is written.
    /// \param size Number of elements to decode.
    /// \param initial_time Base timestamp in milliseconds for the first element (must be non-negative).
    template<class TickType>
    void decode_time_delta(
            const std::uint32_t* deltas,
            TickType* ticks,
            std::size_t size,
            std::int64_t initial_time) {
        if (size == 0) return;

        for (std::size_t i = 0; i < size; ++i) {
            initial_time += static_cast<std::int64_t>(deltas[i]);
            ticks[i].time_ms = initial_time;
        }
    }
    
//------------------------------------------------------------------------------

    /// \ingroup dfh_tick_id_delta
    /// \brief Tries to encode trade IDs as ZigZag-encoded int32 deltas stored in \c uint32_t.
    /// \details
    /// Stores deltas as: \c delta = (cur_id - prev_id - 1).
    /// This makes consecutive IDs (prev_id + 1) produce zero deltas, improving compression.
    /// The function is intended for "try u32, fallback to u64": it returns \c false if a delta
    /// does not fit into \c int32_t.
    /// \tparam TickType Tick structure type. Must provide:
    /// - \c trade_id() returning an integer ID (expected to fit into \c int64_t),
    /// - \c set_trade_id(uint64_t) for decoding.
    /// \param ticks Pointer to input ticks.
    /// \param output Output buffer for encoded deltas. Must contain at least \p size elements.
    /// \param size Number of ticks to encode.
    /// \param initial_id Base ID used for the first delta.
    /// \return \c true if all deltas fit in \c int32_t; otherwise \c false.
    /// \throw std::invalid_argument If \p initial_id is out of range or if IDs are not strictly increasing.
    ///
    /// \note This encoder assumes \c trade_id fits into signed 64-bit and is strictly increasing.
    template<class TickType>
    bool encode_id_delta_u32(
            const TickType* ticks,
            std::uint32_t* output,
            std::size_t size,
            std::int64_t initial_id) {
        if (size == 0) return true;
		if (initial_id < 0 ||
            initial_id >= static_cast<std::int64_t>(TickType::max_trade_id())) {
            throw std::invalid_argument("encode_id_delta_u32: initial_id out of range");
        }

		const std::int64_t first_id = static_cast<std::int64_t>(ticks[0].trade_id());
        if (first_id <= initial_id) {
            throw std::invalid_argument("encode_id_delta_u32: ticks[0].trade_id() <= initial_id");
        }

        constexpr std::int64_t delta_offset = 1;
        std::int64_t cur_id, delta;
        for (std::size_t i = 0; i < size; ++i) {
            cur_id = static_cast<std::int64_t>(ticks[i].trade_id());
			if (!diff_fits_i64(cur_id, initial_id)) throw std::invalid_argument("encode_id_delta_u32: int64 diff overflow");
			delta = cur_id - delta_offset;
            if (!diff_fits_i32(delta, initial_id)) return false; // fallback to u64 path 
            delta -= initial_id;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(delta));
            initial_id = cur_id;
        }
        return true;
    }
    
    /// \ingroup dfh_tick_id_delta
    /// \brief Decodes ZigZag-encoded int32 deltas stored in \c uint32_t back into trade IDs.
    /// \details
    /// Restores IDs using: \c cur_id = prev_id + delta + 1, where \c delta is ZigZag-decoded.
    /// \tparam TickType Tick structure type. Must provide \c set_trade_id(uint64_t).
    /// \param deltas Input deltas array. Must contain \p size elements.
    /// \param ticks Output tick array. Must contain \p size elements. Only trade ID is written.
    /// \param size Number of elements to decode.
    /// \param initial_id Base ID used for the first reconstruction.
    /// \note No range validation is performed; caller must ensure input is valid.
    template<class TickType>
    void decode_id_delta_u32(
            const std::uint32_t* deltas,
            TickType* ticks,
            std::size_t size,
            std::int64_t initial_id) {
        constexpr std::int64_t delta_offset = 1;
        for (std::size_t i = 0; i < size; ++i) {
            initial_id += static_cast<std::int64_t>(zigzag_decode_u32(deltas[i])) + delta_offset;
            ticks[i].set_trade_id(static_cast<std::uint64_t>(initial_id));
        }
    }
    
    /// \ingroup dfh_tick_id_delta
    /// \brief Encodes trade IDs as ZigZag-encoded int64 deltas stored in \c uint64_t.
    /// \details
    /// Stores deltas as: \c delta = (cur_id - prev_id - 1).
    /// This makes consecutive IDs (prev_id + 1) produce zero deltas, improving compression.
    /// \tparam TickType Tick structure type. Must provide \c trade_id().
    /// \param ticks Pointer to input ticks.
    /// \param output Output buffer for encoded deltas. Must contain at least \p size elements.
    /// \param size Number of ticks to encode.
    /// \param initial_id Base ID used for the first delta.
    /// \throw std::invalid_argument If \p initial_id is out of range or if IDs are not strictly increasing.
    ///
    /// \note This encoder assumes \c trade_id fits into signed 64-bit and is strictly increasing.
    template<class TickType>
    void encode_id_delta_u64(
            const TickType* ticks,
            std::uint64_t* output,
            std::size_t size,
            std::int64_t initial_id) {
        if (size == 0) return true;
		if (initial_id < 0 ||
            initial_id >= static_cast<std::int64_t>(TickType::max_trade_id())) {
            throw std::invalid_argument("encode_id_delta_u32: initial_id out of range");
        }

		const std::int64_t first_id = static_cast<std::int64_t>(ticks[0].trade_id());
        if (first_id <= initial_id) {
            throw std::invalid_argument("encode_id_delta_u32: ticks[0].trade_id() <= initial_id");
        }

        constexpr std::int64_t delta_offset = 1;
        std::int64_t cur_id, delta;
        for (std::size_t i = 0; i < size; ++i) {
            cur_id = static_cast<std::int64_t>(ticks[i].trade_id());
			if (!diff_fits_i64(cur_id, initial_id)) throw std::invalid_argument("encode_id_delta_u64: int64 diff overflow");
			delta = cur_id - delta_offset;
            if (!diff_fits_i64(delta, initial_id)) throw std::invalid_argument("encode_id_delta_u64: int64 diff overflow");
            delta -= initial_id;
            output[i] = zigzag_encode_u64(delta);
            initial_id = cur_id;
        }
    }
    
    /// \ingroup dfh_tick_id_delta
    /// \brief Decodes ZigZag-encoded int64 deltas stored in \c uint64_t back into trade IDs.
    /// \details
    /// Restores IDs using: \c cur_id = prev_id + delta + 1, where \c delta is ZigZag-decoded.
    /// \tparam TickType Tick structure type. Must provide \c set_trade_id(uint64_t).
    /// \param deltas Input deltas array. Must contain \p size elements.
    /// \param ticks Output tick array. Must contain \p size elements. Only trade ID is written.
    /// \param size Number of elements to decode.
    /// \param initial_id Base ID used for the first reconstruction.
    /// \note No range validation is performed; caller must ensure input is valid.
    template<class TickType>
    void decode_id_delta_u64(
            const std::uint64_t* deltas,
            TickType* ticks,
            std::size_t size,
            std::int64_t initial_id) {
        constexpr std::int64_t delta_offset = 1;
        for (std::size_t i = 0; i < size; ++i) {
            initial_id += zigzag_decode_u64(deltas[i]) + delta_offset;
            ticks[i].set_trade_id(static_cast<std::uint64_t>(initial_id));
        }
    }

//------------------------------------------------------------------------------

    /// \defgroup dfh_tick_price_delta_zigzag Price delta ZigZag codecs
    /// \ingroup dfh_compression
    /// \brief Helpers for encoding/decoding tick price fields as delta + ZigZag integers.
    ///
    /// These helpers convert a selected floating-point price field into a scaled integer series:
    /// `scaled = llround(price * price_scale)`, then store per-element deltas relative to the previous
    /// scaled value and ZigZag-encode signed deltas into unsigned integers.
    ///
    /// Supported output formats:
    /// - int32 deltas packed into uint32 (`encode_price_delta_zig_zag_int32` / `decode_price_delta_zig_zag_int32`)
    /// - int64 deltas packed into uint64 (`encode_price_delta_zig_zag_int64` / `decode_price_delta_zig_zag_int64`)
    ///
    /// The price field is selected at compile time via a pointer-to-member: `double TickType::*`.
    ///
    /// \par Example
    /// \code
    /// struct MarketTick {
    ///     double last = 0.0;
    ///     double bid  = 0.0;
    ///     double ask  = 0.0;
    /// };
    ///
    /// std::vector<MarketTick> ticks = {
    ///     {1.23450, 1.23440, 1.23460},
    ///     {1.23455, 1.23445, 1.23465},
    ///     {1.23460, 1.23450, 1.23470}
    /// };
    ///
    /// const double price_scale = 1e5; // 5 digits
    /// const std::int64_t init_last = std::llround(ticks[0].last * price_scale);
    ///
    /// std::vector<std::uint32_t> deltas(ticks.size());
    /// dfh::compression::encode_price_delta_zig_zag_int32<MarketTick, &MarketTick::last>(
    ///     ticks.data(), deltas.data(), ticks.size(), price_scale, init_last);
    ///
    /// std::vector<MarketTick> restored(ticks.size());
    /// dfh::compression::decode_price_delta_zig_zag_int32<MarketTick, &MarketTick::last>(
    ///     deltas.data(), restored.data(), restored.size(), price_scale, init_last);
    /// \endcode
    ///
    /// \note \p initial_price is an integer in the scaled domain (same units as `llround(price * price_scale)`).
    /// \note Decode functions write only the selected field; other fields in ticks are untouched.
    /// \note The int32 variant validates that each delta fits into int32 range.


    /// \ingroup dfh_tick_price_delta_zigzag
    /// \brief Encodes a selected price field as ZigZag-encoded int32 deltas stored in uint32.
    /// \details Returns \c false if a delta does not fit into int32 range. Intended for "try u32, fallback to u64".
    /// \tparam TickType Tick structure type.
    /// \tparam PriceMember Pointer-to-member selecting the price field (double TickType::*).
    /// \param ticks Pointer to the first tick.
    /// \param output Output buffer. Must contain at least \p size elements.
    /// \param size Number of ticks to encode.
    /// \param price_scale Scale factor, typically 10^digits.
    /// \param initial_price Initial scaled price value used as the delta base.
    /// \return \c true if all deltas fit in int32_t; otherwise \c false.
    template<class TickType, double TickType::* PriceMember>
    bool encode_price_delta_zig_zag_u32(
            const TickType* ticks,
            std::uint32_t* output,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) {
        if (size == 0) return true;

        constexpr std::int64_t min_i32 =
            static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min());
        constexpr std::int64_t max_i32 =
            static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max());

        std::int64_t scaled_price, delta; 
        for (std::size_t i = 0; i < size; ++i) {
            scaled_price = std::llround((ticks[i].*PriceMember) * price_scale);
            if (!diff_fits_i32(scaled_price, initial_price)) return false;
            output[i] = zigzag_encode_u32(static_cast<std::int32_t>(scaled_price - initial_price));
            initial_price = scaled_price;
        }
        return true;
    }

    /// \ingroup dfh_tick_price_delta_zigzag
    /// \brief Decodes ZigZag-encoded int32 deltas stored in uint32 into a selected price field.
    /// \tparam TickType Tick structure type.
    /// \tparam PriceMember Pointer-to-member selecting the price field (double TickType::*).
    /// \param deltas Input deltas array. Must contain \p size elements.
    /// \param ticks Output tick array. Must contain \p size elements. Only the selected field is written.
    /// \param size Number of elements to decode.
    /// \param price_scale Scale factor used in encoding, typically 10^digits.
    /// \param initial_price Initial scaled price value used to reconstruct the series.
    /// \note Writes only \p PriceMember in each tick; other fields remain unchanged.
    template<class TickType, double TickType::* PriceMember>
    void decode_price_delta_zig_zag_u32(
            const std::uint32_t* deltas,
            TickType* ticks,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) {
        const double inv_scale = 1.0 / price_scale;
        std::int64_t scaled_price;
        for (std::size_t i = 0; i < size; ++i) {
            scaled_price = initial_price + zigzag_decode_u32(deltas[i]);
            ticks[i].*PriceMember = static_cast<double>(scaled_price) * inv_scale;
            initial_price = scaled_price;
        }
    }

    /// \ingroup dfh_tick_price_delta_zigzag
    /// \brief Encodes a selected price field as ZigZag-encoded int64 deltas stored in uint64.
    /// \details Throws if an intermediate subtraction would overflow int64_t.
    /// \tparam TickType Tick structure type.
    /// \tparam PriceMember Pointer-to-member selecting the price field (double TickType::*).
    /// \param ticks Pointer to the first tick.
    /// \param output Output buffer. Must contain at least \p size elements.
    /// \param size Number of ticks to encode.
    /// \param price_scale Scale factor, typically 10^digits.
    /// \param initial_price Initial scaled price value used as the delta base.
    /// \throw std::overflow_error If (scaled_price - initial_price) would overflow int64_t.
    template<class TickType, double TickType::* PriceMember>
    void encode_price_delta_zig_zag_u64(
            const TickType* ticks,
            std::uint64_t* output,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price) {
        if (size == 0) return;
        constexpr std::int64_t min_i64 = std::numeric_limits<std::int64_t>::min();
        constexpr std::int64_t max_i64 = std::numeric_limits<std::int64_t>::max();
        std::int64_t scaled_price;
        for (std::size_t i = 0; i < size; ++i) {
            scaled_price = std::llround((ticks[i].*PriceMember) * price_scale);
            if (!safe_diff_check_i64(scaled_price, initial_price)) throw std::overflow_error("encode_price_delta_zig_zag_u64: int64 delta overflow");
            output[i] = zigzag_encode_u64(scaled_price - initial_price);
            initial_price = scaled_price;
        }
    }

    /// \ingroup dfh_tick_price_delta_zigzag
    /// \brief Decodes ZigZag-encoded int64 deltas stored in uint64 into a selected price field.
    /// \tparam TickType Tick structure type.
    /// \tparam PriceMember Pointer-to-member selecting the price field (double TickType::*).
    /// \param deltas Input deltas array. Must contain \p size elements.
    /// \param ticks Output tick array. Must contain \p size elements. Only the selected field is written.
    /// \param size Number of elements to decode.
    /// \param price_scale Scale factor used in encoding, typically 10^digits.
    /// \param initial_price Initial scaled price value used to reconstruct the series.
    /// \note Writes only \p PriceMember in each tick; other fields remain unchanged.
    template<class TickType, double TickType::* PriceMember>
    void decode_price_delta_zig_zag_u64(
            const std::uint64_t* deltas,
            TickType* ticks,
            std::size_t size,
            double price_scale,
            std::int64_t initial_price
        ) {
        const double inv_scale = 1.0 / price_scale;
        std::int64_t scaled_price;
        for (std::size_t i = 0; i < size; ++i) {
            scaled_price = initial_price + zigzag_decode_u64(deltas[i]);
            ticks[i].*PriceMember = static_cast<double>(scaled_price) * inv_scale;
            initial_price = scaled_price;
        }
    }

//------------------------------------------------------------------------------

    /// \brief Performs delta and Zig-Zag encoding in a single pass.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    /// \param initial_value
    void encode_delta_zig_zag_u32(
            const std::int32_t* input,
            std::uint32_t* output,
            std::size_t size,
            std::int32_t initial_value) {
        if (size == 0) return;
        std::int32_t prev = initial_value, delta_value;
#       if defined(__SSE2__)
        constexpr std::size_t simd_width = 4;
        const std::size_t aligned_size = size - ((size - 1) % simd_width);
        __m128i delta;

        if (reinterpret_cast<std::uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<std::uintptr_t>(output) % 16 == 0) {
            if (aligned_size > 1) {
                for (std::size_t i = 0; i < simd_width; ++i) {
                    delta_value = input[i] - prev;
                    output[i] = (delta_value << 1) ^ (delta_value >> 31);
                    prev = input[i];
                }
            }
            for (std::size_t i = simd_width; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i - 1])));

                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1),
                        _mm_srai_epi32(delta, 31)));
            }
        } else {
            for (std::size_t i = 1; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i - 1])));

                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1),
                        _mm_srai_epi32(delta, 31)));
            }
        }

        delta_value = input[0] - initial_value;
        output[0] = (delta_value << 1) ^ (delta_value >> 31);
        prev = aligned_size > 0 ? input[aligned_size - 1] : initial_value;
        for (std::size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - prev;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
            prev = input[i];
        }
#       else
        for (std::size_t i = 0; i < size; ++i) {
            delta_value = input[i] - prev;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
            prev = input[i];
        }
#       endif
    }

    /// \brief Performs delta and Zig-Zag decoding in a single pass.
    /// \param input Pointer to the encoded array.
    /// \param output Pointer to the decoded array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for reconstruction.
    void decode_delta_zig_zag_u32(
            const std::uint32_t* input,
            std::int32_t* output,
            std::size_t size,
            std::int32_t initial_value) {
        if (size == 0) return;
        std::int32_t zigzag = (input[0] >> 1) ^ -(input[0] & 1);
        output[0] = initial_value + zigzag;
        for (std::size_t i = 1; i < size; ++i) {
            zigzag = (input[i] >> 1) ^ -(input[i] & 1);
            output[i] = output[i - 1] + zigzag;
        }
    }

    void encode_delta_zig_zag_u32(
            const std::uint32_t* input,
            std::uint32_t* output,
            std::size_t size,
            std::uint32_t initial_value) {
        constexpr std::int64_t min_val = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::min());
        constexpr std::int64_t max_val = static_cast<std::int64_t>(std::numeric_limits<std::int32_t>::max());
        std::int64_t raw_delta;
        std::int32_t delta;
        for (std::size_t i = 0; i < size; ++i) {
            raw_delta = static_cast<std::int64_t>(input[i]) - static_cast<std::int64_t>(initial_value);
            if (raw_delta < min_val || raw_delta > max_val) throw std::overflow_error("Delta overflow: input[i] - initial_value > int32 range");
            initial_value = input[i];
            delta = static_cast<std::int32_t>(raw_delta);
            output[i] = (delta << 1) ^ (delta >> 31);
        }
    }

    void decode_delta_zig_zag_u32(
            const std::uint32_t* input,
            std::uint32_t* output,
            std::size_t size,
            std::uint32_t initial_value) {
        if (size == 0) return;
        std::int64_t zigzag = (input[0] >> 1) ^ -(input[0] & 1);
        output[0] = static_cast<std::uint32_t>(static_cast<std::int64_t>(initial_value) + zigzag);
        for (std::size_t i = 1; i < size; ++i) {
            zigzag = (input[i] >> 1) ^ -(input[i] & 1);
            output[i] = static_cast<std::uint32_t>(static_cast<std::int64_t>(output[i - 1]) + zigzag);
        }
    }

//------------------------------------------------------------------------------

    /// \brief Performs delta and Zig-Zag encoding in a single pass (64-bit).
    /// \param input Pointer to the input array (int64_t).
    /// \param output Pointer to the output array (uint64_t).
    /// \param size Number of elements in the array.
    /// \param initial_value The reference value for delta computation (64-bit).
    void encode_delta_zig_zag_u64(
            const std::int64_t* input,
            std::uint64_t* output,
            std::size_t size,
            std::int64_t initial_value) {
        if (size == 0) return;
        std::int64_t prev = initial_value, delta_value;

#       if defined(__SSE2__)
        constexpr std::size_t simd_width = 2; // SSE2 обрабатывает 2 int64 за раз
        const std::size_t aligned_size = size - ((size - 1) % simd_width);
        __m128i delta;

        if (reinterpret_cast<std::uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<std::uintptr_t>(output) % 16 == 0) {
            if (aligned_size > 1) {
                for (std::size_t i = 0; i < simd_width; ++i) {
                    delta_value = input[i] - prev;
                    output[i] = (delta_value << 1) ^ (delta_value >> 63);
                    prev = input[i];
                }
            }
            for (std::size_t i = simd_width; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi64(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i - 1])));
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))
                    ));
            }
        } else {
            for (std::size_t i = 1; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi64(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i - 1])));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))
                    ));
            }
        }

        delta_value = input[0] - initial_value;
        output[0] = (delta_value << 1) ^ (delta_value >> 63);
        prev = aligned_size > 0 ? input[aligned_size - 1] : initial_value;
        for (std::size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - prev;
            output[i] = (delta_value << 1) ^ (delta_value >> 63);
            prev = input[i];
        }
#       else
        for (std::size_t i = 0; i < size; ++i) {
            delta_value = input[i] - prev;
            output[i] = (delta_value << 1) ^ (delta_value >> 63);
            prev = input[i];
        }
#       endif
    }

    /// \brief Performs delta and Zig-Zag decoding in a single pass for 64-bit integers.
    /// \param input Pointer to the encoded array.
    /// \param output Pointer to the decoded array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for reconstruction.
    void decode_delta_zig_zag_u64(
            const std::uint64_t* input,
            std::int64_t* output,
            std::size_t size,
            std::int64_t initial_value) {
        if (size == 0) return;
        std::int64_t zigzag = (input[0] >> 1) ^ -(input[0] & 1);
        output[0] = initial_value + zigzag;
        for (std::size_t i = 1; i < size; ++i) {
            zigzag = (input[i] >> 1) ^ -(input[i] & 1);
            output[i] = output[i - 1] + zigzag;
        }
    }

    void encode_delta_zig_zag_u64(
            const std::uint64_t* input,
            std::uint64_t* output,
            std::size_t size,
            std::uint64_t initial_value) {
        std::int64_t delta;
        for (std::size_t i = 0; i < size; ++i) {
            delta = static_cast<std::int64_t>(input[i]) - static_cast<std::int64_t>(initial_value);
            initial_value = input[i];
            output[i] = (delta << 1) ^ (delta >> 63);
        }
    }

    void decode_delta_zig_zag_u64(
            const std::uint64_t* input,
            std::uint64_t* output,
            std::size_t size,
            std::uint64_t initial_value) {
        if (size == 0) return;
        std::int64_t zigzag = (input[0] >> 1) ^ -(input[0] & 1);
        output[0] = static_cast<std::uint64_t>(static_cast<std::int64_t>(initial_value) + zigzag);
        for (std::size_t i = 1; i < size; ++i) {
            zigzag = (input[i] >> 1) ^ -(input[i] & 1);
            output[i] = static_cast<std::uint64_t>(static_cast<int64_t>(output[i - 1]) + zigzag);
        }
    }

//------------------------------------------------------------------------------

    /// \brief Performs delta and Zig-Zag encoding in a single pass.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for delta computation.
    void encode_delta_zig_zag_chunked8_u32(
            const std::int32_t* input, 
            std::uint32_t* output, 
            std::size_t size, 
            std::int32_t initial_value) {
        if (size == 0) return;

        constexpr std::size_t simd_width = 8;
        const std::size_t aligned_size = size - (size % simd_width);

#       if defined(__SSE2__)
        constexpr std::size_t sse_simd_width = 4;

        __m128i base = _mm_set1_epi32(initial_value);
        __m128i delta;

        if (reinterpret_cast<std::uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<std::uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                delta = _mm_sub_epi32(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width])),
                    base);
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                base = _mm_set1_epi32(input[i + simd_width - 1]);
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                delta = _mm_sub_epi32(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width])),
                    base);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                base = _mm_set1_epi32(input[i + simd_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = input[aligned_size - 1];
        }

        std::int32_t delta_value;
        for (std::size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
        }
#       else
        std::int32_t delta_value;
        for (std::size_t i = 0; i < aligned_size; i += simd_width) {
            std::size_t j_max = i + simd_width;
            for (std::size_t j = i; j < j_max; ++j) {
                delta_value = input[j] - initial_value;
                output[j] = (delta_value << 1) ^ (delta_value >> 31);
            }
            initial_value = input[j_max - 1];
        }
        for (std::size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
        }
#       endif
    }

    /// \brief Performs delta and Zig-Zag decoding in a single pass.
    /// \param input Pointer to the encoded array.
    /// \param output Pointer to the decoded array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for reconstruction.
    void decode_delta_zig_zag_chunked8_u32(
            const std::uint32_t* input, 
            std::int32_t* output, 
            std::size_t size, 
            std::int32_t initial_value) {
        if (size == 0) return;

        constexpr std::size_t simd_width = 8;
        const std::size_t aligned_size = size - (size % simd_width);

#       if defined(__SSE2__)
        constexpr std::size_t sse_simd_width = 4;

        __m128i base = _mm_set1_epi32(initial_value);
        __m128i delta;

        const __m128i one = _mm_set1_epi32(1);
        const __m128i zero = _mm_setzero_si128();

        if (reinterpret_cast<std::uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<std::uintptr_t>(output) % 16 == 0) {
            for (std::size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                delta = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width]));
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi32(output[i + simd_width - 1]);
            }
        } else {
            for (std::size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                delta = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width]));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi32(output[i + simd_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = output[aligned_size - 1];
        }

        for (std::size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
        }
#       else
        for (std::size_t i = 0; i < aligned_size; i += simd_width) {
            std::size_t j_max = i + simd_width;
            for (std::size_t j = i; j < j_max; ++j) {
                output[j] = initial_value + ((input[j] >> 1) ^ -(input[j] & 1));
                initial_value = output[j];
            }
        }
        for (std::size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
            initial_value = output[i];
        }
#       endif
    }

//------------------------------------------------------------------------------

    /// \brief Performs delta and Zig-Zag encoding in a single pass.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for delta computation.
    void encode_delta_zig_zag_chunked4_u32(
            const std::int32_t* input, 
            std::uint32_t* output, 
            std::size_t size, 
            std::int32_t initial_value) {
        if (size == 0) return;

        constexpr std::size_t simd_width = 4;
        const std::size_t aligned_size = size - (size % simd_width);

#       if defined(__SSE2__)
        __m128i base = _mm_set1_epi32(initial_value);
        __m128i delta;

        if (reinterpret_cast<std::uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<std::uintptr_t>(output) % 16 == 0) {
            for (std::size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                base = _mm_set1_epi32(input[i + simd_width - 1]);
            }
        } else {
            for (std::size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi32(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(_mm_slli_epi32(delta, 1), _mm_srai_epi32(delta, 31)));

                base = _mm_set1_epi32(input[i + simd_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = input[aligned_size - 1];
        }

        std::int32_t delta_value;
        for (std::size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
        }
#       else
        std::int32_t delta_value;
        for (std::size_t i = 0; i < aligned_size; i += simd_width) {
            std::size_t j_max = i + simd_width;
            for (std::size_t j = i; j < j_max; ++j) {
                delta_value = input[j] - initial_value;
                output[j] = (delta_value << 1) ^ (delta_value >> 31);
            }
            initial_value = input[j_max - 1];
        }
        for (std::size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 31);
        }
#       endif
    }

    /// \brief Performs delta and Zig-Zag decoding in a single pass.
    /// \param input Pointer to the encoded array.
    /// \param output Pointer to the decoded array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for reconstruction.
    void decode_delta_zig_zag_chunked4_u32(
            const std::uint32_t* input, 
            std::int32_t* output, 
            std::size_t size, 
            std::int32_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 4;
        const size_t aligned_size = size - (size % simd_width);

#       if defined(__SSE2__)
        __m128i base = _mm_set1_epi32(initial_value);
        __m128i delta;

        const __m128i one = _mm_set1_epi32(1);
        const __m128i zero = _mm_setzero_si128();

        if (reinterpret_cast<std::uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<std::uintptr_t>(output) % 16 == 0) {
            for (std::size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi32(output[i + simd_width - 1]);
            }
        } else {
            for (std::size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i]));
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi32(_mm_xor_si128(_mm_srli_epi32(delta, 1),
                        _mm_sub_epi32(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi32(output[i + simd_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = output[aligned_size - 1];
        }

        for (std::size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
        }
#       else
        for (std::size_t i = 0; i < aligned_size; i += simd_width) {
            std::size_t j_max = i + simd_width;
            for (std::size_t j = i; j < j_max; ++j) {
                output[j] = initial_value + ((input[j] >> 1) ^ -(input[j] & 1));
                initial_value = output[j];
            }
        }
        for (std::size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
            initial_value = output[i];
        }
#       endif
    }

//------------------------------------------------------------------------------

    /// \brief Performs delta and Zig-Zag encoding in a single pass for 64-bit integers.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for delta computation.
    void encode_delta_zig_zag_chunked4_u64(
            const std::int64_t* input, 
            std::uint64_t* output, 
            std::size_t size, 
            std::int64_t initial_value) {
        if (size == 0) return;

        constexpr std::size_t simd_width = 4;
        const std::size_t aligned_size = size - (size % simd_width);

#       if defined(__SSE2__)
        constexpr std::size_t sse_simd_width = 2;

        __m128i base = _mm_set1_epi64x(initial_value);
        __m128i delta;

        if (reinterpret_cast<std::uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<std::uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi64(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))));


                delta = _mm_sub_epi64(
                    _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width])),
                    base);
                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))));

                base = _mm_set1_epi64x(input[i + simd_width - 1]);
            }
        } else {
            for (size_t i = 0; i < aligned_size; i += simd_width) {
                delta = _mm_sub_epi64(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i])),
                    base);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))));


                delta = _mm_sub_epi64(
                    _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width])),
                    base);
                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_xor_si128(
                        _mm_slli_epi64(delta, 1),
                        _mm_or_si128(_mm_srli_epi64(delta, 63), _mm_slli_epi64(_mm_srai_epi32(_mm_shuffle_epi32(delta, _MM_SHUFFLE(3, 3, 1, 1)), 31), 1))));

                base = _mm_set1_epi64x(input[i + simd_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = input[aligned_size - 1];
        }

        std::int64_t delta_value;
        for (std::size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 63);
        }
#       else
        std::int64_t delta_value;
        for (std::size_t i = 0; i < aligned_size; i += simd_width) {
            std::size_t j_max = i + simd_width;
            for (std::size_t j = i; j < j_max; ++j) {
                delta_value = input[j] - initial_value;
                output[j] = (delta_value << 1) ^ (delta_value >> 63);
            }
            initial_value = input[j_max - 1];
        }
        for (std::size_t i = aligned_size; i < size; ++i) {
            delta_value = input[i] - initial_value;
            output[i] = (delta_value << 1) ^ (delta_value >> 63);
        }
#       endif
    }

    /// \brief Performs delta and Zig-Zag decoding in a single pass for 64-bit integers.
    /// \param input Pointer to the encoded array.
    /// \param output Pointer to the decoded array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for reconstruction.
    void decode_delta_zig_zag_chunked4_u64(
            const std::uint64_t* input, 
            std::int64_t* output, 
            std::size_t size, 
            std::int64_t initial_value) {
        if (size == 0) return;

        constexpr std::size_t chunk_width = 4;
        const std::size_t aligned_size = size - (size % chunk_width);

#       if defined(__SSE2__)
        constexpr std::size_t sse_simd_width = 2;
        __m128i base = _mm_set1_epi64x(initial_value);
        __m128i delta;

        const __m128i one = _mm_set1_epi64x(1);
        const __m128i zero = _mm_setzero_si128();

        if (reinterpret_cast<std::uintptr_t>(input) % 16 == 0 &&
            reinterpret_cast<std::uintptr_t>(output) % 16 == 0) {
            for (size_t i = 0; i < aligned_size; i += chunk_width) {
                delta = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i]));

                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi64(_mm_xor_si128(
                        _mm_srli_epi64(delta, 1),
                        _mm_sub_epi64(zero, _mm_and_si128(delta, one))), base));

                delta = _mm_load_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width]));

                _mm_store_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_add_epi64(_mm_xor_si128(
                        _mm_srli_epi64(delta, 1),
                        _mm_sub_epi64(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi64x(output[i + chunk_width - 1]);
            }
        } else {
            for (std::size_t i = 0; i < aligned_size; i += chunk_width) {
                delta = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i]));

                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i]),
                    _mm_add_epi64(_mm_xor_si128(
                        _mm_srli_epi64(delta, 1),
                        _mm_sub_epi64(zero, _mm_and_si128(delta, one))), base));

                delta = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&input[i + sse_simd_width]));

                _mm_storeu_si128(reinterpret_cast<__m128i*>(&output[i + sse_simd_width]),
                    _mm_add_epi64(_mm_xor_si128(
                        _mm_srli_epi64(delta, 1),
                        _mm_sub_epi64(zero, _mm_and_si128(delta, one))), base));

                base = _mm_set1_epi64x(output[i + chunk_width - 1]);
            }
        }

        if (aligned_size > 0) {
            initial_value = output[aligned_size - 1];
        }

        for (std::size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
        }
#       else
        for (std::size_t i = 0; i < aligned_size; i += chunk_width) {
            std::size_t j_max = i + chunk_width;
            for (std::size_t j = i; j < j_max; ++j) {
                output[j] = initial_value + ((input[j] >> 1) ^ -(input[j] & 1));
                initial_value = output[j];
            }
            initial_value = output[j_max - 1];
        }
        for (std::size_t i = aligned_size; i < size; ++i) {
            output[i] = initial_value + ((input[i] >> 1) ^ -(input[i] & 1));
            initial_value = output[i];
        }
#       endif
    }

};

#endif // _DFH_COMPRESSION_UTILS_ZIG_ZAG_DELTA_HPP_INCLUDED
