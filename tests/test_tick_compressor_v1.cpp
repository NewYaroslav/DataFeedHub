#include <iostream>
#include <vector>
#include <filesystem>
#include <cassert>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <random>
#include <ctime>
#include <gzip/config.hpp>
#include <gzip/decompress.hpp>
#include <gzip/utils.hpp>
#include <time_shield_cpp/time_shield.hpp>
#include <fast_double_parser.h>
#include <DataFeedHub/src/structures/ticks/flags.hpp>
#include <DataFeedHub/src/structures/ticks/MarketTick.hpp>
#include <DataFeedHub/src/structures/ticks/TickEncodingConfig.hpp>
#include <DataFeedHub/src/structures/ticks/TickSequence.hpp>
#include <DataFeedHub/src/utils/math_utils.hpp>
#include <DataFeedHub/src/utils/fixed_point.hpp>
#include <DataFeedHub/src/utils/aligned_allocator.hpp>
#include <DataFeedHub/src/utils/sse_double_int64_utils.hpp>
#include <DataFeedHub/src/utils/vbyte.hpp>
#include <DataFeedHub/src/utils/simdcomp.hpp>
#include <DataFeedHub/src/utils/zip_utils.hpp>
#include <DataFeedHub/src/utils/bybit_parser.hpp>
#include <DataFeedHub/src/utils/binance_parser.hpp>
#include <DataFeedHub/src/utils/string_utils.hpp>
#include <DataFeedHub/src/compression/utils/zstd_utils.hpp>
#include <DataFeedHub/src/compression/utils/frequency_encoding.hpp>
#include <DataFeedHub/src/compression/utils/repeat_encoding.hpp>
#include <DataFeedHub/src/compression/utils/volume_scaling.hpp>
#include <DataFeedHub/src/compression/utils/zig_zag.hpp>
#include <DataFeedHub/src/compression/utils/zig_zag_delta.hpp>
#include <DataFeedHub/src/compression/ticks/TickCompressorV1/TickCompressionContextV1.hpp>
#include <DataFeedHub/src/compression/ticks/TickCompressorV1/TickEncoderV1.hpp>
#include <DataFeedHub/src/compression/ticks/TickCompressorV1/TickDecoderV1.hpp>
#include <DataFeedHub/src/compression/ticks/TickCompressorV1.hpp>


namespace dfh::compression {

    /// \brief Типы криптовалютных бирж
    enum class ExchangeType {
        BYBIT_FUTURES,   ///< Bybit Exchange
        BINANCE_SPOT,    ///< Binance Exchange
        BINANCE_FUTURES, ///< Binance Exchange
        // Добавляйте новые биржи здесь
    };

    /// \brief Конфигурация файла с тик-данными
    struct TickFileConfig {
        std::string file_path;  ///< Путь к файлу
        ExchangeType exchange;  ///< Тип криптобиржи
        size_t price_digits;    ///< Количество знаков после запятой для цены
        size_t volume_digits;   ///< Количество знаков после запятой для объема
        bool auto_detect_precision; ///<

        /// \brief Конструктор для инициализации всех параметров
        /// \param path Путь к файлу
        /// \param price Количество знаков после запятой для цены
        /// \param volume Количество знаков после запятой для объема
        /// \param exch Тип криптобиржи
        TickFileConfig(
                const std::string& path,
                ExchangeType exch,
                size_t price,
                size_t volume,
                bool auto_precision)
            : file_path(path), exchange(exch),
            price_digits(price), volume_digits(volume),
            auto_detect_precision(auto_precision) {}
    };

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

