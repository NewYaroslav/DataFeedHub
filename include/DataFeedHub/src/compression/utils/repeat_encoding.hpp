#pragma once
#ifndef _DFH_COMPRESSION_UTILS_REPEAT_ENCODING_HPP_INCLUDED
#define _DFH_COMPRESSION_UTILS_REPEAT_ENCODING_HPP_INCLUDED

/// \file repeat_encoding.hpp
/// \brief

namespace dfh::compression {

    /// \brief Encodes an array of values with run-length compression for repeated
    ///        values. Uses the least significant bit (LSB) as a flag to distinguish
    ///        between compressed (with repeat count) and uncompressed values.
    /// \param input Pointer to the input array of values.
    /// \param input_size The number of elements in the input array.
    /// \param bits The number of bits used to represent the maximum value of a
    ///             single element (excluding the repeat count). Maximum value
    ///             calculated as `( (1 << bits) - 1)`.
    /// \param output Pointer to the output array for compressed data.
    /// \param output_size Reference to a variable where the size of the compressed
    ///                    array will be stored.
    void encode_with_repeats(
            const uint32_t* input,
            size_t input_size,
            size_t bits,
            uint32_t* output,
            size_t& output_size) {
        uint32_t max_volume = (1UL << bits) - 1UL; // Maximum representable value
        size_t value_count_bits = bits + 1;       // Number of bits to encode value and flag

        uint32_t repeated_value = input[0];
        uint32_t repeat_count = 0;

        size_t offset = 0;

        for (size_t i = 0; i < input_size; ++i) {
            if (input[i] <= max_volume && input[i] == repeated_value) {
                ++repeat_count;
                continue;
            }

            if (repeat_count > 0) {
                // Compress the repeated value with the repeat count
                uint32_t compressed = ((repeated_value << 1) | 1UL) | (repeat_count << value_count_bits);
                output[offset++] = compressed;
                repeat_count = 0;
            }

            // Store the new value uncompressed
            repeated_value = input[i];
            output[offset++] = repeated_value << 1;
        }

        // Handle the last repeated value
        if (repeat_count > 0) {
            uint32_t compressed = ((repeated_value << 1) | 1UL) | (repeat_count << value_count_bits);
            output[offset++] = compressed;
        }

        output_size = offset; // Store the size of the compressed data
    }

    void decode_with_repeats(
            const uint32_t* encoded,
            size_t encoded_size,
            size_t bits,
            uint32_t* decoded,
            size_t& decoded_size) {
        uint32_t value_mask = (1UL << bits) - 1UL;
        size_t value_count_bits = bits + 1;

        size_t output_index = 0;

        for (size_t i = 0; i < encoded_size; ++i) {
            uint32_t value = encoded[i];

            if (value & 0x1) { // If LSB is set, it's a compressed value
                uint32_t repeated_value = (value >> 1) & value_mask; // Extract value
                uint32_t repeat_count = value >> value_count_bits;   // Extract repeat count

                // Fill the repeated values
                for (uint32_t j = 0; j < repeat_count; ++j) {
                    decoded[output_index++] = repeated_value;
                }
            } else {
                // Extract value and store it
                decoded[output_index++] = value >> 1;
            }
        }

        decoded_size = output_index; // Store the size of the decoded data
    }

//------------------------------------------------------------------------------

    inline void encode_zero_with_repeats(
            const uint32_t* input,
            size_t input_size,
            uint32_t* output,
            size_t& output_size) {
        uint32_t repeated_value = input[0];
        uint32_t repeat_count = 0;
        output_size = 0;
        for (size_t i = 0; i < input_size; ++i) {
            if (input[i] == 0 && input[i] == repeated_value) {
                ++repeat_count;
                continue;
            }

            if (repeat_count > 0) {
                output[output_size++] = ((repeat_count << 1) | 1UL);
                repeat_count = 0;
            }

            repeated_value = input[i];
            output[output_size++] = repeated_value << 1;
        }

        if (repeat_count > 0) {
            output[output_size++] = ((repeat_count << 1) | 1UL);
        }
    }

    inline void decode_zero_with_repeats(
            const uint32_t* encoded,
            size_t encoded_size,
            uint32_t* decoded,
            size_t& decoded_size) {
        uint32_t repeat_count, value, j;
        decoded_size = 0;
        for (size_t i = 0; i < encoded_size; ++i) {
            value = encoded[i];
            if (value & 0x1) {
                repeat_count = value >> 1;
                for (j = 0; j < repeat_count; ++j) {
                    decoded[decoded_size++] = 0;
                }
            } else {
                decoded[decoded_size++] = value >> 1;
            }
        }
    }

//------------------------------------------------------------------------------

