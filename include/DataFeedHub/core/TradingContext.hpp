#pragma once
#ifndef _DTH_TRADINGCONTEXT_HPP_INCLUDED
#define _DTH_TRADING_CONTEXT_HPP_INCLUDED

/// \file TradingContext.hpp
/// \brief Управляет жизненным циклом источника данных, брокера и совокупности торговых ботов.

namespace dfh {

    /// \brief Контекст, который связывает DataFeed, IBroker и набор торговых ботов.
    class TradingContext {
    private:
        std::unique_ptr<DataFeed> data_feed;
        std::unique_ptr<IBroker> broker;
        std::vector<std::unique_ptr<BaseTradingBot>> bots;

    public:
        /// \brief Конструктор, принимающий владение источником данных и брокером.
        /// \param df Уникальный указатель на источник рыночной информации.
        /// \param br Уникальный указатель на объект IBroker.
        TradingContext(std::unique_ptr<DataFeed> df, std::unique_ptr<IBroker> br)
            : data_feed(std::move(df)), broker(std::move(br)) {}

        /// \brief Регистрирует бота, который будет получать события и запускаться вместе со стратегией.
        /// \param bot Уникальный указатель на бот.
        void add_bot(std::unique_ptr<BaseTradingBot> bot) {
            bots.push_back(std::move(bot));
        }

        /// \brief Запускает источник данных, затем всех зарегистрированных ботов.
        void start() {
            data_feed->start();
            for (auto& bot : bots) {
                bot->start();
            }
        }

        /// \brief Останавливает источник данных и равномерно завершает работу ботов.
        void stop() {
            data_feed->stop();
            for (auto& bot : bots) {
                bot->stop();
            }
        }
    };

}; // namespace dfh

#endif // _DTH_TRADING_CONTEXT_HPP_INCLUDED
