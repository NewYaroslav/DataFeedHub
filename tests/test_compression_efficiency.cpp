#include <iostream>
#include <vector>
#include <filesystem>
#include <cassert>
#include <fstream>
#include <sstream>
#include <stdexcept>
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
#include <DataFeedHub/src/compression/utils/frequency_encoding.hpp>
#include <DataFeedHub/src/compression/utils/repeat_encoding.hpp>
#include <DataFeedHub/src/compression/utils/volume_scaling.hpp>
#include <DataFeedHub/src/compression/utils/zig_zag.hpp>
#include <DataFeedHub/src/compression/utils/zig_zag_delta.hpp>
#include <DataFeedHub/src/compression/ticks/TickCompressorV1/TickCompressionContextV1.hpp>
#include <DataFeedHub/src/compression/ticks/TickCompressorV1/TickEncoderV1.hpp>
#include <DataFeedHub/src/compression/ticks/TickCompressorV1/TickDecoderV1.hpp>

namespace dfh::compression {

    /// \brief Типы криптовалютных бирж
    enum class ExchangeType {
        Bybit,  ///< Bybit Exchange
        Binance ///< Binance Exchange
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
        for (const auto& entry : std::filesystem::directory_iterator(folder_path)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
            }
        }
        return files;
    }

    /// \brief Загружает тики из всех файлов в папке
    /// \param folder_path Путь к папке
    /// \param sequence Секвенция тиков для заполнения
    void load_ticks_from_folder(
            const std::string& folder_path,
            ExchangeType exchange,
            std::function<void(MarketTickSequence&)> callback) {
        auto files = get_files_in_folder(folder_path);
        for (const auto& file : files) {
            try {
                std::string content = read_file_to_string(file);
                std::cout << "content: " << content.size() << std::endl;
                dfh::MarketTickSequence sequence;

                switch (exchange) {
                case ExchangeType::Bybit: {
                        if (!gzip::is_compressed(content.c_str(), content.size())) {
                            std::cerr << "The file is not compressed." << std::endl;
                            continue;
                        }
                        std::cout << "Decompressing the file: " << file << std::endl;
                        std::string decompressed_data = gzip::decompress(content);
                        std::cout << "Decompressed data size: " << decompressed_data.size() << std::endl;
                        dfh::utils::parse_bybit_trades(sequence, decompressed_data, 3000000, true);
                        break;
                    }
                case ExchangeType::Binance: {
                        std::cout << "Decompressing the file: " << file << std::endl;
                        std::string decompressed_data = dfh::utils::extract_first_file_from_zip(content);
                        std::cout << "Decompressed data size: " << decompressed_data.size() << std::endl;
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
    }

    class TestTickEncoderV1 {
    public:
        explicit TestTickEncoderV1(TickCompressionContextV1& buffer) : buffer(buffer) {}

        void encode_price_last_v1(
                std::vector<uint8_t>& output,
                const MarketTick* ticks,
                size_t num_ticks,
                double price_scale,
                int64_t initial_price) {
            auto &deltas_u32 = buffer.deltas_u32;
            auto &deltas_u64= buffer.deltas_u64;
            try {
                deltas_u32.resize(num_ticks);
                encode_last_delta_zig_zag_int32(ticks, deltas_u32.data(), num_ticks, price_scale, initial_price);
                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.data(), num_ticks);
            } catch(std::overflow_error& e) {
                deltas_u64.resize(num_ticks);
                deltas_u32.resize(num_ticks);
                encode_last_delta_zig_zag_int64(ticks, deltas_u64.data(), num_ticks, price_scale, initial_price);
                dfh::utils::append_vbyte<uint64_t>(output, deltas_u64.data(), num_ticks);
            }
        }

        void encode_price_last_v2(
                std::vector<uint8_t>& output,
                const MarketTick* ticks,
                size_t num_ticks,
                double price_scale,
                int64_t initial_price) {
            auto &deltas_u32 = buffer.deltas_u32;
            auto &deltas_u64= buffer.deltas_u64;
            try {
                deltas_u32.resize(num_ticks);
                encode_last_delta_zig_zag_int32(ticks, deltas_u32.data(), num_ticks, price_scale, initial_price);
                dfh::utils::append_simdcomp(output, deltas_u32.data(), num_ticks);
            } catch(std::overflow_error& e) {
                deltas_u64.resize(num_ticks);
                deltas_u32.resize(num_ticks);
                encode_last_delta_zig_zag_int64(ticks, deltas_u64.data(), num_ticks, price_scale, initial_price);
                dfh::utils::append_vbyte<uint64_t>(output, deltas_u64.data(), num_ticks);
            }
        }

        void encode_price_last_v3(
                std::vector<uint8_t>& output,
                const MarketTick* ticks,
                size_t num_ticks,
                double price_scale,
                int64_t initial_price) {
            auto &deltas_u32 = buffer.deltas_u32;
            auto &deltas_u64= buffer.deltas_u64;
            try {
                deltas_u32.resize(num_ticks);
                encode_last_delta_zig_zag_int32(ticks, deltas_u32.data(), num_ticks, price_scale, initial_price);

                size_t repeats_size = 0;
                //encode_with_repeats(deltas_u32.data(), num_ticks, 0, deltas_u32.data(), repeats_size);
                encode_run_length(deltas_u32.data(), num_ticks, deltas_u32.data(), repeats_size);
                deltas_u32.resize(repeats_size);

                dfh::utils::append_vbyte<uint32_t>(output, repeats_size);
                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.data(), num_ticks);
            } catch(std::overflow_error& e) {
                deltas_u64.resize(num_ticks);
                deltas_u32.resize(num_ticks);
                encode_last_delta_zig_zag_int64(ticks, deltas_u64.data(), num_ticks, price_scale, initial_price);

                size_t repeats_size = 0;
                //encode_with_repeats(deltas_u64.data(), num_ticks, 0, deltas_u64.data(), repeats_size);
                encode_run_length(deltas_u32.data(), num_ticks, deltas_u32.data(), repeats_size);

                deltas_u64.resize(repeats_size);

                dfh::utils::append_vbyte<uint32_t>(output, repeats_size);
                dfh::utils::append_vbyte<uint64_t>(output, deltas_u64.data(), num_ticks);
            }
        }

        void encode_price_last_v4(
                std::vector<uint8_t>& output,
                const MarketTick* ticks,
                size_t num_ticks,
                double price_scale,
                int64_t initial_price) {
            auto &deltas_u32 = buffer.deltas_u32;
            auto &deltas_u64= buffer.deltas_u64;
            try {
                deltas_u32.resize(num_ticks);
                encode_last_delta_zig_zag_int32(ticks, deltas_u32.data(), num_ticks, price_scale, initial_price);

                size_t repeats_size = 0;
                encode_run_length(deltas_u32.data(), num_ticks, deltas_u32.data(), repeats_size);
                deltas_u32.resize(repeats_size);

                dfh::utils::append_vbyte<uint32_t>(output, repeats_size);
                dfh::utils::append_simdcomp(output, deltas_u32.data(), num_ticks);
            } catch(std::overflow_error& e) {
                deltas_u64.resize(num_ticks);
                deltas_u32.resize(num_ticks);
                encode_last_delta_zig_zag_int64(ticks, deltas_u64.data(), num_ticks, price_scale, initial_price);

                size_t repeats_size = 0;
                encode_run_length(deltas_u64.data(), num_ticks, deltas_u64.data(), repeats_size);
                deltas_u64.resize(repeats_size);

                dfh::utils::append_vbyte<uint32_t>(output, repeats_size);
                dfh::utils::append_vbyte<uint64_t>(output, deltas_u64.data(), num_ticks);
            }
        }

        void encode_price_last_v5(
                std::vector<uint8_t>& output,
                const MarketTick* ticks,
                size_t num_ticks,
                double price_scale,
                int64_t initial_price) {
            auto &deltas_u32 = buffer.deltas_u32;
            auto &deltas_u64= buffer.deltas_u64;
            auto &values_u32 = buffer.values_u32;
            auto &values_u64 = buffer.values_u64;
            auto &index_map_u32 = buffer.index_map_u32;
            try {
                deltas_u32.resize(num_ticks);
                encode_last_delta_zig_zag_int32(ticks, deltas_u32.data(), num_ticks, price_scale, initial_price);
                encode_frequency(deltas_u32.data(), deltas_u32.data(), num_ticks, values_u32, index_map_u32);

                encode_delta_sorted<uint32_t, uint32_t>(values_u32.data(), values_u32.data(), values_u32.size(), 0);
                encode_delta_zig_zag_int32(index_map_u32.data(), index_map_u32.data(), index_map_u32.size(), 0);

                uint32_t values_length = values_u32.size();
                values_length = (values_length << 1);

                dfh::utils::append_vbyte<uint32_t>(output, values_length);
                dfh::utils::append_vbyte<uint32_t>(output, values_u32.data(), values_u32.size());
                dfh::utils::append_vbyte<uint32_t>(output, index_map_u32.data(), values_u32.size());
                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.data(), num_ticks);

            } catch(std::overflow_error& e) {
                // Обработка ошибки encode_last_delta_zig_zag_int32 или encode_delta_sorted<uint32_t, uint32_t>
                deltas_u64.resize(num_ticks);
                deltas_u32.resize(num_ticks);
                encode_last_delta_zig_zag_int64(ticks, deltas_u64.data(), num_ticks, price_scale, initial_price);
                encode_frequency(deltas_u64.data(), deltas_u32.data(), num_ticks, values_u64, index_map_u32);

                encode_delta_sorted<uint64_t, uint64_t>(values_u64.data(), values_u64.data(), values_u64.size(), 0);
                encode_delta_zig_zag_int32(index_map_u32.data(), index_map_u32.data(), index_map_u32.size(), 0);

                uint32_t values_length = values_u64.size();
                values_length = (values_length << 1) | 0x1;

                dfh::utils::append_vbyte<uint32_t>(output, values_length);
                dfh::utils::append_vbyte<uint64_t>(output, values_u64.data(), values_u64.size());
                dfh::utils::append_vbyte<uint32_t>(output, index_map_u32.data(), values_u64.size());
                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.data(), num_ticks);
            }
        }

        void encode_price_last_v6(
                std::vector<uint8_t>& output,
                const MarketTick* ticks,
                size_t num_ticks,
                double price_scale,
                int64_t initial_price) {
            auto &deltas_u32 = buffer.deltas_u32;
            auto &deltas_u64= buffer.deltas_u64;
            auto &values_u32 = buffer.values_u32;
            auto &values_u64 = buffer.values_u64;
            auto &index_map_u32 = buffer.index_map_u32;
            try {
                deltas_u32.resize(num_ticks);
                encode_last_delta_zig_zag_int32(ticks, deltas_u32.data(), num_ticks, price_scale, initial_price);
                encode_frequency(deltas_u32.data(), deltas_u32.data(), num_ticks, values_u32, index_map_u32);

                encode_delta_sorted<uint32_t, uint32_t>(values_u32.data(), values_u32.data(), values_u32.size(), 0);
                encode_delta_zig_zag_int32(index_map_u32.data(), index_map_u32.data(), index_map_u32.size(), 0);

                uint32_t values_length = values_u32.size();
                values_length = (values_length << 1);

                dfh::utils::append_vbyte<uint32_t>(output, values_length);
                dfh::utils::append_simdcomp(output, values_u32.data(), values_u32.size());
                dfh::utils::append_simdcomp(output, index_map_u32.data(), values_u32.size());
                dfh::utils::append_simdcomp(output, deltas_u32.data(), num_ticks);
            } catch(std::overflow_error& e) {
                // Обработка ошибки encode_last_delta_zig_zag_int32 или encode_delta_sorted<uint32_t, uint32_t>
                deltas_u64.resize(num_ticks);
                deltas_u32.resize(num_ticks);
                encode_last_delta_zig_zag_int64(ticks, deltas_u64.data(), num_ticks, price_scale, initial_price);
                encode_frequency(deltas_u64.data(), deltas_u32.data(), num_ticks, values_u64, index_map_u32);

                encode_delta_sorted<uint64_t, uint64_t>(values_u64.data(), values_u64.data(), values_u64.size(), 0);
                encode_delta_zig_zag_int32(index_map_u32.data(), index_map_u32.data(), index_map_u32.size(), 0);

                uint32_t values_length = values_u64.size();
                values_length = (values_length << 1) | 0x1;

                dfh::utils::append_vbyte<uint32_t>(output, values_length);
                dfh::utils::append_vbyte<uint64_t>(output, values_u64.data(), values_u64.size());
                dfh::utils::append_simdcomp(output, index_map_u32.data(), values_u64.size());
                dfh::utils::append_simdcomp(output, deltas_u32.data(), num_ticks);
            }
        }

        void encode_price_last_v7(
                std::vector<uint8_t>& output,
                const MarketTick* ticks,
                size_t num_ticks,
                double price_scale,
                int64_t initial_price) {
            auto &deltas_u32 = buffer.deltas_u32;
            auto &deltas_u64= buffer.deltas_u64;
            auto &values_u32 = buffer.values_u32;
            auto &values_u64 = buffer.values_u64;
            auto &index_map_u32 = buffer.index_map_u32;
            try {
                deltas_u32.resize(num_ticks);
                encode_last_delta_zig_zag_int32(ticks, deltas_u32.data(), num_ticks, price_scale, initial_price);
                encode_frequency(deltas_u32.data(), deltas_u32.data(), num_ticks, values_u32, index_map_u32);

                size_t repeats_size = 0;
                encode_zero_with_repeats(deltas_u32.data(), deltas_u32.size(), deltas_u32.data(), repeats_size);
                deltas_u32.resize(repeats_size);

                encode_delta_sorted<uint32_t, uint32_t>(values_u32.data(), values_u32.data(), values_u32.size(), 0);
                encode_delta_zig_zag_int32(index_map_u32.data(), index_map_u32.data(), index_map_u32.size(), 0);

                uint32_t values_length = values_u32.size();
                values_length = (values_length << 1);

                dfh::utils::append_vbyte<uint32_t>(output, values_length);
                dfh::utils::append_simdcomp(output, values_u32.data(), values_u32.size());
                dfh::utils::append_simdcomp(output, index_map_u32.data(), values_u32.size());

                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.size());
                dfh::utils::append_simdcomp(output, deltas_u32.data(), deltas_u32.size());
            } catch(std::overflow_error& e) {
                // Обработка ошибки encode_last_delta_zig_zag_int32 или encode_delta_sorted<uint32_t, uint32_t>
                deltas_u64.resize(num_ticks);
                deltas_u32.resize(num_ticks);
                encode_last_delta_zig_zag_int64(ticks, deltas_u64.data(), num_ticks, price_scale, initial_price);
                encode_frequency(deltas_u64.data(), deltas_u32.data(), num_ticks, values_u64, index_map_u32);

                size_t repeats_size = 0;
                encode_zero_with_repeats(deltas_u32.data(), deltas_u32.size(), deltas_u32.data(), repeats_size);
                deltas_u32.resize(repeats_size);

                encode_delta_sorted<uint64_t, uint64_t>(values_u64.data(), values_u64.data(), values_u64.size(), 0);
                encode_delta_zig_zag_int32(index_map_u32.data(), index_map_u32.data(), index_map_u32.size(), 0);

                uint32_t values_length = values_u64.size();
                values_length = (values_length << 1) | 0x1;

                dfh::utils::append_vbyte<uint32_t>(output, values_length);
                dfh::utils::append_vbyte<uint64_t>(output, values_u64.data(), values_u64.size());
                dfh::utils::append_simdcomp(output, index_map_u32.data(), values_u64.size());

                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.size());
                dfh::utils::append_simdcomp(output, deltas_u32.data(), deltas_u32.size());
            }
        }

        void encode_price_last_v8(
                std::vector<uint8_t>& output,
                const MarketTick* ticks,
                size_t num_ticks,
                double price_scale,
                int64_t initial_price) {
            auto &deltas_u32 = buffer.deltas_u32;
            auto &deltas_u64= buffer.deltas_u64;
            auto &values_u32 = buffer.values_u32;
            auto &values_u64 = buffer.values_u64;
            auto &index_map_u32 = buffer.index_map_u32;
            try {
                deltas_u32.resize(num_ticks);
                encode_last_delta_zig_zag_int32(ticks, deltas_u32.data(), num_ticks, price_scale, initial_price);
                encode_frequency(deltas_u32.data(), deltas_u32.data(), num_ticks, values_u32, index_map_u32);

                size_t repeats_size = 0;
                encode_run_length(deltas_u32.data(), deltas_u32.size(), deltas_u32.data(), repeats_size);
                deltas_u32.resize(repeats_size);

                encode_delta_sorted<uint32_t, uint32_t>(values_u32.data(), values_u32.data(), values_u32.size(), 0);
                encode_delta_zig_zag_int32(index_map_u32.data(), index_map_u32.data(), index_map_u32.size(), 0);

                uint32_t values_length = values_u32.size();
                values_length = (values_length << 1);

                dfh::utils::append_vbyte<uint32_t>(output, values_length);
                dfh::utils::append_simdcomp(output, values_u32.data(), values_u32.size());
                dfh::utils::append_simdcomp(output, index_map_u32.data(), values_u32.size());

                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.size());
                dfh::utils::append_simdcomp(output, deltas_u32.data(), deltas_u32.size());
            } catch(std::overflow_error& e) {
                // Обработка ошибки encode_last_delta_zig_zag_int32 или encode_delta_sorted<uint32_t, uint32_t>
                deltas_u64.resize(num_ticks);
                deltas_u32.resize(num_ticks);
                encode_last_delta_zig_zag_int64(ticks, deltas_u64.data(), num_ticks, price_scale, initial_price);
                encode_frequency(deltas_u64.data(), deltas_u32.data(), num_ticks, values_u64, index_map_u32);

                size_t repeats_size = 0;
                encode_run_length(deltas_u32.data(), deltas_u32.size(), deltas_u32.data(), repeats_size);
                deltas_u32.resize(repeats_size);

                encode_delta_sorted<uint64_t, uint64_t>(values_u64.data(), values_u64.data(), values_u64.size(), 0);
                encode_delta_zig_zag_int32(index_map_u32.data(), index_map_u32.data(), index_map_u32.size(), 0);

                uint32_t values_length = values_u64.size();
                values_length = (values_length << 1) | 0x1;

                dfh::utils::append_vbyte<uint32_t>(output, values_length);
                dfh::utils::append_vbyte<uint64_t>(output, values_u64.data(), values_u64.size());
                dfh::utils::append_simdcomp(output, index_map_u32.data(), values_u64.size());

                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.size());
                dfh::utils::append_simdcomp(output, deltas_u32.data(), deltas_u32.size());
            }
        }

        void encode_volume(
                std::vector<uint8_t>& output,
                const MarketTick* ticks,
                size_t num_ticks,
                double volume_scale,
                size_t bits) {
            auto &deltas_u32 = buffer.deltas_u32;
            auto &deltas_u64= buffer.deltas_u64;
            auto &values_u32 = buffer.values_u32;
            auto &values_u64 = buffer.values_u64;
            auto &index_map_u32 = buffer.index_map_u32;
            try {
                deltas_u32.resize(num_ticks);
                scale_volume_int32(ticks, deltas_u32.data(), num_ticks, volume_scale);
                encode_frequency(deltas_u32.data(), deltas_u32.data(), num_ticks, values_u32, index_map_u32);

                size_t repeats_size = 0;
                encode_with_repeats(deltas_u32.data(), deltas_u32.size(), bits, deltas_u32.data(), repeats_size);
                deltas_u32.resize(repeats_size);

                encode_delta_sorted<uint32_t, uint32_t>(values_u32.data(), values_u32.data(), values_u32.size(), 0);
                encode_delta_zig_zag_int32(index_map_u32.data(), index_map_u32.data(), index_map_u32.size(), 0);

                uint32_t values_length = values_u32.size();
                values_length = (values_length << 1);

                dfh::utils::append_vbyte<uint32_t>(output, values_length);
                dfh::utils::append_simdcomp(output, values_u32.data(), values_u32.size());
                dfh::utils::append_simdcomp(output, index_map_u32.data(), values_u32.size());

                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.size());
                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.data(), deltas_u32.size());
            } catch(std::overflow_error& e) {
                // Обработка ошибки scale_volume_int32, encode_frequency и encode_delta_sorted<uint32_t, uint32_t>
                deltas_u64.resize(num_ticks);
                deltas_u32.resize(num_ticks);
                scale_volume_int64(ticks, deltas_u64.data(), num_ticks, volume_scale);
                encode_frequency(deltas_u64.data(), deltas_u32.data(), num_ticks, values_u64, index_map_u32);

                size_t repeats_size = 0;
                encode_with_repeats(deltas_u32.data(), deltas_u32.size(), bits, deltas_u32.data(), repeats_size);
                deltas_u32.resize(repeats_size);

                encode_delta_sorted<uint64_t, uint64_t>(values_u64.data(), values_u64.data(), values_u64.size(), 0);
                encode_delta_zig_zag_int32(index_map_u32.data(), index_map_u32.data(), index_map_u32.size(), 0);

                uint32_t values_length = values_u64.size();
                values_length = (values_length << 1) | 0x1;

                dfh::utils::append_vbyte<uint32_t>(output, values_length);
                dfh::utils::append_vbyte<uint64_t>(output, values_u64.data(), values_u64.size());
                dfh::utils::append_simdcomp(output, index_map_u32.data(), values_u64.size());

                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.size());
                dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.data(), deltas_u32.size());
            }
        }

        void encode_time(
                std::vector<uint8_t>& output,
                const MarketTick* ticks,
                size_t num_ticks,
                int64_t initial_time,
                size_t bits) {
            auto &deltas_u32 = buffer.deltas_u32;
            auto &values_u32 = buffer.values_u32;
            auto &index_map_u32 = buffer.index_map_u32;

            deltas_u32.resize(num_ticks);
            encode_time_delta(ticks, deltas_u32.data(), num_ticks, initial_time);
            encode_frequency(deltas_u32.data(), deltas_u32.data(), num_ticks, values_u32, index_map_u32);

            size_t repeats_size = 0;
            encode_with_repeats(deltas_u32.data(), deltas_u32.size(), bits, deltas_u32.data(), repeats_size);
            deltas_u32.resize(repeats_size);

            encode_delta_sorted<uint32_t, uint32_t>(values_u32.data(), values_u32.data(), values_u32.size(), 0);
            encode_delta_zig_zag_int32(index_map_u32.data(), index_map_u32.data(), index_map_u32.size(), 0);

            dfh::utils::append_vbyte<uint32_t>(output, values_u32.size());
            dfh::utils::append_simdcomp(output, values_u32.data(), values_u32.size());
            dfh::utils::append_simdcomp(output, index_map_u32.data(), values_u32.size());

            dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.size());
            dfh::utils::append_vbyte<uint32_t>(output, deltas_u32.data(), deltas_u32.size());
        }

        void encode_side_flags(
                std::vector<uint8_t>& output,
                const MarketTick* ticks,
                size_t num_ticks) {
            constexpr size_t chunk_width = sizeof(uint8_t);
            constexpr size_t bif_offset = 4;
            const size_t start_offset = output.size();
            const size_t aligned_size = num_ticks - (num_ticks % chunk_width);

            output.resize(start_offset + ((num_ticks + (chunk_width - 1)) / chunk_width));

            size_t max_j, byte_index = start_offset;
            for (size_t i = 0; i < aligned_size; i += chunk_width, ++byte_index) {
                max_j = i + chunk_width;
                output[byte_index] = (ticks[i].flags >> bif_offset) & 0x1;
                for (size_t j = i + 1; j < max_j; ++j) {
                    output[byte_index] <<= 1;
                    output[byte_index] |= (ticks[j].flags >> bif_offset) & 0x1;
                }
            }

            for (size_t bit_index = 0, i = aligned_size; i < num_ticks; ++i, ++bit_index) {
                output[byte_index] |= ((ticks[i].flags >> bif_offset) & 0x1) << bit_index;
            }
        }

    private:
        TickCompressionContextV1& buffer; ///< Ссылка на общий буфер
    };


    void test_encode_price_efficiency(
            std::array<size_t, 8> &encode_price_size,
            const std::vector<MarketTick>& ticks,
            size_t price_digits,
            size_t volume_digits) {
        std::cout << "Testing compression efficiency:\n";

        TickCompressionContextV1 buffer;
        TestTickEncoderV1 encoder(buffer);

        // Настройки сжатия
        double price_scale = utils::pow10<double>(price_digits);
        int64_t initial_price = std::llround(ticks[0].last * price_scale);
        //uint64_t base_time = ticks[0].time_ms;

        // Сжатие
        std::vector<uint8_t> compressed_prices;

        compressed_prices.clear();
        encoder.encode_price_last_v1(compressed_prices, ticks.data(), ticks.size(), price_scale, initial_price);
        encode_price_size[0] += compressed_prices.size();

        compressed_prices.clear();
        encoder.encode_price_last_v2(compressed_prices, ticks.data(), ticks.size(), price_scale, initial_price);
        encode_price_size[1] += compressed_prices.size();

        compressed_prices.clear();
        encoder.encode_price_last_v3(compressed_prices, ticks.data(), ticks.size(), price_scale, initial_price);
        encode_price_size[2] += compressed_prices.size();

        compressed_prices.clear();
        encoder.encode_price_last_v4(compressed_prices, ticks.data(), ticks.size(), price_scale, initial_price);
        encode_price_size[3] += compressed_prices.size();

        compressed_prices.clear();
        encoder.encode_price_last_v5(compressed_prices, ticks.data(), ticks.size(), price_scale, initial_price);
        encode_price_size[4] += compressed_prices.size();

        compressed_prices.clear();
        encoder.encode_price_last_v6(compressed_prices, ticks.data(), ticks.size(), price_scale, initial_price);
        encode_price_size[5] += compressed_prices.size();

        compressed_prices.clear();
        encoder.encode_price_last_v7(compressed_prices, ticks.data(), ticks.size(), price_scale, initial_price);
        encode_price_size[6] += compressed_prices.size();

        compressed_prices.clear();
        encoder.encode_price_last_v8(compressed_prices, ticks.data(), ticks.size(), price_scale, initial_price);
        encode_price_size[7] += compressed_prices.size();
    }

} // namespace dfh::compression

