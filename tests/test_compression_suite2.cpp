#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <random>
#include <chrono>
#include <fstream>
#include <string>
#include <stdexcept>
#include <map>
#include <set>
#include <unordered_map>
#include <fast_double_parser.h>
#include <Eigen/Dense>
#include <time_shield_cpp/time_shield.hpp>

namespace dfh {
namespace utils {

    /// \brief Returns a power of 10 based on the number of digits.
    /// \tparam T Return type, which can be any numeric type (e.g., int64_t, uint64_t, double).
    /// \param digits Number of decimal places (must be in the range 0-18).
    /// \return Power of 10 corresponding to the given number of digits.
    /// \throws std::out_of_range If `digits` is not in the range [0, 18].
    template <typename T>
    inline const T pow10(size_t digits) {
        // Precomputed powers of 10 for digits from 0 to 18
        static const int64_t powers_of_ten[19] = {
            1LL,                 // 10^0
            10LL,                // 10^1
            100LL,               // 10^2
            1000LL,              // 10^3
            10000LL,             // 10^4
            100000LL,            // 10^5
            1000000LL,           // 10^6
            10000000LL,          // 10^7
            100000000LL,         // 10^8
            1000000000LL,        // 10^9
            10000000000LL,       // 10^10
            100000000000LL,      // 10^11
            1000000000000LL,     // 10^12
            10000000000000LL,    // 10^13
            100000000000000LL,   // 10^14
            1000000000000000LL,  // 10^15
            10000000000000000LL, // 10^16
            100000000000000000LL,// 10^17
            1000000000000000000LL// 10^18
        };

        if (digits > 18) throw std::out_of_range("Digits must be in the range 0-18.");
        return static_cast<T>(powers_of_ten[digits]);
    }

};

    /// \enum TickEncodingMode
    /// \brief Defines modes for handling `bid` and `ask` prices during encoding and decoding.
    enum class TickEncodingMode {

        /// \brief Only the `last` price is stored. `bid` and `ask` prices are not reconstructed.
        /// \details This mode minimizes data storage by excluding any computation or storage of `bid` and `ask` prices.
        LAST_ONLY           = 0,

        /// \brief `bid` and `ask` prices are generated using a fixed spread around the `last` price.
        /// \details In this mode, `ask` is calculated as `last + (spread / 2)`, and `bid` is calculated as `last - (spread / 2)`.
        /// The `spread` value is defined in the `TickEncodingConfig` structure in pips.
        FIXED_SPREAD        = 0x1,

        /// \brief Key points with known spreads are identified, and spreads are extrapolated forward.
        /// \details
        /// - Between key points, the spread is linearly extrapolated into the future until the next key point is reached.
        /// - Before the first key point, the spread is calculated as the average of the known spreads.
        /// - This mode is suitable when `ask` and `bid` prices are dynamically calculated, ensuring reasonable spread estimates.
        TRADE_BASED_FORWARD = 0x2,

        /// \brief Key points with known spreads are identified, and a mixed extrapolation approach is used.
        /// \details
        /// - For each period between two key points, the maximum spread between the two points is used.
        /// - Before the first key point and after the last key point:
        ///   - The spread is calculated as the maximum of the configured spread, the average spread, and the spread at the nearest key point.
        /// - This mode prioritizes conservative spread estimates and is suitable for ensuring reliable `ask` and `bid` reconstruction.
        TRADE_BASED_MIXED = 0x3
    };

    /// \struct TickEncodingConfig
    /// \brief Configuration for encoding and decoding tick sequences.
    struct TickEncodingConfig {
        size_t price_digits;             ///< Number of decimal places for prices.
        size_t volume_digits;            ///< Number of decimal places for volumes.
        size_t fixed_spread;             ///< Fixed spread in pips (used only in `FIXED_SPREAD` mode).
        TickEncodingMode encoding_mode;  ///< Mode for handling bid and ask prices during encoding/decoding.
        bool enable_trade_based_encoding;///< Optimize encoding for trade-based data where only `last` prices are available.
        bool enable_tick_flags;          ///< Enable processing of `TickUpdateFlags` during encoding and decoding.
        bool enable_received_time;       ///< Include the `received_time` field in the encoded data.

