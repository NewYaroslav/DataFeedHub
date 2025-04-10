#pragma once
#ifndef _DFH_UTILS_ZIP_UTILS_HPP_INCLUDED
#define _DFH_UTILS_ZIP_UTILS_HPP_INCLUDED

/// \file zip_utils.hpp
/// \brief Utilities for extracting ZIP archives directly from memory.

#include <mz.h>
#include <mz_strm.h>
#include <mz_strm_buf.h>
#include <mz_strm_mem.h>
#include <mz_strm_split.h>
#include <mz_zip.h>
#include <mz_zip_rw.h>
//#define GZIP_USING_ZLIB_NG
#include <gzip/decompress.hpp>

namespace dfh::utils {

    /// \brief Extracts all files from a ZIP archive provided as a string.
    /// \param zip_data Raw data of the ZIP archive in memory.
    /// \param output_files A vector to store extracted file name and content pairs.
    /// \throws std::runtime_error If the extraction fails at any stage.
    void extract_zip_in_memory(
            const std::string& zip_data,
            std::vector<std::pair<std::string, std::string>>& output_files) {
        void* zip_handle = mz_zip_create();
        if (!zip_handle) {
            throw std::runtime_error("Failed to create zip handle.");
        }

        void* mem_stream = mz_stream_mem_create();
        if (!mem_stream) {
            mz_zip_delete(&zip_handle);
            throw std::runtime_error("Failed to create memory stream.");
        }

        // Инициализируем поток данными
        mz_stream_mem_set_buffer(mem_stream, (void*)zip_data.data(), zip_data.size());
        mz_stream_open(mem_stream, nullptr, MZ_OPEN_MODE_READ);

        if (mz_zip_open(zip_handle, mem_stream, MZ_OPEN_MODE_READ) != MZ_OK) {
            mz_zip_delete(&zip_handle);
            mz_stream_mem_delete(&mem_stream);
            throw std::runtime_error("Failed to open zip from memory.");
        }

        int32_t entry_result = mz_zip_goto_first_entry(zip_handle);
        while (entry_result == MZ_OK) {
            // Skip directories
            if (mz_zip_entry_is_dir(zip_handle) == MZ_OK) {
                entry_result = mz_zip_goto_next_entry(zip_handle);
                continue;
            }

            // Получаем информацию о текущем файле
            mz_zip_file* zip_entry = nullptr;
            if (mz_zip_entry_get_info(zip_handle, &zip_entry) != MZ_OK) {
                mz_zip_close(zip_handle);
                mz_zip_delete(&zip_handle);
                mz_stream_mem_delete(&mem_stream);
                throw std::runtime_error("Failed to get entry information.");
            }

            std::string file_name(zip_entry->filename);
            if (file_name.empty()) {
                entry_result = mz_zip_goto_next_entry(zip_handle);
                continue;
            }

            std::ostringstream debug_info;
            debug_info << "[DEBUG] ZIP Entry: " << file_name
                       << ", compression: " << zip_entry->compression_method
                       << ", AES version: " << static_cast<int>(zip_entry->aes_version);
            std::cout << debug_info.str() << std::endl;

            // Открываем файл для чтения
            int32_t open_result = mz_zip_entry_read_open(zip_handle, 0, nullptr);
            if (open_result != MZ_OK) {
                mz_zip_close(zip_handle);
                mz_zip_delete(&zip_handle);
                mz_stream_mem_delete(&mem_stream);
                throw std::runtime_error("Failed to open zip entry for reading: " + file_name + "; Code: " + std::to_string(open_result));
            }

            // Читаем содержимое файла
            std::string file_content;
            file_content.resize(static_cast<size_t>(zip_entry->uncompressed_size));
            if (mz_zip_entry_read(zip_handle, file_content.data(), file_content.size()) < 0) {
                mz_zip_entry_close(zip_handle);
                mz_zip_close(zip_handle);
                mz_zip_delete(&zip_handle);
                mz_stream_mem_delete(&mem_stream);
                throw std::runtime_error("Failed to read content of: " + file_name);
            }

            mz_zip_entry_close(zip_handle);
            output_files.emplace_back(file_name, file_content);
            entry_result = mz_zip_goto_next_entry(zip_handle);
        }

        // Закрываем zip-архив и поток
        mz_zip_close(zip_handle);
        mz_zip_delete(&zip_handle);
        mz_stream_mem_delete(&mem_stream);
    }

    /// \brief Extracts the first file from a ZIP archive provided as a string.
    /// \param zip_data Raw data of the ZIP archive in memory.
    /// \return Content of the first file found in the archive.
    /// \throws std::runtime_error If the archive is empty or extraction fails.
    std::string extract_first_file_from_zip(const std::string& zip_data) {
        std::vector<std::pair<std::string, std::string>> output_files;
        extract_zip_in_memory(zip_data, output_files);
        if (output_files.empty()) throw std::runtime_error("ZIP archive is empty.");
        return output_files[0].second;
    }

    /// \brief Decompresses a GZIP-compressed file into a std::string.
    /// \param gzip_data Raw data of the GZIP file in memory.
    /// \param max_bytes Maximum allowed decompressed size in bytes (default: 2GB).
    /// \return Decompressed content as a string.
    /// \throws std::runtime_error If input is empty or decompression fails.
    std::string extract_from_gzip(const std::string& gzip_data, size_t max_bytes = 2000000000) {
        if (gzip_data.empty()) throw std::runtime_error("GZIP archive is empty.");
        return gzip::decompress(gzip_data.data(), gzip_data.size(), max_bytes);
    }

}; // namespace dfh::utils

#endif // _DFH_UTILS_ZIP_UTILS_HPP_INCLUDED