    /// \brief Получает список всех файлов в указанной папке
    /// \param folder_path Путь к папке
    /// \return Вектор строк с путями к файлам
    std::vector<std::string> get_files_in_folder(const std::string& folder_path) {
        std::vector<std::string> files;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(folder_path)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
            }
        }
        return files;
    }

    /// \brief Загружает тики из всех файлов в папке
    /// \param folder_path Путь к папке
    /// \param sequence Секвенция тиков для заполнения
    size_t load_ticks_from_folder(
            const std::string& folder_path,
            ExchangeType exchange,
            std::function<void(MarketTickSequence&)> callback) {
        size_t raw_size = 0;
        auto files = get_files_in_folder(folder_path);
        for (const auto& file : files) {
            try {
                std::string content = read_file_to_string(file);
                std::cout << "content: " << content.size() << std::endl;
                dfh::MarketTickSequence sequence;

                switch (exchange) {
                case ExchangeType::BYBIT_FUTURES: {
                        if (!gzip::is_compressed(content.c_str(), content.size())) {
                            std::cerr << "The file is not compressed." << std::endl;
                            continue;
                        }
                        std::cout << "Decompressing the file: " << file << std::endl;
                        std::string decompressed_data = gzip::decompress(content);
                        std::cout << "Decompressed data size: " << decompressed_data.size() << std::endl;
                        raw_size += decompressed_data.size();
                        dfh::utils::parse_bybit_trades(sequence, decompressed_data, 3000000, true);
                        break;
                    }
                case ExchangeType::BINANCE_SPOT: {
                        std::cout << "Decompressing the file: " << file << std::endl;
                        std::string decompressed_data = dfh::utils::extract_first_file_from_zip(content);
                        std::cout << "Decompressed data size: " << decompressed_data.size() << std::endl;
                        raw_size += decompressed_data.size();
                        dfh::utils::parse_binance_spot_trades(sequence, decompressed_data, 3000000, true);
                        break;
                    }
                case ExchangeType::BINANCE_FUTURES: {
                        std::cout << "Decompressing the file: " << file << std::endl;
                        std::string decompressed_data = dfh::utils::extract_first_file_from_zip(content);
                        std::cout << "Decompressed data size: " << decompressed_data.size() << std::endl;
                        raw_size += decompressed_data.size();
                        dfh::utils::parse_binance_futures_trades(sequence, decompressed_data, 3000000, true);
                        break;
                    }
                default: break;
                };

                std::cout << "price_digits: " << sequence.price_digits << std::endl;
                std::cout << "volume_digits: " << sequence.volume_digits << std::endl;
                callback(sequence);
            } catch (const std::exception& e) {
                std::cerr << "Error loading ticks from file " << file << ": " << e.what() << std::endl;
            }
        }
        return raw_size;
    }

    void find_min_max_sample_sizes(const std::vector<std::vector<uint8_t>>& samples) {
        if (samples.empty()) {
            std::cout << "No samples provided." << std::endl;
            return;
        }

        size_t min_size = std::numeric_limits<size_t>::max();
        size_t max_size = 0;

        for (const auto& sample : samples) {
            if (sample.size() < min_size) {
                min_size = sample.size();
            }
            if (sample.size() > max_size) {
                max_size = sample.size();
            }
        }

        std::cout << "Minimum sample size: " << min_size << " bytes" << std::endl;
        std::cout << "Maximum sample size: " << max_size << " bytes" << std::endl;
    }

    template <typename Func>
    uint64_t measure_execution_time(Func func) {
        auto start = std::chrono::high_resolution_clock::now();
        func();
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    }

    void test_compression(
            dfh::compression::TickCompressorV1& compressor,
            uint64_t& compressed_size,
            uint64_t& min_size,
            uint64_t& max_size,
            uint64_t& compression_time,
            uint64_t& decompression_time,
            const std::vector<dfh::MarketTick>& ticks,
            const dfh::TickEncodingConfig& config) {
        if (ticks.empty()) return;
        std::cout << "ticks:  " << ticks.size() << std::endl;

        std::vector<uint8_t> compressed_data;

        uint64_t compression_duration = measure_execution_time([&]() {
            compressor.compress(ticks, config, compressed_data);
        });

        std::vector<dfh::MarketTick> decompress_ticks;
        uint64_t decompression_duration = measure_execution_time([&]() {
            compressor.decompress(compressed_data, decompress_ticks);
        });

        if (compressed_data.size() < min_size) min_size = compressed_data.size();
        if (compressed_data.size() > max_size) max_size = compressed_data.size();

        compressed_size += compressed_data.size();
        compression_time += compression_duration;
        decompression_time += decompression_duration;

        std::cout << "comp dur: " << compression_duration << std::endl;
        std::cout << "dec dur:  " << decompression_duration << std::endl;
    }
}