        /// \brief Default constructor for `TickEncodingConfig`.
        /// \details Initializes all fields with default values.
        TickEncodingConfig()
            : price_digits(2),              ///< Default to 2 decimal places for prices.
              volume_digits(0),             ///< Default to integer volume representation.
              fixed_spread(0),
              encoding_mode(TickEncodingMode::LAST_ONLY),
              enable_trade_based_encoding(false), ///< Default to standard tick encoding.
              enable_tick_flags(false),           ///< Default to not processing `TickUpdateFlags`.
              enable_received_time(false) {}      ///< Default to excluding `received_time`.

        /// \brief Constructor to initialize all fields explicitly.
        /// \param pd Number of decimal places for prices.
        /// \param vd Number of decimal places for volumes.
        /// \param mode Encoding mode to define how bid and ask prices are handled.
        /// \param spread Fixed spread in pips (only used in `FIXED_SPREAD` mode).
        /// \param trade_based Enable trade-based encoding.
        /// \param tick_flags Enable processing of `TickUpdateFlags`.
        /// \param received_time Include the `received_time` field in encoded data.
        TickEncodingConfig(
            int pd,
            int vd,
            TickEncodingMode mode,
            int spread,
            bool trade_based,
            bool tick_flags,
            bool received_time)
            : price_digits(pd),
              volume_digits(vd),
              fixed_spread(spread),
              encoding_mode(mode),
              enable_trade_based_encoding(trade_based),
              enable_tick_flags(tick_flags),
              enable_received_time(received_time) {}
    };

    /// \brief Flags indicating the status of tick data
    enum class TickStatusFlags : uint64_t {
        NONE = 0,               ///< No flags set
        REALTIME = 1 << 0,      ///< Data received in real-time
        INITIALIZED = 1 << 1    ///< Data has been initialized
    };

    /// \brief Flags describing updates in tick data
    enum class TickUpdateFlags : uint64_t {
        NONE = 0,               ///< No updates
        BID_UPDATED = 1 << 0,   ///< Bid price updated
        ASK_UPDATED = 1 << 1,   ///< Ask price updated
		LAST_UPDATED = 1 << 2,  ///< Last price updated
        VOLUME_UPDATED = 1 << 3,///< Volume updated
        TICK_FROM_BUY = 1 << 4, ///< Tick resulted from a buy trade
        TICK_FROM_SELL = 1 << 5 ///< Tick resulted from a sell trade
    };

    /// \brief Combines two UpdateFlags using bitwise OR
    /// \param a First flag
    /// \param b Second flag
    /// \return Combined flags
    inline TickUpdateFlags operator|(TickUpdateFlags a, TickUpdateFlags b) {
        return static_cast<TickUpdateFlags>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
    }

    /// \brief Checks if specific flags are set in a bitmask
    /// \param flags The bitmask of flags
    /// \param flag The flag to check
    /// \return True if the flag is set, false otherwise
    inline bool has_flag(uint64_t flags, TickUpdateFlags flag) {
        return (flags & static_cast<uint64_t>(flag)) != 0;
    }

	/// \brief Sets a specific flag in a bitmask.
	/// \param flags The bitmask of flags.
	/// \param flag The flag to set.
	inline void set_flag_in_place(uint64_t& flags, TickUpdateFlags flag) {
		flags = flags | static_cast<uint64_t>(flag);
	}

	/// \brief Sets a specific flag in a bitmask.
	/// \param flags The bitmask of flags.
	/// \param flag The flag to set.
	/// \return The updated bitmask with the specified flag set.
	inline uint64_t set_flag(uint64_t flags, TickUpdateFlags flag) {
		return flags | static_cast<uint64_t>(flag);
	}

	/// \brief Clears a specific flag in a bitmask.
	/// \param flags The bitmask of flags.
	/// \param flag The flag to clear.
	/// \return The updated bitmask with the specified flag cleared.
	inline uint64_t clear_flag(uint64_t flags, TickUpdateFlags flag) {
		return flags & ~static_cast<uint64_t>(flag);
	}

