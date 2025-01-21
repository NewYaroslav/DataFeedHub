#pragma once
#ifndef _DTH_TRADING_STRATEGY_HPP_INCLUDED
#define _DTH_TRADING_STRATEGY_HPP_INCLUDED

/// \file TradingStrategy.hpp
/// \brief

namespace dfh {

    class TradingStrategy : public BaseTradingBot {
    private:
        std::vector<std::unique_ptr<BaseTradingBot>> bots;

    public:
        void add_bot(std::unique_ptr<BaseTradingBot> bot) {
            bots.push_back(std::move(bot));
        }

        void on_event(const BaseEvent& event) override {
            for (auto& bot : bots) {
                bot->on_event(event);
            }
        }

        void start() override {
            for (auto& bot : bots) {
                bot->start();
            }
        }

        void stop() override {
            for (auto& bot : bots) {
                bot->stop();
            }
        }
    };

}; // namespace dfh

#endif // _DTH_TRADING_STRATEGY_HPP_INCLUDED
