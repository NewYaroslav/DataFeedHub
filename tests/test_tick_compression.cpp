#include <iostream>
#include <vector>
#include <cassert>
#include <random>
#include <ctime>
#include <time_shield_cpp/time_shield.hpp>
#include <DataFeedHub/src/structures/ticks/flags.hpp>
#include <DataFeedHub/src/structures/ticks/MarketTick.hpp>
#include <DataFeedHub/src/structures/ticks/TickEncodingConfig.hpp>
#include <DataFeedHub/src/utils/math_utils.hpp>
#include <DataFeedHub/src/utils/fixed_point.hpp>
#include <DataFeedHub/src/utils/aligned_allocator.hpp>
#include <DataFeedHub/src/utils/sse_double_int64_utils.hpp>
#include <DataFeedHub/src/utils/vbyte.hpp>
#include <DataFeedHub/src/utils/simdcomp.hpp>
#include <DataFeedHub/src/compression/utils/frequency_encoding.hpp>
#include <DataFeedHub/src/compression/utils/repeat_encoding.hpp>
#include <DataFeedHub/src/compression/utils/volume_scaling.hpp>
#include <DataFeedHub/src/compression/utils/zig_zag.hpp>
#include <DataFeedHub/src/compression/utils/zig_zag_delta.hpp>
#include <DataFeedHub/src/compression/ticks/TickCompressorV1/TickCompressionContextV1.hpp>
#include <DataFeedHub/src/compression/ticks/TickCompressorV1/TickEncoderV1.hpp>
#include <DataFeedHub/src/compression/ticks/TickCompressorV1/TickDecoderV1.hpp>


namespace dfh::compression {

    std::vector<MarketTick> generate_random_ticks(
            size_t count,
            double initial_price,
            int64_t price_change_size,
            int64_t volume_change_size,
            size_t price_digits,
            size_t volume_digits) {
        std::vector<MarketTick> ticks;
        ticks.reserve(count);

        const double price_scale = dfh::utils::pow10<double>(price_digits);
        const double volume_scale = dfh::utils::pow10<double>(volume_digits);

        const double inv_price_scale = 1.0 / price_scale;
        const double inv_volume_scale = 1.0 / volume_scale;

        std::random_device rd;
        //std::mt19937 gen(123124124);
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int64_t> time_increment_dist(0, 60000); // Time increment [0, 200] ms
        std::uniform_int_distribution<int64_t> price_change_dist(-price_change_size, price_change_size); // Price change
        std::uniform_int_distribution<int64_t> volume_change_dist(0, volume_change_size);
        std::bernoulli_distribution side_dist(0.5); // Randomly select TICK_FROM_BUY or TICK_FROM_SELL

        uint64_t current_time = time_shield::MS_PER_HOUR;
        double last_price = initial_price;

        for (size_t i = 0; i < count; ++i) {
            MarketTick tick;

            // Set time_ms
            uint64_t time_increment = time_increment_dist(gen);
            current_time += time_increment;
            tick.time_ms = current_time;

            // Generate price
            double price_change = (double)price_change_dist(gen) * inv_price_scale;
            double new_price = std::max(0.0, last_price + price_change);
            tick.last = new_price;

            // Generate volume
            tick.volume = (double)volume_change_dist(gen) * inv_volume_scale;

            // Set flags
            tick.flags = 0;
            if (new_price != last_price) {
                tick.set_flag(TickUpdateFlags::LAST_UPDATED);
            }
            if (side_dist(gen)) {
                tick.set_flag(TickUpdateFlags::TICK_FROM_BUY);
            } else {
                tick.set_flag(TickUpdateFlags::TICK_FROM_SELL);
            }

            // Push tick to vector
            ticks.push_back(tick);
            last_price = new_price;
        }

        return ticks;
    }

    // Compares prices of two tick arrays
    bool compare_tick_prices(
            const std::vector<MarketTick>& ticks1,
            const std::vector<MarketTick>& ticks2,
            size_t price_digits) {
        if (ticks1.size() != ticks2.size()) return false;
        const double price_scale = dfh::utils::pow10<double>(price_digits);
        const double price_tolerance = dfh::utils::precision_tolerance(price_digits);
        for (size_t i = 0; i < ticks1.size(); ++i) {
            if (std::fabs(ticks1[i].last - ticks2[i].last) >= price_tolerance) { // Allow small floating-point error
                std::cout << "Price mismatch at index " << i
                          << ": ticks1.last = " << std::llround(ticks1[i].last * price_scale)
                          << ", ticks2.last = " << std::llround(ticks2[i].last * price_scale) << std::endl;
                return false;
            }
        }
        return true;
    }

