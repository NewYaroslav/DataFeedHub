#pragma once
#ifndef _DTH_TRADINGCONTEXT_HPP_INCLUDED
#define _DTH_TRADING_CONTEXT_HPP_INCLUDED

/// \file TradingContext.hpp
/// \brief

namespace dfh {

    class TradingContext {
    private:
        std::unique_ptr<DataFeed> data_feed;
        std::unique_ptr<IBroker> broker;
        std::vector<std::unique_ptr<BaseTradingBot>> bots;

    public:
        TradingContext(std::unique_ptr<DataFeed> df, std::unique_ptr<IBroker> br)
            : data_feed(std::move(df)), broker(std::move(br)) {}

        void add_bot(std::unique_ptr<BaseTradingBot> bot) {
            bots.push_back(std::move(bot));
        }

        void start() {
            data_feed->start();
            for (auto& bot : bots) {
                bot->start();
            }
        }

        void stop() {
            data_feed->stop();
            for (auto& bot : bots) {
                bot->stop();
            }
        }
    };

}; // namespace dfh

#endif // _DTH_TRADING_CONTEXT_HPP_INCLUDED
