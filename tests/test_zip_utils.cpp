/// \file test_zip_utils.cpp
/// \brief Test program for GZIP and ZIP extraction utilities.

#include <DataFeedHub/utils.hpp>

#include <fstream>
#include <iostream>
#include <string>

/// \brief Reads the entire binary file into a std::string.
/// \param file_path Path to the file.
/// \return File content as a binary string.
/// \throws std::runtime_error If file can't be opened.
std::string read_file_to_string(const std::string& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + file_path);
    }
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

int main() {
    try {
        // --- Test BTCUSDT2023-09-23.csv.gz (plain GZIP) ---
        {
            std::string file_path = "BTCUSDT2023-09-23.csv.gz";
            std::cout << "[GZIP] Testing file: " << file_path << '\n';

            std::string gzip_data = read_file_to_string(file_path);
            std::string content = dfh::utils::extract_from_gzip(gzip_data);

            std::cout << "Decompressed size: " << content.size() << " bytes\n";
            std::cout << "First 300 characters:\n" << content.substr(0, 300) << "\n...\n";

            std::ofstream out_file("extracted_BTCUSDT.csv", std::ios::binary);
            out_file.write(content.data(), content.size());
        }

        std::cout << "\n";

        // --- Test ALGOUSDT-trades-2023-10-07.zip (ZIP with folder path inside) ---
        {
            std::string file_path = "ALGOUSDT-trades-2023-10-07.zip";
            //std::string file_path = "BTCUSDT-trades-2022-01-21.zip";
            std::cout << "[ZIP] Testing file: " << file_path << '\n';

            std::string zip_data = read_file_to_string(file_path);

            std::vector<std::pair<std::string, std::string>> files;
            dfh::utils::extract_zip_in_memory(zip_data, files);

            std::cout << "Extracted files:\n";
            for (const auto& [name, content] : files) {
                std::cout << " - " << name << " (" << content.size() << " bytes)\n";
            }

            std::string content = dfh::utils::extract_first_file_from_zip(zip_data);

            std::cout << "Extracted size: " << content.size() << " bytes\n";
            std::cout << "First 300 characters:\n" << content.substr(0, 300) << "\n...\n";

            std::ofstream out_file("extracted_ALGOUSDT.csv", std::ios::binary);
            out_file.write(content.data(), content.size());
        }

    } catch (const std::exception& e) {
        std::cerr << "[ERROR] " << e.what() << '\n';
        return 1;
    }

    return 0;
}