    bool compare_tick_volumes(
            const std::vector<MarketTick>& ticks1,
            const std::vector<MarketTick>& ticks2,
            size_t volume_digits) {
        if (ticks1.size() != ticks2.size()) return false;
        const double volume_scale = dfh::utils::pow10<double>(volume_digits);
        const double volume_tolerance = dfh::utils::precision_tolerance(volume_digits);
        for (size_t i = 0; i < ticks1.size(); ++i) {
            if (std::fabs(ticks1[i].volume - ticks2[i].volume) >= volume_tolerance) {
                std::cout << "Volume mismatch at index " << i
                          << ": ticks1[i].volume = " << std::llround(ticks1[i].volume * volume_scale)
                          << ", ticks2[i].volume = " << std::llround(ticks2[i].volume * volume_scale)<< std::endl;
                return false;
            }
        }
        return true;
    }

    // Compares timestamps of two tick arrays
    bool compare_tick_time(const std::vector<MarketTick>& ticks1, const std::vector<MarketTick>& ticks2) {
        if (ticks1.size() != ticks2.size()) {
            return false;
        }
        for (size_t i = 0; i < ticks1.size(); ++i) {
            if (ticks1[i].time_ms != ticks2[i].time_ms) {
                std::cout << "Time mismatch at index " << i
                          << ": ticks1.time_ms = " << ticks1[i].time_ms
                          << ", ticks2.time_ms = " << ticks2[i].time_ms << std::endl;
                return false;
            }
        }
        return true;
    }

    // Compares flags of two tick arrays
    bool compare_tick_side_flags(const std::vector<MarketTick>& ticks1, const std::vector<MarketTick>& ticks2) {
        if (ticks1.size() != ticks2.size()) return false;
        for (size_t i = 0; i < ticks1.size(); ++i) {
            if (ticks1[i].has_flag(TickUpdateFlags::TICK_FROM_BUY) != ticks2[i].has_flag(TickUpdateFlags::TICK_FROM_BUY)) {
                std::cout << "Flags mismatch at index " << i
                          << ": ticks1.flags = " << ticks1[i].flags
                          << ", ticks2.flags = " << ticks2[i].flags << std::endl;
                return false;
            }
            if (ticks1[i].has_flag(TickUpdateFlags::TICK_FROM_SELL) != ticks2[i].has_flag(TickUpdateFlags::TICK_FROM_SELL)) {
                std::cout << "Flags mismatch at index " << i
                          << ": ticks1.flags = " << ticks1[i].flags
                          << ", ticks2.flags = " << ticks2[i].flags << std::endl;
                return false;
            }
        }
        return true;
    }

    void test_encode_decode_price_last(int64_t price_change_size = 1000) {
        std::cout << "[Testing encode and decode for price_last]\n";
        std::cout << "price_change_size " << price_change_size << std::endl;

        TickCompressionContextV1 buffer;
        TickEncoderV1 encoder(buffer);
        TickDecoderV1 decoder(buffer);

        size_t price_digits = 3;
        size_t volume_digits = 3;
        int64_t volume_change_size = 1000;

        const double price_scale = dfh::utils::pow10<double>(price_digits);

        std::vector<dfh::MarketTick> ticks = generate_random_ticks(
            1000,
            10000.0,
            price_change_size,
            volume_change_size,
            price_digits,
            volume_digits);

        int64_t initial_price = std::llround(ticks[0].last * price_scale);

        std::vector<uint8_t> compressed;
        encoder.encode_price_last(
            compressed,
            ticks.data(),
            ticks.size(),
            price_scale,
            initial_price
        );

        std::vector<MarketTick> decompressed(ticks.size());
        size_t offset = 0;
        decoder.decode_price_last(
            decompressed.data(),
            compressed.data(),
            offset,
            ticks.size(),
            price_scale,
            initial_price
        );

        assert(decompressed.size() == ticks.size());
        assert(compare_tick_prices(ticks, decompressed, price_digits));

        std::cout << "Price_last test passed!\n";
    }

    void test_encode_decode_volume(int64_t volume_change_size = 1000) {
        std::cout << "[Testing encode and decode for volume]\n";
        std::cout << "volume_change_size " << volume_change_size << std::endl;

        TickCompressionContextV1 buffer;
        TickEncoderV1 encoder(buffer);
        TickDecoderV1 decoder(buffer);

        size_t price_digits = 3;
        size_t volume_digits = 3;
        int64_t price_change_size = 1000;

        const double volume_scale = dfh::utils::pow10<double>(volume_digits);

        std::vector<dfh::MarketTick> ticks = generate_random_ticks(
            1000,
            10000.0,
            price_change_size,
            volume_change_size,
            price_digits,
            volume_digits);

        std::vector<uint8_t> compressed;
        encoder.encode_volume(
            compressed,
            ticks.data(),
            ticks.size(),
            volume_scale
        );

        std::vector<MarketTick> decompressed(ticks.size());
        size_t offset = 0;
        decoder.decode_volume(
            decompressed.data(),
            compressed.data(),
            offset,
            ticks.size(),
            volume_scale
        );

        assert(decompressed.size() == ticks.size());
        assert(compare_tick_volumes(ticks, decompressed, volume_digits));

        std::cout << "Volume test passed!\n";
    }

