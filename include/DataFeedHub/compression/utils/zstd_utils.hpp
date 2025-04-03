#pragma once
#ifndef _DFH_COMPRESSION_UTILS_ZSTD_UTILS_HPP_INCLUDED
#define _DFH_COMPRESSION_UTILS_ZSTD_UTILS_HPP_INCLUDED

/// \file zstd_utils.hpp
/// \brief Compression and decompression utilities based on Zstandard (ZSTD), including dictionary training, metadata embedding, and C++ header export.

namespace dfh::compression {

    /// \brief Compresses binary data using ZSTD and a dictionary.
    /// \param input Pointer to the input binary data.
    /// \param input_size Size of the input binary data.
    /// \param dictionary Pointer to the dictionary data.
    /// \param dictionary_size Size of the dictionary.
    /// \param output Reference to a vector for storing compressed data.
    /// \param compress_level Compression level (default: ZSTD_maxCLevel()).
    /// \throw std::runtime_error if compression fails.
    /// \throw std::invalid_argument if input, dictionary, or sizes are invalid.
    void compress_zstd_data(
            const void* input,
            size_t input_size,
            const void* dictionary,
            size_t dictionary_size,
            std::vector<uint8_t>& output,
            int compress_level = ZSTD_maxCLevel()) {
        if (!input || input_size == 0 || !dictionary || dictionary_size == 0) {
            throw std::invalid_argument("Invalid input or dictionary data.");
        }

        size_t offset = output.size();
        size_t max_compressed_size = ZSTD_compressBound(input_size);
        output.resize(offset + max_compressed_size);

        ZSTD_CCtx* cctx = ZSTD_createCCtx();
        if (!cctx) {
            throw std::runtime_error("Failed to create ZSTD compression context.");
        }

        size_t compressed_size = ZSTD_compress_usingDict(
            cctx,
            output.data() + offset,
            max_compressed_size,
            input,
            input_size,
            dictionary,
            dictionary_size,
            compress_level
        );

        ZSTD_freeCCtx(cctx);

        if (ZSTD_isError(compressed_size)) {
            throw std::runtime_error(std::string("Compression error: ") + ZSTD_getErrorName(compressed_size));
        }

        output.resize(compressed_size + offset);
    }

    /// \brief Compresses binary data using ZSTD and a dictionary.
    /// \param input Pointer to the input binary data.
    /// \param input_size Size of the input binary data.
    /// \param dictionary Pointer to the dictionary data.
    /// \param dictionary_size Size of the dictionary.
    /// \param signature Unique signature for the compressed format.
    /// \param num_samples Number of elements in the input data (e.g., ticks or bars).
    ///                    Stored before the compressed data to allow easy retrieval without decompression.
    /// \param output Reference to a vector for storing compressed data.
    /// \param compress_level Compression level (default: ZSTD_maxCLevel()).
    /// \throw std::runtime_error if compression fails.
    /// \throw std::invalid_argument if input, dictionary, or sizes are invalid.
    void compress_zstd_data(
            const void* input,
            size_t input_size,
            const void* dictionary,
            size_t dictionary_size,
            uint8_t signature,
            uint32_t num_samples,
            std::vector<uint8_t>& output,
            int compress_level = ZSTD_maxCLevel()) {
        if (!input || input_size == 0 || !dictionary || dictionary_size == 0) {
            throw std::invalid_argument("Invalid input or dictionary data.");
        }

        const size_t max_compressed_size = ZSTD_compressBound(input_size);
        output.reserve(max_compressed_size + 5);
        output.push_back(signature);
        dfh::utils::append_vbyte<uint32_t>(output, num_samples);
        const size_t initial_size = output.size();
        output.resize(max_compressed_size + initial_size);

        ZSTD_CCtx* cctx = ZSTD_createCCtx();
        if (!cctx) {
            throw std::runtime_error("Failed to create ZSTD compression context.");
        }

        size_t compressed_size = ZSTD_compress_usingDict(
            cctx,
            output.data() + initial_size,
            max_compressed_size,
            input,
            input_size,
            dictionary,
            dictionary_size,
            compress_level
        );

        ZSTD_freeCCtx(cctx);

        if (ZSTD_isError(compressed_size)) {
            throw std::runtime_error(std::string("Compression error: ") + ZSTD_getErrorName(compressed_size));
        }

        output.resize(compressed_size + initial_size);
    }

    /// \brief Decompresses binary data using ZSTD and a dictionary.
    /// \param input Pointer to the compressed binary data.
    /// \param input_size Size of the compressed binary data.
    /// \param dictionary Pointer to the dictionary data.
    /// \param dictionary_size Size of the dictionary.
    /// \param output Reference to a vector for storing decompressed data.
    /// \throw std::invalid_argument if input, dictionary, or sizes are invalid.
    /// \throw std::runtime_error if decompression fails.
    void decompress_zstd_data(
            const void* input,
            size_t input_size,
            const void* dictionary,
            size_t dictionary_size,
            std::vector<uint8_t>& output) {
        if (!input || input_size == 0 || !dictionary || dictionary_size == 0) {
            throw std::invalid_argument("Invalid input or dictionary data.");
        }

        unsigned long long decompressed_size = ZSTD_getFrameContentSize(input, input_size);
        if (decompressed_size == ZSTD_CONTENTSIZE_ERROR) {
            throw std::runtime_error("Input was not compressed by ZSTD.");
        }
        if (decompressed_size == ZSTD_CONTENTSIZE_UNKNOWN) {
            throw std::runtime_error("Original size is unknown.");
        }

        output.resize(decompressed_size);

        ZSTD_DCtx* dctx = ZSTD_createDCtx();
        if (!dctx) {
            throw std::runtime_error("Failed to create ZSTD decompression context.");
        }

        size_t result_size = ZSTD_decompress_usingDict(
            dctx,
            output.data(),
            decompressed_size,
            input,
            input_size,
            dictionary,
            dictionary_size
        );

        ZSTD_freeDCtx(dctx);

        if (ZSTD_isError(result_size)) {
            throw std::runtime_error(std::string("Decompression error: ") + ZSTD_getErrorName(result_size));
        }

        output.resize(result_size);
    }