    /// \struct MarketTick
    /// \brief Represents a detailed market tick with flags
    struct MarketTick {
        double ask;              ///< Ask price
        double bid;              ///< Bid price
        double last;             ///< Price of last trade (Last)
        double volume;           ///< Trade volume (can store both whole units and high precision)
        uint64_t time_ms;        ///< Tick timestamp in milliseconds
        uint64_t received_ms;    ///< Time when tick was received from the server
        uint64_t flags;          ///< Flags representing tick characteristics (combination of TickUpdateFlags)

        /// \brief Default constructor that initializes all fields to zero or equivalent values
        MarketTick()
            : ask(0.0), bid(0.0), last(0.0),
              volume(0.0), time_ms(0),
              received_ms(0), flags(0) {}

        /// \brief Constructor to initialize all fields
        /// \param a Ask price
        /// \param b Bid price
        /// \param l Price of last trade (Last)
        /// \param v Trade volume in whole units
        /// \param vr Trade volume with high precision
        /// \param ts Tick timestamp in milliseconds
        /// \param rt Time when tick was received from the server
        /// \param f Flags representing tick characteristics
        MarketTick(double a, double b, double l, double v,
                   uint64_t ts, uint64_t rt, uint64_t f)
            : ask(a), bid(b), last(l), volume(v),
              time_ms(ts), received_ms(rt), flags(f) {}

        /// \brief Sets a specific flag in the tick's flags.
        /// \param flag The flag to set (from TickUpdateFlags).
        void set_flag(TickUpdateFlags flag) {
            flags |= static_cast<uint64_t>(flag);
        }

        void set_flag(TickUpdateFlags flag, bool value) {
            flags |= value ? static_cast<uint64_t>(flag) : 0x00;
        }

        /// \brief Checks if a specific flag is set in the tick's flags.
        /// \param flag The flag to check (from TickUpdateFlags).
        /// \return True if the flag is set, otherwise false.
        bool has_flag(TickUpdateFlags flag) const {
            return (flags & static_cast<uint64_t>(flag)) != 0;
        }
    };

    /// \brief Generic structure for a sequence of ticks with additional metadata
    template <typename TickType>
    struct TickSequence {
        std::vector<TickType> ticks; ///< Sequence of tick data of the specified type
        uint64_t flags;              ///< Tick data flags (bitmask of UpdateFlags)
        uint16_t symbol_index;       ///< Index of the symbol
        uint16_t provider_index;     ///< Index of the data provider
        uint16_t price_digits;       ///< Number of decimal places for price
        uint16_t volume_digits;      ///< Number of decimal places for volume

        TickSequence() :
            flags(0), symbol_index(0), provider_index(0),
            price_digits(0), volume_digits(0) {
        }

        /// \brief Constructor to initialize all fields
        /// \param ts Sequence of tick data
        /// \param f Tick data flags
        /// \param si Index of the symbol
        /// \param pi Index of the data provider
        /// \param d Number of decimal places for price
        /// \param vd Number of decimal places for volume
        TickSequence(std::vector<TickType> ts, uint64_t f, uint16_t si, uint16_t pi, uint16_t d, uint16_t vd)
            : ticks(std::move(ts)), flags(f),
            symbol_index(si), provider_index(pi),
            price_digits(d), volume_digits(vd) {
        }
    };

    // Tick sequence structures for specific tick types
    using MarketTickSequence = TickSequence<MarketTick>;

    /// \brief Reads the entire content of a file into a std::string.
    /// \param file_path Path to the input file.
    /// \return A std::string containing the file's content.
    /// \throws std::runtime_error If the file cannot be opened.
    std::string read_file_to_string(const std::string& file_path) {
        std::ifstream file(file_path, std::ios::binary | std::ios::ate);
        if (!file) {
            throw std::runtime_error("Failed to open file: " + file_path);
        }

        std::streamsize file_size = file.tellg();
        if (file_size <= 0) {
            return {};
        }

        std::string content(static_cast<size_t>(file_size), '\0');
        file.seekg(0, std::ios::beg);
        if (!file.read(&content[0], file_size)) {
            throw std::runtime_error("Failed to read file: " + file_path);
        }

        return content;
    }

