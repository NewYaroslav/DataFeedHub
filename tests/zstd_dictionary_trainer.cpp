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
#include <DataFeedHub/src/compression/utils/frequency_encoding.hpp>
#include <DataFeedHub/src/compression/utils/repeat_encoding.hpp>
#include <DataFeedHub/src/compression/utils/volume_scaling.hpp>
#include <DataFeedHub/src/compression/utils/zig_zag.hpp>
#include <DataFeedHub/src/compression/utils/zig_zag_delta.hpp>
#include <DataFeedHub/src/compression/ticks/TickCompressorV1/TickCompressionContextV1.hpp>
#include <DataFeedHub/src/compression/ticks/TickCompressorV1/TickEncoderV1.hpp>
#include <DataFeedHub/src/compression/ticks/TickCompressorV1/TickDecoderV1.hpp>
#include <DataFeedHub/src/compression/utils/zstd_utils.hpp>

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
                case ExchangeType::BYBIT_FUTURES: {
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
                case ExchangeType::BINANCE_SPOT: {
                        std::cout << "Decompressing the file: " << file << std::endl;
                        std::string decompressed_data = dfh::utils::extract_first_file_from_zip(content);
                        std::cout << "Decompressed data size: " << decompressed_data.size() << std::endl;
                        dfh::utils::parse_binance_spot_trades(sequence, decompressed_data, 3000000, true);
                        break;
                    }
                case ExchangeType::BINANCE_FUTURES: {
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

    class RawTickCompressorV1 {
    public:

        RawTickCompressorV1(
                uint64_t interval = time_shield::MS_PER_HOUR)
            : m_buffer(), m_encoder(m_buffer), m_decoder(m_buffer), m_interval_ms(interval) {
        }

        void set_config(const TickEncodingConfig &config) {
            m_config = config;
        }

        void compress(const std::vector<MarketTick>& ticks, std::vector<uint8_t>& output) {
            if (!m_config.enable_trade_based_encoding) throw std::invalid_argument("?");
            const size_t max_digits = 18;
            if (m_config.price_digits > max_digits || m_config.volume_digits > max_digits) {
                throw std::invalid_argument("Price or volume digits exceed maximum allowed digits.");
            }

            uint8_t header = 0x00;
            // Record the precision levels
            // Bits 0-4: Number of decimal places for the price
            // Bit 5: Flag indicating the use of tick flags
            // Bit 6: Flag indicating the use of real volume (double)
            header |= (m_config.price_digits & 0x1F);
            header |= (m_config.enable_tick_flags << 5) & 0x20;
            header |= (m_config.enable_trade_based_encoding << 6) & 0x40;
            output.push_back(header);

            // Bits 0-4: Number of decimal places for the volume
            header = 0x00;
            header |= (m_config.volume_digits & 0x1F);
            header |= (ticks[0].has_flag(TickUpdateFlags::LAST_UPDATED) << 5) & 0x20;
            output.push_back(header);

            uint64_t base_unix_hour = (ticks[0].time_ms / m_interval_ms);
            dfh::utils::append_vbyte<uint32_t>(output, base_unix_hour);

            const double price_scale = utils::pow10<double>(m_config.price_digits);
            const uint64_t initial_price = std::llround(ticks[0].last * price_scale);
            dfh::utils::append_vbyte<uint64_t>(output, initial_price);
            dfh::utils::append_vbyte<uint32_t>(output, ticks.size());

            m_encoder.encode_price_last(
                output,
                ticks.data(),
                ticks.size(),
                price_scale,
                initial_price);

            m_encoder.encode_volume(
                output,
                ticks.data(),
                ticks.size(),
                utils::pow10<double>(m_config.volume_digits));

            m_encoder.encode_time(
                output,
                ticks.data(),
                ticks.size(),
                base_unix_hour * m_interval_ms);

            if (m_config.enable_tick_flags) {
                m_encoder.encode_side_flags(output, ticks.data(), ticks.size());
            }
        }

        void decompress(const std::vector<uint8_t>& input, std::vector<MarketTick>& ticks) {
            size_t offset = 0;
            uint8_t header = input[offset++];
            m_config.price_digits = header & 0x1F;
            m_config.enable_tick_flags           = (header & 0x20) != 0;
            m_config.enable_trade_based_encoding = (header & 0x40) != 0;

            header = input[offset++];
            m_config.volume_digits  = header & 0x1F;
            const bool last_updated = (header & 0x20) != 0;

            uint64_t base_unix_hour = dfh::utils::extract_vbyte<uint32_t>(input.data(), offset);
            const uint64_t base_unix_time = base_unix_hour * m_interval_ms;
            const uint64_t initial_price = dfh::utils::extract_vbyte<uint64_t>(input.data(), offset);
            const size_t num_ticks = dfh::utils::extract_vbyte<uint32_t>(input.data(), offset);

            ticks.resize(num_ticks);

            m_decoder.decode_price_last(
                ticks.data(),
                input.data(),
                offset,
                num_ticks,
                utils::pow10<double>(m_config.price_digits),
                initial_price);

            m_decoder.decode_volume(
                ticks.data(),
                input.data(),
                offset,
                num_ticks,
                utils::pow10<double>(m_config.volume_digits));

            m_decoder.decode_time(
                ticks.data(),
                input.data(),
                offset,
                num_ticks,
                base_unix_time);

            if (m_config.enable_tick_flags) {
                m_decoder.decode_side_flags(
                    ticks.data(),
                    input.data(),
                    offset,
                    num_ticks);
                if (last_updated) {
                    ticks[0].set_flag(TickUpdateFlags::LAST_UPDATED);
                }
            }
        }

    private:
        TickCompressionContextV1 m_buffer;
        TickEncoderV1         m_encoder;
        TickDecoderV1         m_decoder;
        TickEncodingConfig    m_config;      ///< The encoding configuration.
        uint64_t              m_interval_ms; ///< The time interval for tick data segmentation in milliseconds.
    };

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

    void trim_samples_to_limit(std::vector<std::vector<uint8_t>>& samples, size_t max_size = 4ULL * 1024ULL * 1024ULL * 1024ULL) {
        // Подсчитаем общий размер образцов
        size_t total_size = std::accumulate(samples.begin(), samples.end(), size_t(0),
            [](size_t sum, const std::vector<uint8_t>& sample) {
                return sum + sample.size();
            });

        // Если общий размер превышает лимит, нужно удалить образцы
        while (total_size > max_size) {
            size_t middle_index = samples.size() / 2;
            total_size -= samples[middle_index].size();
            samples.erase(samples.begin() + middle_index);
        }

        std::cout << "Total size after trimming: " << total_size << " bytes" << std::endl;
    }

    void split_samples_by_size(
            const std::vector<std::vector<uint8_t>>& samples,
            std::vector<std::vector<uint8_t>>& small_samples,
            std::vector<std::vector<uint8_t>>& medium_samples,
            std::vector<std::vector<uint8_t>>& large_samples,
            size_t small_threshold = 64* 1024,
            size_t large_threshold = 2 * 1024 * 1024) {
        for (const auto& sample : samples) {
            if (sample.size() < small_threshold) {
                small_samples.push_back(sample);
            } else
            if (sample.size() <= large_threshold) {
                medium_samples.push_back(sample);
            } else {
                large_samples.push_back(sample);
            }
        }
    }

}

int main() {

    std::vector<dfh::compression::TickFileConfig> tick_files = {
        {"..\\..\\dataset\\zstd\\trades\\train\\binance\\spot", dfh::compression::ExchangeType::BINANCE_SPOT, 0, 0, true},
        {"..\\..\\dataset\\zstd\\trades\\train\\binance\\futures2", dfh::compression::ExchangeType::BINANCE_FUTURES, 0, 0, true},
        {"..\\..\\dataset\\zstd\\trades\\train\\bybit\\futures", dfh::compression::ExchangeType::BYBIT_FUTURES, 0, 0, true},
    };

    std::vector<size_t> dict_sizes = {
        10 * 1024,
        25 * 1024,
        50 * 1024,
        75 * 1024,
        100 * 1024,
        125 * 1024,
        150 * 1024,
        200 * 1024,
        250 * 1024,
        300 * 1024,
        400 * 1024,
        500 * 1024,
        750 * 1024,
        1000 * 1024,
        1500 * 1024,
        2000 * 1024,
        5000 * 1024,
        10000 * 1024
    };
    std::vector<std::vector<uint8_t>> samples;

    // Загружаем данные
    for (const auto& config : tick_files) {
        load_ticks_from_folder(config.file_path, config.exchange,
                [&](dfh::MarketTickSequence& sequence) {
            std::cout << "Ticks size: " << sequence.ticks.size() << std::endl;
            if (sequence.ticks.empty()) {
                std::cerr << "No ticks loaded from folder: " << config.file_path << std::endl;
                return;
            }

            dfh::TickEncodingConfig compressor_config;
            compressor_config.enable_tick_flags = true;
            compressor_config.enable_trade_based_encoding = true;
            compressor_config.price_digits = sequence.price_digits;
            compressor_config.volume_digits = sequence.volume_digits;
            compressor_config.fixed_spread = 1;

            dfh::compression::RawTickCompressorV1 compressor;
            compressor.set_config(compressor_config);

            std::vector<dfh::MarketTick> ticks;
            int last_hour = time_shield::hour_of_day(time_shield::ms_to_sec(sequence.ticks[0].time_ms));
            for (const auto& tick : sequence.ticks) {
                int tick_hour = time_shield::hour_of_day(time_shield::ms_to_sec(tick.time_ms));
                if (tick_hour != last_hour) {
                    last_hour = tick_hour;

                    std::vector<uint8_t> compressed_data;
                    compressor_config.enable_tick_flags = ((ticks[0].time_ms % 2) == 0);
                    compressor.set_config(compressor_config);
                    compressor.compress(ticks, compressed_data);
                    samples.push_back(std::move(compressed_data));
                    ticks.clear();
                }
                ticks.push_back(tick);
            }
            if (!ticks.empty()) {
                std::vector<uint8_t> compressed_data;
                compressor_config.enable_tick_flags = ((ticks[0].time_ms % 2) == 0);
                compressor.set_config(compressor_config);
                compressor.compress(ticks, compressed_data);
                samples.push_back(std::move(compressed_data));
            }
        });
    }

    dfh::compression::find_min_max_sample_sizes(samples);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(samples.begin(), samples.end(), gen);

    std::vector<std::vector<uint8_t>> small_samples;
    std::vector<std::vector<uint8_t>> medium_samples;
    std::vector<std::vector<uint8_t>> large_samples;

    dfh::compression::split_samples_by_size(
        samples,
        small_samples,
        medium_samples,
        large_samples,
        64* 1024,
        2 * 1024 * 1024);

    size_t test_sample_size = static_cast<size_t>(std::ceil(samples.size() * 0.2));
    std::vector<std::vector<uint8_t>> test_samples(
        samples.end() - test_sample_size, samples.end());
    samples.resize(samples.size() - test_sample_size);

    size_t test_small_sample_size = static_cast<size_t>(std::ceil(small_samples.size() * 0.2));
    std::vector<std::vector<uint8_t>> test_small_samples(
        small_samples.end() - test_small_sample_size, small_samples.end());
    small_samples.resize(small_samples.size() - test_small_sample_size);

    size_t test_medium_sample_size = static_cast<size_t>(std::ceil(medium_samples.size() * 0.2));
    std::vector<std::vector<uint8_t>> test_medium_samples(
        medium_samples.end() - test_medium_sample_size, medium_samples.end());
    medium_samples.resize(medium_samples.size() - test_medium_sample_size);

    size_t test_large_sample_size = static_cast<size_t>(std::ceil(large_samples.size() * 0.2));
    std::vector<std::vector<uint8_t>> test_large_samples(
        large_samples.end() - test_large_sample_size, large_samples.end());
    large_samples.resize(large_samples.size() - test_large_sample_size);

    dfh::compression::trim_samples_to_limit(samples);
    dfh::compression::trim_samples_to_limit(small_samples);
    dfh::compression::trim_samples_to_limit(medium_samples);
    dfh::compression::trim_samples_to_limit(large_samples);

    std::cout << "Training samples: " << samples.size() << ", Test samples: " << test_samples.size() << std::endl;
    std::cout << "Training small samples: " << small_samples.size() << ", Test samples: " << test_small_samples.size() << std::endl;
    std::cout << "Training medium samples: " << medium_samples.size() << ", Test samples: " << test_medium_samples.size() << std::endl;
    std::cout << "Training large samples: " << large_samples.size() << ", Test samples: " << test_large_samples.size() << std::endl;

    for (size_t dict_size : dict_sizes) {
        std::cout << "Training dictionary with size: " << dict_size << std::endl;

        {
            // 4. Обучаем словарь
            auto dictionary = dfh::compression::train_zstd(samples, dict_size);

            // 5. Оцениваем эффективность сжатия
            uint64_t original_size = 0;
            uint64_t compressed_size = 0;
            uint64_t compression_time = 0;
            uint64_t decompression_time = 0;


            for (const auto& sample : test_samples) {
                original_size += sample.size();

                std::vector<uint8_t> compressed_sample;
                auto compression_start = std::chrono::high_resolution_clock::now();
                dfh::compression::compress_zstd_data(
                    sample.data(), sample.size(),
                    dictionary.data(), dictionary.size(),
                    compressed_sample);
                auto compression_end = std::chrono::high_resolution_clock::now();
                compression_time += std::chrono::duration_cast<std::chrono::microseconds>(compression_end - compression_start).count();

                compressed_size += compressed_sample.size();

                std::vector<uint8_t> decompressed_sample;
                auto decompression_start = std::chrono::high_resolution_clock::now();
                dfh::compression::decompress_zstd_data(
                    compressed_sample.data(), compressed_sample.size(),
                    dictionary.data(), dictionary.size(),
                    decompressed_sample);
                auto decompression_end = std::chrono::high_resolution_clock::now();
                decompression_time += std::chrono::duration_cast<std::chrono::microseconds>(decompression_end - decompression_start).count();

                if (sample != decompressed_sample) {
                    std::cout << "Error decompression!" << std::endl;
                }
            }

            double compression_ratio = static_cast<double>(original_size) / static_cast<double>(compressed_size);

            std::cout
                << "one comp ratio: " << compression_ratio
                << "; size: " << (compressed_size / test_samples.size())
                << "; time: " << (compression_time / test_samples.size()) << "us"
                << "; dec time: " << (decompression_time / test_samples.size())
                << "us" << std::endl;

            // 6. Сохраняем словарь
            std::string dict_name = "zstd_one_dict_" + std::to_string(dict_size);
            std::string dict_file_path = dict_name + ".hpp";

            dfh::compression::save_binary_as_header(dictionary, dict_name, dict_file_path);
            std::cout << "dictionary saved to: " << dict_file_path << std::endl;
        }

        {
            // 4. Обучаем словарь
            auto dictionary = dfh::compression::train_zstd(small_samples, dict_size);

            // 5. Оцениваем эффективность сжатия
            uint64_t original_size = 0;
            uint64_t compressed_size = 0;
            uint64_t compression_time = 0;
            uint64_t decompression_time = 0;

            for (const auto& sample : test_small_samples) {
                original_size += sample.size();

                std::vector<uint8_t> compressed_sample;
                auto compression_start = std::chrono::high_resolution_clock::now();
                dfh::compression::compress_zstd_data(
                    sample.data(), sample.size(),
                    dictionary.data(), dictionary.size(),
                    compressed_sample);
                auto compression_end = std::chrono::high_resolution_clock::now();
                compression_time += std::chrono::duration_cast<std::chrono::microseconds>(compression_end - compression_start).count();

                compressed_size += compressed_sample.size();

                std::vector<uint8_t> decompressed_sample;
                auto decompression_start = std::chrono::high_resolution_clock::now();
                dfh::compression::decompress_zstd_data(
                    compressed_sample.data(), compressed_sample.size(),
                    dictionary.data(), dictionary.size(),
                    decompressed_sample);
                auto decompression_end = std::chrono::high_resolution_clock::now();
                decompression_time += std::chrono::duration_cast<std::chrono::microseconds>(decompression_end - decompression_start).count();

                if (sample != decompressed_sample) {
                    std::cout << "Error decompression!" << std::endl;
                }
            }

            double compression_ratio = static_cast<double>(original_size) / static_cast<double>(compressed_size);

            std::cout
                << "small comp ratio: " << compression_ratio
                << "; size: " << (compressed_size / test_small_samples.size())
                << "; time: " << (compression_time / test_small_samples.size()) << "us"
                << "; dec time: " << (decompression_time / test_small_samples.size())
                << "us" << std::endl;

            // 6. Сохраняем словарь
            std::string dict_name = "zstd_small_dict_" + std::to_string(dict_size);
            std::string dict_file_path = dict_name + ".hpp";

            dfh::compression::save_binary_as_header(dictionary, dict_name, dict_file_path);
            std::cout << "dictionary saved to: " << dict_file_path << std::endl;
        }

        {
            // 4. Обучаем словарь
            auto dictionary = dfh::compression::train_zstd(medium_samples, dict_size);

            // 5. Оцениваем эффективность сжатия
            uint64_t original_size = 0;
            uint64_t compressed_size = 0;
            uint64_t compression_time = 0;
            uint64_t decompression_time = 0;

            for (const auto& sample : test_medium_samples) {
                original_size += sample.size();

                std::vector<uint8_t> compressed_sample;
                auto compression_start = std::chrono::high_resolution_clock::now();
                dfh::compression::compress_zstd_data(
                    sample.data(), sample.size(),
                    dictionary.data(), dictionary.size(),
                    compressed_sample);
                auto compression_end = std::chrono::high_resolution_clock::now();
                compression_time += std::chrono::duration_cast<std::chrono::microseconds>(compression_end - compression_start).count();

                compressed_size += compressed_sample.size();

                std::vector<uint8_t> decompressed_sample;
                auto decompression_start = std::chrono::high_resolution_clock::now();
                dfh::compression::decompress_zstd_data(
                    compressed_sample.data(), compressed_sample.size(),
                    dictionary.data(), dictionary.size(),
                    decompressed_sample);
                auto decompression_end = std::chrono::high_resolution_clock::now();
                decompression_time += std::chrono::duration_cast<std::chrono::microseconds>(decompression_end - decompression_start).count();

                if (sample != decompressed_sample) {
                    std::cout << "Error decompression!" << std::endl;
                }
            }

            double compression_ratio = static_cast<double>(original_size) / static_cast<double>(compressed_size);

            std::cout
                << "medium comp ratio: " << compression_ratio
                << "; size: " << (compressed_size / test_medium_samples.size())
                << "; time: " << (compression_time / test_medium_samples.size()) << "us"
                << "; dec time: " << (decompression_time / test_medium_samples.size())
                << "us" << std::endl;

            // 6. Сохраняем словарь
            std::string dict_name = "zstd_medium_dict_" + std::to_string(dict_size);
            std::string dict_file_path = dict_name + ".hpp";

            dfh::compression::save_binary_as_header(dictionary, dict_name, dict_file_path);
            std::cout << "dictionary saved to: " << dict_file_path << std::endl;
        }

        {
            // 4. Обучаем словарь
            auto dictionary = dfh::compression::train_zstd(large_samples, dict_size);

            // 5. Оцениваем эффективность сжатия
            uint64_t original_size = 0;
            uint64_t compressed_size = 0;
            uint64_t compression_time = 0;
            uint64_t decompression_time = 0;

            for (const auto& sample : test_large_samples) {
                original_size += sample.size();

                std::vector<uint8_t> compressed_sample;
                auto compression_start = std::chrono::high_resolution_clock::now();
                dfh::compression::compress_zstd_data(
                    sample.data(), sample.size(),
                    dictionary.data(), dictionary.size(),
                    compressed_sample);
                auto compression_end = std::chrono::high_resolution_clock::now();
                compression_time += std::chrono::duration_cast<std::chrono::microseconds>(compression_end - compression_start).count();

                compressed_size += compressed_sample.size();

                std::vector<uint8_t> decompressed_sample;
                auto decompression_start = std::chrono::high_resolution_clock::now();
                dfh::compression::decompress_zstd_data(
                    compressed_sample.data(), compressed_sample.size(),
                    dictionary.data(), dictionary.size(),
                    decompressed_sample);
                auto decompression_end = std::chrono::high_resolution_clock::now();
                decompression_time += std::chrono::duration_cast<std::chrono::microseconds>(decompression_end - decompression_start).count();

                if (sample != decompressed_sample) {
                    std::cout << "Error decompression!" << std::endl;
                }
            }

            double compression_ratio = static_cast<double>(original_size) / static_cast<double>(compressed_size);

            std::cout
                << "large comp ratio: " << compression_ratio
                << "; size: " << (compressed_size / test_large_samples.size())
                << "; time: " << (compression_time / test_large_samples.size()) << "us"
                << "; dec time: " << (decompression_time / test_large_samples.size())
                << "us" << std::endl;

            // 6. Сохраняем словарь
            std::string dict_name = "zstd_large_dict_" + std::to_string(dict_size);
            std::string dict_file_path = dict_name + ".hpp";

            dfh::compression::save_binary_as_header(dictionary, dict_name, dict_file_path);
            std::cout << "dictionary saved to: " << dict_file_path << std::endl;
        }
    }

    return EXIT_SUCCESS;
}