    /// \brief Extracts signature byte from compressed data.
    /// \param data Pointer to compressed data.
    /// \param size Size of the compressed data.
    /// \return Signature byte.
    /// \throw std::invalid_argument If data is too small.
    inline uint8_t extract_signature(const uint8_t* data, size_t size) {
        if (!data || size < 1) {
            throw std::invalid_argument("Data is too small to contain a signature.");
        }
        return data[0];
    }

    /// \brief Extracts number of samples from compressed data encoded as VByte after signature.
    /// \param data Pointer to compressed data.
    /// \param size Size of the compressed data.
    /// \return Number of samples.
    /// \throw std::invalid_argument If VByte is malformed or exceeds buffer.
    inline uint32_t extract_num_samples(const uint8_t* data, size_t size) {
        if (!data || size < 2) {
            throw std::invalid_argument("Data is too small to contain a VByte.");
        }
        size_t offset = 1; // skip signature
        return dfh::utils::extract_vbyte<uint32_t>(data, offset);
    }

    /// \brief Trains a ZSTD dictionary from binary samples and returns it as a binary array.
    /// \param samples A vector of pairs, where each pair contains a pointer to binary data and its size.
    /// \param dict_buffer_capacity The maximum size of the dictionary in bytes (default: 102400).
    /// \return A vector of uint8_t containing the dictionary binary data.
    /// \throw std::invalid_argument If the samples vector is empty.
    /// \throw std::runtime_error If dictionary training fails.
    std::vector<uint8_t> train_zstd(
            const std::vector<std::vector<uint8_t>>& samples,
            size_t dict_buffer_capacity = 102400) {
        // Validate inputs
        if (samples.empty()) {
            throw std::invalid_argument("Samples cannot be empty.");
        }

        size_t total_samples_size = 0;
        for (const auto& sample : samples) {
            if (sample.empty()) {
                throw std::invalid_argument("Invalid sample data or size.");
            }
            total_samples_size += sample.size();
        }

        // Allocate memory for concatenated samples
        std::vector<uint8_t> concatenated_samples(total_samples_size);
        size_t offset = 0;
        std::vector<size_t> sample_sizes(samples.size());

        for (size_t i = 0; i < samples.size(); ++i) {
            std::memcpy(concatenated_samples.data() + offset, samples[i].data(), samples[i].size());
            sample_sizes[i] = samples[i].size();
            offset += samples[i].size();
        }

        // Allocate memory for the dictionary
        std::vector<uint8_t> dict_buffer(dict_buffer_capacity);

        // Train the dictionary
        size_t dict_size = ZDICT_trainFromBuffer(
            dict_buffer.data(),
            dict_buffer_capacity,
            concatenated_samples.data(),
            sample_sizes.data(),
            samples.size());

        if (ZDICT_isError(dict_size)) {
            throw std::runtime_error(std::string("Dictionary training failed: ") + ZDICT_getErrorName(dict_size));
        }

        // Resize the dictionary buffer to the actual size
        dict_buffer.resize(dict_size);

        return dict_buffer;
    }

    /// \brief Converts binary data to a C++ header file.
    /// \param binary_data The binary data to save.
    /// \param name The name of the array in the generated header file.
    /// \param header_path The path where the header file will be saved.
    /// \throws std::runtime_error If the file cannot be written.
    void save_binary_as_header(
            const std::vector<uint8_t>& binary_data,
            const std::string& name,
            const std::string& header_path) {
        if (binary_data.empty()) {
            throw std::invalid_argument("Binary data cannot be empty.");
        }
        if (name.empty()) {
            throw std::invalid_argument("Name cannot be empty.");
        }
        if (header_path.empty()) {
            throw std::invalid_argument("Header path cannot be empty.");
        }

        std::string name_upper = utils::to_upper_case(name);
        std::string name_lower = utils::to_lower_case(name);

        std::ostringstream header_content;
        header_content << "#ifndef " << name_upper << "_HPP_INCLUDED\n"
                       << "#define " << name_upper << "_HPP_INCLUDED\n\n"
                       << "namespace binary_data {\n"
                       << "\tconst static unsigned char " << name_lower << "[" << binary_data.size() << "] = {\n\t\t";

        // Write binary data as hex values
        for (size_t i = 0; i < binary_data.size(); ++i) {
            if (i > 0 && (i % 16) == 0) {
                header_content << "\n\t\t";
            }
            header_content << utils::convert_hex_to_string(binary_data[i]) << ", ";
        }

        header_content << "\n\t};\n}\n\n#endif // " << name_upper << "_HPP_INCLUDED\n";

        // Write header content to file
        std::ofstream header_file(header_path, std::ios::out);
        if (!header_file.is_open()) {
            throw std::runtime_error("Failed to open header file: " + header_path);
        }

        header_file << header_content.str();
        header_file.close();

        if (!header_file) {
            throw std::runtime_error("Failed to write header file: " + header_path);
        }
    }

}

#endif // _DFH_COMPRESSION_UTILS_ZSTD_UTILS_HPP_INCLUDED