    /// \brief Saves binary data to a file.
    /// \param file_path Path to the output file.
    /// \param binary_data The vector containing binary data to write.
    /// \throws std::runtime_error If the file cannot be opened.
    void save_binary_to_file(const std::string& file_path, const std::vector<uint8_t>& binary_data) {
        std::ofstream file(file_path, std::ios::binary | std::ios::out);
        if (!file) {
            throw std::runtime_error("Failed to open file: " + file_path);
        }

        file.write(reinterpret_cast<const char*>(binary_data.data()), binary_data.size());
        if (!file) {
            throw std::runtime_error("Failed to write to file: " + file_path);
        }
    }

    /// \brief Parses CSV trade data from Bybit and appends to a vector of MarketTick
    /// \param csv_data The CSV data as a string.
    /// \param ticks The vector to append parsed MarketTick structures.
    void parse_bybit_trades(const std::string& csv_data, MarketTickSequence& sequence, size_t reserve_size = 1000000) {
        std::istringstream csv_stream(csv_data);
        std::string line;

        sequence.ticks.reserve(reserve_size);
        if (!std::getline(csv_stream, line)) return; // Skip the header line
        while (std::getline(csv_stream, line)) {
            std::istringstream line_stream(line);
            std::string field;

            // Read fields
            double timestamp = 0.0;
            std::string symbol;
            std::string side;
            double size = 0.0;
            double price = 0.0;
            std::string tickDirection;
            std::string trdMatchID;
            double grossValue = 0.0;
            double homeNotional = 0.0;
            double foreignNotional = 0.0;

            try {
                std::getline(line_stream, field, ',');
                fast_double_parser::parse_number(field.c_str(), &timestamp);
                std::getline(line_stream, symbol, ',');
                std::getline(line_stream, side, ',');
                std::getline(line_stream, field, ',');
                fast_double_parser::parse_number(field.c_str(), &size);
                std::getline(line_stream, field, ',');
                fast_double_parser::parse_number(field.c_str(), &price);
                std::getline(line_stream, tickDirection, ',');
                std::getline(line_stream, trdMatchID, ',');
                std::getline(line_stream, field, ','); //grossValue = std::stod(field);
                std::getline(line_stream, field, ','); //homeNotional = std::stod(field);
                std::getline(line_stream, field, ','); //foreignNotional = std::stod(field);
            } catch (const std::exception& e) {
                throw std::runtime_error("Failed to parse line: " + line);
            }

            // Create MarketTick and set flags
            MarketTick tick;
            tick.last = price;
            tick.volume = size;
            tick.time_ms = time_shield::fsec_to_ms(timestamp);

            if (side == "Buy") {
                tick.set_flag(TickUpdateFlags::TICK_FROM_BUY);
            } else
            if (side == "Sell") {
                tick.set_flag(TickUpdateFlags::TICK_FROM_SELL);
            }

            // Handle tickDirection
            if (tickDirection == "PlusTick") {
                tick.set_flag(TickUpdateFlags::LAST_UPDATED);
            } else
            if (tickDirection == "MinusTick") {
                tick.set_flag(TickUpdateFlags::LAST_UPDATED);
            }

            sequence.ticks.push_back(tick);
        }
    }
}

namespace dfh {

    /// \brief Encodes an array of int32_t using Zig-Zag encoding with optional SIMD optimization.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    void zig_zag_encode_int32(const int32_t* input, uint32_t* output, size_t size) {
#       if defined(__AVX2__)
        constexpr size_t simd_width = 8; // AVX2 обрабатывает 8 int32 за раз
        const size_t aligned_size = size - (size % simd_width);

        for (size_t i = 0; i < aligned_size; i += simd_width) {
            __m256i vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&input[i]));
            _mm256_storeu_si256(
                reinterpret_cast<__m256i*>(&output[i]),
                _mm256_xor_si256(_mm256_slli_epi32(vec, 1), _mm256_srai_epi32(vec, 31)));
        }

        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = (input[i] << 1) ^ (input[i] >> 31);
        }
#       else
        for (size_t i = 0; i < size; ++i) {
            output[i] = (input[i] << 1) ^ (input[i] >> 31);
        }