    void test_encode_decode_time() {
        std::cout << "[Testing encode and decode for time]\n";

        TickCompressionContextV1 buffer;
        TickEncoderV1 encoder(buffer);
        TickDecoderV1 decoder(buffer);

        size_t price_digits = 3;
        size_t volume_digits = 3;

        int64_t price_change_size = 1000;
        int64_t volume_change_size = 1000;

        std::vector<dfh::MarketTick> ticks = generate_random_ticks(
            1000,
            10000.0,
            price_change_size,
            volume_change_size,
            price_digits,
            volume_digits);

        int64_t base_time = time_shield::MS_PER_HOUR;

        std::vector<uint8_t> compressed;
        encoder.encode_time(
            compressed,
            ticks.data(),
            ticks.size(),
            base_time
        );

        std::vector<MarketTick> decompressed(ticks.size());
        size_t offset = 0;
        decoder.decode_time(
            decompressed.data(),
            compressed.data(),
            offset,
            ticks.size(),
            base_time
        );

        assert(decompressed.size() == ticks.size());
        assert(compare_tick_time(ticks, decompressed));

        std::cout << "Time test passed!\n";
    }

    void test_encode_decode_side_flags() {
        std::cout << "[Testing encode and decode for side_flags]\n";

        TickCompressionContextV1 buffer;
        TickEncoderV1 encoder(buffer);
        TickDecoderV1 decoder(buffer);

        size_t price_digits = 3;
        size_t volume_digits = 3;

        int64_t price_change_size = 1000;
        int64_t volume_change_size = 1000;

        std::vector<dfh::MarketTick> ticks = generate_random_ticks(
            1000,
            10000.0,
            price_change_size,
            volume_change_size,
            price_digits,
            volume_digits);

        std::vector<uint8_t> compressed;
        encoder.encode_side_flags(
            compressed,
            ticks.data(),
            ticks.size()
        );

        std::vector<MarketTick> decompressed(ticks.size());
        size_t offset = 0;
        decoder.decode_side_flags(
            decompressed.data(),
            compressed.data(),
            offset,
            ticks.size()
        );

        assert(decompressed.size() == ticks.size());
        assert(compare_tick_side_flags(ticks, decompressed));

        std::cout << "Side_flags test passed!\n";
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

    void test_tick_compressor_v1(int64_t price_change_size = 1000, int64_t volume_change_size = 1000) {
        std::cout << "[Testing TickCompressorV1]\n";

        // Параметры тестирования
        size_t price_digits = 3;
        size_t volume_digits = 3;

        TickEncodingConfig config;
        config.enable_tick_flags = true;
        config.enable_trade_based_encoding = true;
        config.price_digits = price_digits;
        config.volume_digits = volume_digits;
        config.fixed_spread = 1;
        RawTickCompressorV1 compressor;
        compressor.set_config(config);

        // Генерация случайных тиков
        std::vector<dfh::MarketTick> ticks = generate_random_ticks(
            1000,
            10000.0, // Начальная цена
            price_change_size,
            volume_change_size,
            price_digits,
            volume_digits
        );

        // Сжатие тиков
        std::vector<uint8_t> compressed_data;
        compressor.compress(ticks, compressed_data);

        // Распаковка тиков
        std::vector<dfh::MarketTick> decompressed_ticks;
        compressor.decompress(compressed_data, decompressed_ticks);

        // Проверки
        assert(decompressed_ticks.size() == ticks.size());
        assert(compare_tick_prices(ticks, decompressed_ticks, price_digits));
        assert(compare_tick_volumes(ticks, decompressed_ticks, volume_digits));
        assert(compare_tick_time(ticks, decompressed_ticks));
        assert(compare_tick_side_flags(ticks, decompressed_ticks));

        std::cout << "TickCompressorV1 test passed!\n";
    }

} // namespace dfh::compression

int main() {
    dfh::compression::test_encode_decode_price_last();
    dfh::compression::test_encode_decode_volume();
    dfh::compression::test_encode_decode_time();
    dfh::compression::test_encode_decode_side_flags();

    dfh::compression::test_encode_decode_price_last(0xFFFFFFFF);
    dfh::compression::test_encode_decode_volume(0xFFFFFFFF);

    dfh::compression::test_tick_compressor_v1();
    dfh::compression::test_tick_compressor_v1(0xFFFFFFFF, 0xFFFFFFFF);

    std::cout << "All tests passed!\n";
    return 0;
}
