#pragma once
#ifndef _DFH_UTILS_ZIP_UTILS_HPP_INCLUDED
#define _DFH_UTILS_ZIP_UTILS_HPP_INCLUDED

/// \file zip_utils.hpp
/// \brief

#include <mz.h>
#include <mz_strm.h>
#include <mz_strm_buf.h>
#include <mz_strm_mem.h>
#include <mz_strm_split.h>
#include <mz_zip.h>
#include <mz_zip_rw.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>

namespace dfh::utils {

    /// \brief
    /// \param zip_data The raw data of the ZIP archive.
    /// \param output_files
    /// \throws std::runtime_error If the extraction fails.
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

        if (mz_zip_open(zip_handle, mem_stream, MZ_OPEN_MODE_READ) != MZ_OK) {
            mz_zip_delete(&zip_handle);
            mz_stream_mem_delete(&mem_stream);
            throw std::runtime_error("Failed to open zip from memory.");
        }

        int32_t entry_result = mz_zip_goto_first_entry(zip_handle);
        while (entry_result == MZ_OK) {
            mz_zip_file* zip_entry = nullptr;

            // Получаем информацию о текущем файле
            if (mz_zip_entry_get_info(zip_handle, &zip_entry) != MZ_OK) {
                mz_zip_close(zip_handle);
                mz_zip_delete(&zip_handle);
                mz_stream_mem_delete(&mem_stream);
                throw std::runtime_error("Failed to get entry information.");
            }

            std::string file_name(zip_entry->filename);
            if (file_name.empty()) continue;

            // Открываем файл для чтения
            if (mz_zip_entry_read_open(zip_handle, 0, nullptr) != MZ_OK) {
                mz_zip_close(zip_handle);
                mz_zip_delete(&zip_handle);
                mz_stream_mem_delete(&mem_stream);
                throw std::runtime_error("Failed to open zip entry for reading: " + file_name);
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

            // Сохраняем имя и содержимое файла
            output_files.emplace_back(file_name, file_content);
            entry_result = mz_zip_goto_next_entry(zip_handle);
        }

        // Закрываем zip-архив и поток
        mz_zip_close(zip_handle);
        mz_zip_delete(&zip_handle);
        mz_stream_mem_delete(&mem_stream);
    }

    /// \brief Extracts the first file from a ZIP archive provided as a string.
    /// \param zip_data The raw data of the ZIP archive.
    /// \return A string containing the extracted file's content.
    /// \throws std::runtime_error If the extraction fails.
    std::string extract_first_file_from_zip(const std::string& zip_data) {
        std::vector<std::pair<std::string, std::string>> output_files;
        extract_zip_in_memory(zip_data, output_files);
        if (output_files.empty()) throw std::runtime_error("?");
        return output_files[0].second;
    }

}; // namespace dfh::utils

#endif // _DFH_UTILS_ZIP_UTILS_HPP_INCLUDED