#       endif
    }

    /// \brief Decodes an array of uint32_t from Zig-Zag encoding back to int32_t with optional SIMD optimization.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    void zig_zag_decode_int32(const uint32_t* input, int32_t* output, size_t size) {
#       if defined(__AVX2__)
        constexpr size_t simd_width = 8; // AVX2 обрабатывает 8 int32 за раз
        const size_t aligned_size = size - (size % simd_width);

        constexpr __m256i zero = _mm256_setzero_si256();
        constexpr __m256i one = _mm256_set1_epi32(1);

        for (size_t i = 0; i < aligned_size; i += simd_width) {
            __m256i vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&input[i]));
            _mm256_storeu_si256(
                reinterpret_cast<__m256i*>(&output[i]),
                _mm256_xor_si256(_mm256_srli_epi32(vec, 1), _mm256_sub_epi32(
                    zero, _mm256_and_si256(vec, one))));
        }

        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = (input[i] >> 1) ^ -(input[i] & 1);
        }
#       else
        for (size_t i = 0; i < size; ++i) {
            output[i] = (input[i] >> 1) ^ -(input[i] & 1);
        }
#       endif
    }

    void zig_zag_encode_int64(const int64_t* input, uint64_t* output, size_t size) {
#       if defined(__AVX2__)
        constexpr size_t simd_width = 4; // 4 int64 за один SIMD-вектор
        const size_t aligned_size = size - (size % simd_width);

        for (size_t i = 0; i < aligned_size; i += simd_width) {
            __m256i vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&input[i]));
            _mm256_storeu_si256(
                reinterpret_cast<__m256i*>(&output[i]),
                _mm256_xor_si256(_mm256_slli_epi64(vec, 1), _mm256_srai_epi64(vec, 63)));
        }

        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = (input[i] << 1) ^ (input[i] >> 63);
        }
#       else
        for (size_t i = 0; i < size; ++i) {
            output[i] = (input[i] << 1) ^ (input[i] >> 63);
        }
#       endif
    }

    void zig_zag_decode_int64(const uint64_t* input, int64_t* output, size_t size) {
#       if defined(__AVX2__)
        constexpr size_t simd_width = 4; // 4 int64 за один SIMD-вектор
        const size_t aligned_size = size - (size % simd_width);

        constexpr __m256i zero = _mm256_setzero_si256();
        constexpr __m256i one = _mm256_set1_epi64x(1);

        for (size_t i = 0; i < aligned_size; i += simd_width) {
            __m256i vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&input[i]));
            // Преобразование: (n >> 1) ^ -(n & 1)
            __m256i decoded = _mm256_xor_si256(_mm256_srli_epi64(vec, 1), _mm256_sub_epi64(_mm256_setzero_si256(), _mm256_and_si256(vec, _mm256_set1_epi64x(1))));
            _mm256_storeu_si256(
                reinterpret_cast<__m256i*>(&output[i]),
                _mm256_xor_si256(
                    _mm256_srli_epi64(vec, 1),
                    _mm256_sub_epi64(zero, _mm256_and_si256(vec, one))));
        }

        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = (input[i] >> 1) ^ -(input[i] & 1);
        }
#       else
        for (size_t i = 0; i < size; ++i) {
            output[i] = (input[i] >> 1) ^ -(input[i] & 1);
        }