int main() {

    std::vector<dfh::compression::TickFileConfig> tick_files = {
        {"..\\..\\dataset\\zstd\\trades\\train\\binance\\spot", dfh::compression::ExchangeType::BINANCE_SPOT, 0, 0, true},
        {"..\\..\\dataset\\zstd\\trades\\train\\binance\\futures2", dfh::compression::ExchangeType::BINANCE_FUTURES, 0, 0, true},
        {"..\\..\\dataset\\zstd\\trades\\train\\bybit\\futures", dfh::compression::ExchangeType::BYBIT_FUTURES, 0, 0, true},
    };

    //std::vector<std::vector<uint8_t>> samples;
    uint64_t compression_time = 0;
    uint64_t decompression_time = 0;
    uint64_t compressed_size = 0;
    uint64_t raw_original_size = 0;
    uint64_t original_size = 0;
    uint64_t min_size = std::numeric_limits<size_t>::max();
    uint64_t max_size = 0;
    uint64_t num_samples = 0;
    dfh::compression::TickCompressorV1 compressor;

    // Загружаем данные
    for (const auto& config : tick_files) {
        raw_original_size += load_ticks_from_folder(config.file_path, config.exchange,
                [&](dfh::MarketTickSequence& sequence) {
            std::cout << "Ticks size: " << sequence.ticks.size() << std::endl;
            if (sequence.ticks.empty()) {
                std::cerr << "No ticks loaded from folder: " << config.file_path << std::endl;
                return;
            }

            dfh::TickEncodingConfig compressor_config;
            compressor_config.enable_tick_flags           = true;
            compressor_config.enable_trade_based_encoding = true;
            compressor_config.price_digits  = sequence.price_digits;
            compressor_config.volume_digits = sequence.volume_digits;
            compressor_config.fixed_spread = 1;

            std::vector<dfh::MarketTick> ticks;
            int last_hour = time_shield::hour_of_day(time_shield::ms_to_sec(sequence.ticks[0].time_ms));

            for (const auto& tick : sequence.ticks) {
                int tick_hour = time_shield::hour_of_day(time_shield::ms_to_sec(tick.time_ms));

                if (tick_hour != last_hour) {
                    last_hour = tick_hour;
                    original_size += (ticks.size() * (8 * 3)) + ticks.size();

                    std::cout << "time: " << time_shield::to_iso8601_ms(tick.time_ms) << std::endl;

                    test_compression(
                        compressor,
                        compressed_size,
                        min_size,
                        max_size,
                        compression_time,
                        decompression_time,
                        ticks,
                        compressor_config);
                    ++num_samples;

                    ticks.clear();
                }

                ticks.push_back(tick);
            }

            if (!ticks.empty()) {
                original_size += (ticks.size() * (8 * 3)) + ticks.size();
                test_compression(
                    compressor,
                    compressed_size,
                    min_size,
                    max_size,
                    compression_time,
                    decompression_time,
                    ticks,
                    compressor_config);
                ++num_samples;
            }
        });
    }


    compression_time   /= num_samples;
    decompression_time /= num_samples;

    double raw_compression_ratio = static_cast<double>(raw_original_size) / static_cast<double>(compressed_size);
    double compression_ratio = static_cast<double>(original_size) / static_cast<double>(compressed_size);
    raw_original_size /= num_samples;
    original_size /= num_samples;
    compressed_size /= num_samples;

    std::cout << "compression ratio:   " << compression_ratio << "; (raw): " << raw_compression_ratio << std::endl;
    std::cout << "compression time:    " << compression_time << " us" << std::endl;
    std::cout << "decompression time:  " << decompression_time << " us" << std::endl;
    std::cout << "original size (raw): " << raw_original_size << std::endl;
    std::cout << "original size:       " << original_size << std::endl;
    std::cout << "compressed size:     " << compressed_size << std::endl;

    return EXIT_SUCCESS;
}