int main() {
    std::vector<dfh::compression::TickFileConfig> tick_files = {
        {"..\\..\\dataset\\zstd\\trades\\train\\binance\\BTCUSDT", dfh::compression::ExchangeType::Binance, 0, 0, true},
        {"..\\..\\dataset\\zstd\\trades\\train\\bybit\\BTCUSDT", dfh::compression::ExchangeType::Bybit, 0, 0, true},
    };

    std::array<size_t, 8> encode_price_size = {0,0,0,0,0,0,0};
    size_t encode_price_count = 0;

    for (size_t i = 0; i < tick_files.size(); ++i) {
        dfh::compression::load_ticks_from_folder(
                tick_files[i].file_path,
                tick_files[i].exchange,
                [&](dfh::MarketTickSequence& sequence) {
            std::cout << "Ticks size: " << sequence.ticks.size() << std::endl;
            if (sequence.ticks.empty()) {
                std::cerr << "No ticks loaded from folder: " << tick_files[i].file_path << std::endl;
                return;
            }

            dfh::compression::test_encode_price_efficiency(
                encode_price_size,
                sequence.ticks,
                sequence.price_digits,
                sequence.volume_digits);

            encode_price_count++;
        });
    }


    for (size_t ver = 0; ver < encode_price_size.size(); ++ver) {
        int64_t aver_size = (int64_t)((double)encode_price_size[ver] / (double)encode_price_count);
        std::cout << "ver:           " << (ver + 1) << "\n";
        std::cout << "aver size:     " << aver_size <<  " bytes\n";
    }

    std::cout << "All tests completed!\n";
    return 0;
}