#       endif
    }

    /// \brief Checks if a delta array can fit into int32_t without overflow.
    /// \param input Pointer to the input array.
    /// \param size Number of elements in the array.
    /// \return True if all deltas can fit into int32_t, false otherwise.
    bool check_deltas_fit_int32(const int32_t* input, size_t size) {
        for (size_t i = 1; i < size; ++i) {
            if ((static_cast<int64_t>(input[i]) - static_cast<int64_t>(input[i - 1])) > INT32_MAX ||
                (static_cast<int64_t>(input[i]) - static_cast<int64_t>(input[i - 1])) < INT32_MIN) {
                return false;
            }
        }
        return true;
    }

    bool check_deltas_fit_int32(const int32_t* input, size_t size, int32_t initial_value) {
        if ((static_cast<int64_t>(input[0]) - static_cast<int64_t>(initial_value)) > INT32_MAX ||
            (static_cast<int64_t>(input[0]) - static_cast<int64_t>(initial_value)) < INT32_MIN) {
            return false;
        }
        for (size_t i = 1; i < size; ++i) {
            if ((static_cast<int64_t>(input[i]) - static_cast<int64_t>(input[i - 1])) > INT32_MAX ||
                (static_cast<int64_t>(input[i]) - static_cast<int64_t>(input[i - 1])) < INT32_MIN) {
                return false;
            }
        }
        return true;
    }

    /// \brief Performs delta and Zig-Zag encoding in a single pass.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    /// \param initial_value
    void delta_zig_zag_encode_int32(const int32_t* input, uint32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;

        int32_t delta = input[0] - initial_value;
        output[0] = (static_cast<uint32_t>(delta) << 1) ^ static_cast<uint32_t>(delta >> 31);

#       if defined(__AVX2__)
        constexpr size_t simd_width = 8; // AVX2 обрабатывает 8 int32 за раз
        const size_t aligned_size = size - (size % simd_width);

        // Первый элемент обработан отдельно, начинаем с i = 1
        for (size_t i = 1; i < aligned_size; i += simd_width) {
            __m256i delta = _mm256_sub_epi32(
                _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&input[i])),
                _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&input[i-1])));
            _mm256_storeu_si256(
                reinterpret_cast<__m256i*>(&output[i]),
                _mm256_xor_si256(_mm256_slli_epi32(delta, 1),
                    _mm256_srai_epi32(delta, 31)));
        }

        for (size_t i = aligned_size; i < size; ++i) {
            int32_t delta = input[i] - input[i - 1];
            output[i] = (static_cast<uint32_t>(delta) << 1) ^ static_cast<uint32_t>(delta >> 31);
        }
#       else
        for (size_t i = 1; i < size; ++i) {
            int32_t delta = input[i] - input[i - 1];
            output[i] = (delta << 1) ^ (delta >> 31);
        }
#       endif
    }

    /// \brief Performs delta and Zig-Zag decoding in a single pass.
    /// \param input Pointer to the encoded array.
    /// \param output Pointer to the decoded array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for reconstruction.
    void delta_zig_zag_decode_int32(const uint32_t* input, int32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;
#       if defined(__AVX2__)
        constexpr size_t simd_width = 8; // AVX2 обрабатывает 8 int32 за раз
        const size_t aligned_size = size - (size % simd_width);

        constexpr __m256i zero = _mm256_setzero_si256();
        constexpr __m256i one = _mm256_set1_epi32(1);

        for (size_t i = 0; i < aligned_size; i += simd_width) {
            __m256i vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&input[i]));
            _mm256_storeu_si256(
                reinterpret_cast<__m256i*>(&output[i]),
                _mm256_xor_si256(_mm256_srli_epi32(vec, 1), _mm256_sub_epi32(
                    zero, _mm256_and_si256(vec, one))));
        }

        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = (input[i] >> 1) ^ -(input[i] & 1);
        }

        output[0] = initial_value + output[0];
        for (size_t i = 1; i < size; ++i) {
            output[i] += output[i - 1];
        }
#       else
        int32_t zigzag = (input[0] >> 1) ^ -(input[0] & 1);
        output[0] = initial_value + zigzag;
        for (size_t i = 1; i < size; ++i) {
            zigzag = (input[i] >> 1) ^ -(input[i] & 1);
            output[i] = output[i - 1] + zigzag;
        }
#       endif
    }

    /// \brief Performs delta and Zig-Zag encoding in a single pass.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for delta computation.
    void delta_zig_zag_encode_chunked_int32(const int32_t* input, uint32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 8; // AVX2 обрабатывает 8 int32 за раз
        const size_t aligned_size = size - (size % simd_width);

        int32_t prev = initial_value;

#       if defined(__AVX2__)
        // Обработка SIMD блоков
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            __m256i delta = _mm256_sub_epi32(
                _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&input[i])),
                _mm256_set1_epi32(prev));
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&output[i]),
                _mm256_xor_si256(_mm256_slli_epi32(delta, 1), _mm256_srai_epi32(delta, 31)));
            prev = input[i + simd_width - 1];
        }

        // Обработка оставшихся элементов
        int32_t delta;
        for (size_t i = aligned_size; i < size; ++i) {
            delta = input[i] - prev;
            output[i] = (delta << 1) ^ (delta >> 31);
        }
