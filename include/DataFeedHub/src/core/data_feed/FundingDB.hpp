#pragma once
#ifndef _DFH_CORE_FUNDING_DB_HPP_INCLUDED
#define _DFH_CORE_FUNDING_DB_HPP_INCLUDED

namespace dfh {

    /// \class FundingDB
    /// \brief Класс для работы с базой данных фандинга.
    class FundingDB {
    public:
        /// \brief Default constructor.
        FundingDB() = default;

        /// \brief Constructor with configuration.
        /// \param config Configuration settings for the database.
        explicit FundingDB(const Config& config) : BaseDB() {
            set_config(config);
        }

        /// \brief Destructor.
        ~FundingDB() override final = default;

        /// Метод для получения списка пар с противоположными ставками фандинга для указанного времени.
        std::vector<OppositeFundingPair> findOppositeFundingPairs(int64_t timestamp) {
            std::vector<OppositeFundingPair> result;
            try {
                stmt_find_opposite_rates.bind_value(1, timestamp);

                while (stmt_find_opposite_rates.step() == SQLITE_ROW) {
                    OppositeFundingPair pair = {
                        stmt_find_opposite_rates.extract_column<int32_t>(0),
                        stmt_find_opposite_rates.extract_column<int32_t>(1),
                        stmt_find_opposite_rates.extract_column<int32_t>(2),
                        stmt_find_opposite_rates.extract_column<int64_t>(3),
                        stmt_find_opposite_rates.extract_column<double>(4),
                        stmt_find_opposite_rates.extract_column<double>(5),
                        stmt_find_opposite_rates.extract_column<double>(6)
                    };
                    result.push_back(pair);
                }

                stmt_find_opposite_rates.reset();
                stmt_find_opposite_rates.clear_bindings();
            } catch (const sqlite_containers::sqlite_exception& e) {
                std::cerr << "Ошибка при поиске противоположных ставок фандинга: " << e.what() << std::endl;
            }

            return result;
        }


    private:
        // SQL запросы для вставки и получения данных
        sqlite_containers::SqliteStmt m_stmt_insert;
        sqlite_containers::SqliteStmt m_stmt_select;

        /// \brief Creates the main and temporary tables in the database.
        /// \param config Configuration settings for the database.
        void db_create_table(const sqlite_containers::Config& config) override final {
            const char* sql_create_table =
                "CREATE TABLE IF NOT EXISTS funding_data ("
                "provider_id INTEGER NOT NULL, "
                "symbol_id INTEGER NOT NULL, "
                "timestamp INTEGER NOT NULL, "
                "funding_rate REAL NOT NULL, "
                "PRIMARY KEY (provider_id, symbol_id, timestamp));";

            execute(m_sqlite_db, sql_create_table);

            // Подготавливаем запросы
            m_stmt_insert.init(m_sqlite_db, "INSERT OR REPLACE INTO funding_data (provider_id, symbol_id, timestamp, funding_rate) VALUES (?, ?, ?, ?);");
            m_stmt_select.init(m_sqlite_db, "SELECT provider_id, symbol_id, timestamp, funding_rate FROM funding_data WHERE provider_id = ? AND symbol_id = ? AND timestamp BETWEEN ? AND ?;");
        }

        /// \brief Inserts a key-value pair into the database.
        /// \param data
        /// \throws sqlite_exception if an SQLite error occurs.
        void db_insert(const FundingData& data) {
            try {
                m_stmt_insert.bind_value(1, data.provider_id);
                m_stmt_insert.bind_value(2, data.symbol_id);
                m_stmt_insert.bind_value(3, data.timestamp);
                m_stmt_insert.bind_value(4, data.funding_rate);
                m_stmt_insert.execute();
                m_stmt_insert.reset();
                m_stmt_insert.clear_bindings();
                return true;
            } catch (...) {
                db_handle_exception(
                    std::current_exception(),
                    {&m_stmt_insert},
                    "Unknown error occurred while inserting key-value pair into the database.");
            }
        }

        /// \brief Appends the content of the container to the database.
        /// \tparam ContainerT Template for the container type.
        /// \param container Container with content to
        /// \throws sqlite_exception if an SQLite error occurs.
        template<template <class...> class ContainerT>
        void db_append(const ContainerT<FundingData>& container) {
            for (const auto& item : container) {
                db_insert(item);
            }
        }

                // Получение данных о фандинге за указанный период
        std::vector<FundingData> get_funding_data(
                const int32_t& provider_id,
                const int32_t& symbol_id,
                const int64_t& start_time,
                const int64_t& end_time) {
            std::vector<FundingData> results;
            try {
                stmt_select.bind_value(1, provider_id);
                stmt_select.bind_value(2, symbol_id);
                stmt_select.bind_value(3, start_time);
                stmt_select.bind_value(4, end_time);

                while (stmt_select.step() == SQLITE_ROW) {
                    FundingData data(
                        stmt_select.extract_column<int32_t>(0),
                        stmt_select.extract_column<int32_t>(1),
                        stmt_select.extract_column<std::int64_t>(2),
                        stmt_select.extract_column<double>(3)
                    );
                    results.push_back(data);
                }

                stmt_select.reset();
                stmt_select.clear_bindings();
            } catch (const sqlite_containers::sqlite_exception& e) {
                std::cerr << "Ошибка при получении данных о фандинге: " << e.what() << std::endl;
            }

            return results;
        }

    };

}; // namespace dfh

#endif // _DFH_CORE_FUNDING_DB_HPP_INCLUDED
