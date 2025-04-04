#include <iostream>
#include <cassert>
#include <DataFeedHub/storage.hpp>

/// \brief Compares two bars for equality.
bool bar_equal(const dfh::MarketBar& a, const dfh::MarketBar& b) {
    return a.time_ms == b.time_ms &&
           a.open == b.open &&
           a.high == b.high &&
           a.low == b.low &&
           a.close == b.close &&
           a.volume == b.volume &&
           a.quote_volume == b.quote_volume &&
           a.buy_volume == b.buy_volume &&
           a.buy_quote_volume == b.buy_quote_volume &&
           a.spread == b.spread &&
           a.tick_volume == b.tick_volume;
}

/// \brief Generates sample bars for a given day.
/// \param y Год.
/// \param m Месяц.
/// \param d День.
/// \param count Кол-во баров.
/// \return Вектор с барами.
std::vector<dfh::MarketBar> generate_bars(int y, int m, int d, size_t count) {
    std::vector<dfh::MarketBar> bars;
    for (size_t i = 0; i < count; ++i) {
        uint64_t ts = time_shield::ts_ms(y, m, d, 0, 0, 0, 0);
        ts += i * time_shield::MS_PER_1_MIN;
        bars.emplace_back(ts, 1.0 + i, 1.1 + i, 0.9 + i, 1.05 + i,
                          100 + i, 200 + i, 50 + i, 80 + i, i, i);
    }
    return bars;
}

int main() {
    dfh::storage::mdbx::MDBXConfig config;
    //config.pathname = "./test-db";
    config.pathname = "test-db";
    auto connection = dfh::storage::create_connection(std::move(config));

    std::cout << "connect" << std::endl;
    connection->connect();

    dfh::storage::MarketDataStorageHub hub;
    hub.add_storage(dfh::storage::create_storage(connection));

    std::cout << "start" << std::endl;
    hub.start();

    dfh::BarCodecConfig codec;
    codec.time_frame = dfh::TimeFrame::M1;
    codec.tick_size = 0.001;
    codec.price_digits = 5;
    codec.volume_digits = 2;
    codec.quote_volume_digits = 2;
    codec.flags |= dfh::BarStorageFlags::STORE_RAW_BINARY;
    codec.flags |= dfh::BarStorageFlags::LAST_BASED;
    codec.flags |= dfh::BarStorageFlags::ENABLE_VOLUME;
    codec.flags |= dfh::BarStorageFlags::FINALIZED_BARS;

    struct SymbolInfo {
        dfh::MarketType market_type;
        uint16_t exchange_id;
        uint16_t symbol_id;
        int day;
    };

    std::vector<SymbolInfo> symbols = {
        { dfh::MarketType::SPOT, 1, 100, 1 },
        { dfh::MarketType::SPOT, 1, 101, 2 },
        { dfh::MarketType::FUTURES_PERPETUAL_LINEAR, 1, 102, 1 }
    };

    // Запись метаданных БД
    std::cout << "metadata" << std::endl;
    {
        dfh::storage::StorageMetadata metadata;
        metadata.data_flags = dfh::storage::StorageDataFlags::BARS;
        for (const auto& s : symbols) {
            metadata.add_market_type(s.market_type);
            metadata.add_exchange_id(s.exchange_id);
            metadata.add_symbol_id(s.symbol_id);
        }

        auto tx_guard = hub.transaction(dfh::storage::TransactionMode::WRITABLE);

        tx_guard->begin();
        hub.extend_metadata(tx_guard, 0, metadata);
        tx_guard->commit();
    }

    std::cout << "upsert bars" << std::endl;
    {
        // Запись баров
        auto tx_guard = hub.transaction(dfh::storage::TransactionMode::WRITABLE);
        tx_guard->begin();
        for (const auto& s : symbols) {
            auto bars = generate_bars(2025, 4, s.day, 1440);
            hub.upsert(tx_guard, s.market_type, s.exchange_id, s.symbol_id, bars, codec);
        }
        tx_guard->commit();
    }

    std::cout << "fetch bars" << std::endl;
    // Чтение и проверка
    for (const auto& s : symbols) {
        auto original = generate_bars(2025, 4, s.day, 1440);
        std::vector<dfh::MarketBar> read;
        dfh::BarCodecConfig read_codec;

        uint64_t start_ts = time_shield::ts_ms(2025, 4, s.day);
        uint64_t end_ts   = time_shield::ts_ms(2025, 5, s.day);

        auto tx_guard = hub.transaction(dfh::storage::TransactionMode::READ_ONLY);
        tx_guard->begin();
        bool ok = hub.fetch(tx_guard, s.market_type, s.exchange_id, s.symbol_id,
                            codec.time_frame, start_ts, end_ts, read, read_codec);
        tx_guard->commit();

        assert(ok && "Bars not fetched");

        std::cout << "read.size() " << read.size() << std::endl;
        assert(read.size() == original.size());

        for (size_t i = 0; i < read.size(); ++i) {
            assert(bar_equal(read[i], original[i]) && "Bar mismatch");
        }
    }

    std::cout << "stop" << std::endl;
    hub.stop();
    std::cout << "All tests passed." << std::endl;
    return 0;
}