#       else
        int32_t delta;
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                delta = input[j] - prev;
                output[j] = (delta << 1) ^ (delta >> 31);
            }
            prev = input[j_max - 1];
        }
        for (size_t i = aligned_size; i < size; ++i) {
            delta = input[i] - prev;
            output[i] = (delta << 1) ^ (delta >> 31);
        }
#       endif
    }

    /// \brief Performs delta and Zig-Zag decoding in a single pass.
    /// \param input Pointer to the encoded array.
    /// \param output Pointer to the decoded array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for reconstruction.
    void delta_zig_zag_decode_chunked_int32(const uint32_t* input, int32_t* output, size_t size, int32_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 8; // AVX2 обрабатывает 8 int32 за раз
        const size_t aligned_size = size - (size % simd_width);

        int32_t prev = initial_value;

#       if defined(__AVX2__)
        // Константы SIMD
        constexpr __m256i zero = _mm256_setzero_si256();
        constexpr __m256i one = _mm256_set1_epi32(1);

        // Обработка SIMD блоков
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            __m256i encoded = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&input[i]));
            _mm256_storeu_si256(
                reinterpret_cast<__m256i*>(&output[i]),
                _mm256_add_epi32(
                    _mm256_xor_si256(_mm256_srli_epi32(encoded, 1), _mm256_sub_epi32(zero, _mm256_and_si256(encoded, one))),
                    _mm256_set1_epi32(prev)));
            prev = output[i + simd_width - 1];
        }

        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = prev + ((input[i] >> 1) ^ -(input[i] & 1));
        }
#       else
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                output[j] = prev + ((input[j] >> 1) ^ -(input[j] & 1));
            }
            prev = output[j_max - 1];
        }
        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = prev + ((input[i] >> 1) ^ -(input[i] & 1));
        }
#       endif
    }

    /// \brief Performs delta and Zig-Zag encoding in a single pass for 64-bit integers.
    /// \param input Pointer to the input array.
    /// \param output Pointer to the output array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for delta computation.
    void delta_zig_zag_encode_chunked_int64(const int64_t* input, uint64_t* output, size_t size, int64_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 4; // AVX2 обрабатывает 4 int64 за раз
        const size_t aligned_size = size - (size % simd_width);

        int64_t prev = initial_value;

#       if defined(__AVX2__)
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            __m256i delta = _mm256_sub_epi64(
                _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&input[i])),
                _mm256_set1_epi64x(prev));
            _mm256_storeu_si256(reinterpret_cast<__m256i*>(&output[i]),
                _mm256_xor_si256(_mm256_slli_epi64(delta, 1), _mm256_srai_epi64(delta, 63)));
            prev = input[i + simd_width - 1];
        }

        int64_t delta;
        for (size_t i = aligned_size; i < size; ++i) {
            delta = input[i] - prev;
            output[i] = (delta << 1) ^ (delta >> 63);
        }
#       else
        int64_t delta;
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                delta = input[j] - prev;
                output[j] = (delta << 1) ^ (delta >> 63);
            }
            prev = input[j_max - 1];
        }
        for (size_t i = aligned_size; i < size; ++i) {
            delta = input[i] - prev;
            output[i] = (delta << 1) ^ (delta >> 63);
        }
