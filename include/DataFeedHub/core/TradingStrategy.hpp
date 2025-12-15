#pragma once
#ifndef _DTH_TRADING_STRATEGY_HPP_INCLUDED
#define _DTH_TRADING_STRATEGY_HPP_INCLUDED

/// \file TradingStrategy.hpp
/// \brief Собирает несколько торговых ботов в единую стратегию и транслирует события.

namespace dfh {

    /// \brief Компонент, объединяющий несколько BaseTradingBot и перенаправляющий им команды.
    class TradingStrategy : public BaseTradingBot {
    private:
        std::vector<std::unique_ptr<BaseTradingBot>> bots;

    public:
        /// \brief Добавляет дочерний бот и передаёт ему владение.
        /// \param bot Уникальный указатель на новый бот.
        void add_bot(std::unique_ptr<BaseTradingBot> bot) {
            bots.push_back(std::move(bot));
        }

        /// \brief Рассылает входящее событие всем вложенным ботам.
        /// \param event Событие, которое необходимо обработать.
        void on_event(const BaseEvent& event) override {
            for (auto& bot : bots) {
                bot->on_event(event);
            }
        }

        /// \brief Запускает все вложенные боты последовательно.
        void start() override {
            for (auto& bot : bots) {
                bot->start();
            }
        }

        /// \brief Останавливает всех вложенных ботов.
        void stop() override {
            for (auto& bot : bots) {
                bot->stop();
            }
        }
    };

}; // namespace dfh

#endif // _DTH_TRADING_STRATEGY_HPP_INCLUDED
