#pragma once
#ifndef _DTH_MARKET_DATA_LISTENER_HPP_INCLUDED
#define _DTH_MARKET_DATA_LISTENER_HPP_INCLUDED

/// \file MarketDataListener.hpp
/// \brief

namespace dfh::core {

    class MarketDataListener {
    public:

        virtual void on_update(MarketSnapshot&) = 0;

    }; // MarketDataListener

}; // namespace dfh::core

#endif // _DTH_MARKET_DATA_LISTENER_HPP_INCLUDED