#       endif
    }

    /// \brief Performs delta and Zig-Zag decoding in a single pass for 64-bit integers.
    /// \param input Pointer to the encoded array.
    /// \param output Pointer to the decoded array.
    /// \param size Number of elements in the array.
    /// \param initial_value Initial reference value for reconstruction.
    void delta_zig_zag_decode_chunked_int64(const uint64_t* input, int64_t* output, size_t size, int64_t initial_value) {
        if (size == 0) return;

        constexpr size_t simd_width = 4; // AVX2 обрабатывает 4 int64 за раз
        const size_t aligned_size = size - (size % simd_width);

        int64_t prev = initial_value;

#       if defined(__AVX2__)
        const __m256i zero = _mm256_setzero_si256();
        const __m256i one = _mm256_set1_epi64x(1);

        for (size_t i = 0; i < aligned_size; i += simd_width) {
            __m256i encoded = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&input[i]));
            _mm256_storeu_si256(
                reinterpret_cast<__m256i*>(&output[i]),
                _mm256_add_epi64(
                    _mm256_xor_si256(_mm256_srli_epi64(encoded, 1), _mm256_sub_epi64(zero, _mm256_and_si256(encoded, one))),
                    _mm256_set1_epi64x(prev)));
            prev = output[i + simd_width - 1];
        }

        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = prev + ((input[i] >> 1) ^ -(input[i] & 1));
        }
#       else
        for (size_t i = 0; i < aligned_size; i += simd_width) {
            size_t j_max = i + simd_width;
            for (size_t j = i; j < j_max; ++j) {
                output[j] = prev + ((input[j] >> 1) ^ -(input[j] & 1));
            }
            prev = output[j_max - 1];
        }
        for (size_t i = aligned_size; i < size; ++i) {
            output[i] = prev + ((input[i] >> 1) ^ -(input[i] & 1));
        }
#       endif
    }

}

// Test 1: Full-featured test with flags, real volume, and received_ms
void test() {
    dfh::TickEncodingConfig config;

    dfh::MarketTickSequence sequence;
    sequence.price_digits = 2;
    sequence.volume_digits = 3;

    auto csv_data = dfh::read_file_to_string("BTCUSDT2024-10-01.csv");
    dfh::parse_bybit_trades(csv_data, sequence, 3000000);

    config.price_digits  = sequence.price_digits;
    config.volume_digits = sequence.volume_digits;
    config.enable_trade_based_encoding = true;
    config.enable_tick_flags           = true;
    config.enable_received_time        = false;
    config.fixed_spread = 1;

    double price_scale = dfh::utils::pow10<double>(config.price_digits);
    double volume_scale = dfh::utils::pow10<double>(config.volume_digits);

    Eigen::VectorXd prices;
    prices.conservativeResize(sequence.ticks.size());

    for (size_t i = 0; i < sequence.ticks.size(); ++i) {
        prices[i] = sequence.ticks[i].last;
    }

    prices = (prices.array() * price_scale).round();
    Eigen::VectorXd buffer;
    buffer.conservativeResize(sequence.ticks.size());
    buffer.tail(prices.size()-1) = prices.tail(prices.size()-1) - prices.head(prices.size()-1);
    buffer[0] = 0.0;

    Eigen::Matrix<int32_t, Eigen::Dynamic, 1> src32 = buffer.cast<int32_t>();
    if (dfh::check_deltas_fit_int32(src32.data(), src32.size())) {
        std::cout << "fit_int32 ok" << std::endl;
    }

    Eigen::Matrix<int64_t, Eigen::Dynamic, 1> src = buffer.cast<int64_t>();
    Eigen::Matrix<int64_t, Eigen::Dynamic, 1> int_deltas = src;
    Eigen::Matrix<int64_t, Eigen::Dynamic, 1> int_result;
    int_result.conservativeResize(sequence.ticks.size());

    int64_t base = int_deltas[0];
    dfh::zig_zag_encode_int64(src.data(), reinterpret_cast<uint64_t*>(int_result.data()), src.size());
    //dfh::encode_delta_zigzag(int_deltas, int_result, base);

    Eigen::Matrix<int64_t, Eigen::Dynamic, 1> dst;
    dst.conservativeResize(sequence.ticks.size());
    dfh::zig_zag_decode_int64(reinterpret_cast<const uint64_t*>(int_result.data()), dst.data(), src.size());
    //dfh::decode_delta_zigzag(int_result, dst, base);

    for (size_t i = 0; i < sequence.ticks.size(); ++i) {
        if (src[i] != dst[i]) {
            std::cout << "err" << std::endl;
            break;
        }
    }
    std::cout << "ok" << std::endl;
}

// Main function to run all tests
int main() {
    test();
    return 0;
}