    void encode_with_repeats(
            const uint64_t* input,
            size_t input_size,
            size_t bits,
            uint64_t* output,
            size_t& output_size) {
        uint64_t max_volume = (1ULL << bits) - 1ULL; // Maximum representable value
        size_t value_count_bits = bits + 1;       // Number of bits to encode value and flag

        uint64_t repeated_value = input[0];
        uint64_t repeat_count = 0;

        size_t offset = 0;

        for (size_t i = 0; i < input_size; ++i) {
            if (input[i] <= max_volume && input[i] == repeated_value) {
                ++repeat_count;
                continue;
            }

            if (repeat_count > 0) {
                // Compress the repeated value with the repeat count
                output[offset++] = ((repeated_value << 1) | 1UL) | (repeat_count << value_count_bits);
                repeat_count = 0;
            }

            // Store the new value uncompressed
            repeated_value = input[i];
            output[offset++] = repeated_value << 1;
        }

        // Handle the last repeated value
        if (repeat_count > 0) {
            output[offset++] = ((repeated_value << 1) | 1UL) | (repeat_count << value_count_bits);
        }

        output_size = offset; // Store the size of the compressed data
    }

    void decode_with_repeats(
            const uint64_t* encoded,
            size_t encoded_size,
            size_t bits,
            uint64_t* decoded,
            size_t& decoded_size) {
        uint64_t value_mask = (1ULL << bits) - 1ULL;
        size_t value_count_bits = bits + 1;

        size_t output_index = 0;

        for (size_t i = 0; i < encoded_size; ++i) {
            uint64_t value = encoded[i];

            if (value & 0x1) { // If LSB is set, it's a compressed value
                uint64_t repeated_value = (value >> 1) & value_mask; // Extract value
                uint64_t repeat_count = value >> value_count_bits;   // Extract repeat count

                // Fill the repeated values
                for (uint64_t j = 0; j < repeat_count; ++j) {
                    decoded[output_index++] = repeated_value;
                }
            } else {
                // Extract value and store it
                decoded[output_index++] = value >> 1;
            }
        }

        decoded_size = output_index; // Store the size of the decoded data
    }

//------------------------------------------------------------------------------

    inline void encode_run_length(
            const uint64_t* input,
            size_t input_size,
            uint64_t* output,
            size_t& output_size) {
        uint64_t repeated_value = input[0];
        uint64_t repeat_count = 0;
        output_size = 0;
        for (size_t i = 0; i < input_size; ++i) {
            if (input[i] == repeated_value) {
                ++repeat_count;
                continue;
            }

            if (repeat_count >= 3) {
                output[output_size++] = ((repeated_value << 1) | 1UL);
                output[output_size++] = repeat_count;
                repeat_count = 0;
            } else
            while (repeat_count > 0) {
                output[output_size++] = repeated_value << 1;
                --repeat_count;
            }

            repeated_value = input[i];
            output[output_size++] = repeated_value << 1;
        }

        if (repeat_count >= 3) {
            output[output_size++] = ((repeated_value << 1) | 1UL);
            output[output_size++] = repeat_count;
        } else
        while (repeat_count > 0) {
            output[output_size++] = repeated_value << 1;
            --repeat_count;
        }
    }

    inline void encode_run_length(
            const uint32_t* input,
            size_t input_size,
            uint32_t* output,
            size_t& output_size) {
        uint32_t repeated_value = input[0];
        uint32_t repeat_count = 0;
        output_size = 0;
        for (size_t i = 0; i < input_size; ++i) {
            if (input[i] == repeated_value) {
                ++repeat_count;
                continue;
            }

            if (repeat_count >= 4) {
                output[output_size++] = ((repeated_value << 1) | 1UL);
                output[output_size++] = repeat_count;
                repeat_count = 0;
            } else
            while (repeat_count > 0) {
                output[output_size++] = repeated_value << 1;
                --repeat_count;
            }

            repeated_value = input[i];
            output[output_size++] = repeated_value << 1;
        }

        if (repeat_count >= 4) {
            output[output_size++] = ((repeated_value << 1) | 1UL);
            output[output_size++] = repeat_count;
        } else
        while (repeat_count > 0) {
            output[output_size++] = repeated_value << 1;
            --repeat_count;
        }
    }

    inline void decode_run_length(
            const uint32_t* encoded,
            size_t encoded_size,
            uint32_t* decoded,
            size_t& decoded_size) {
        uint32_t repeat_count, value, j;
        decoded_size = 0;
        for (size_t i = 0; i < encoded_size; ++i) {
            value = encoded[i];
            if (value & 0x1) {
                value >>= 1; ++i;
                repeat_count = encoded[i];
                for (j = 0; j < repeat_count; ++j) {
                    decoded[decoded_size++] = value;
                }
            } else {
                decoded[decoded_size++] = value >> 1;
            }
        }
    }

};

#endif // _DFH_COMPRESSION_UTILS_REPEAT_ENCODING_HPP_INCLUDED
