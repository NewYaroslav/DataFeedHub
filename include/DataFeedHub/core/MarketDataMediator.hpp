#ifndef _DTH_MARKET_DATA_MEDIATOR_HPP_INCLUDED
#define _DTH_MARKET_DATA_MEDIATOR_HPP_INCLUDED

namespace dfh::core {

    class MarketDataMediator : public MarketDataListener {
    public:

        /// \brief
        /// \param
        explicit MarketDataMediator(MarketDataBus* bus) : m_bus(bus) {
            m_sub_id = m_bus->register_subscription(this);
        };

        /// \brief Destructor for MarketDataMediator.
        virtual ~MarketDataMediator() {
            m_bus->unregister_subscription(this);
        }

        bool subscribe_timer(uint32_t period_ms) {
            return m_bus->subscribe_timer(m_sub_id, period_ms);
        }

        bool unsubscribe_timer() {
            return m_bus->unsubscribe_timer(m_sub_id);
        }

        bool subscribe_ticks(uint32_t symbol_index, uint32_t provider_index) {
            return m_bus->subscribe_ticks(m_sub_id, symbol_index, provider_index);
        }

        bool unsubscribe_ticks(uint32_t symbol_index, uint32_t provider_index) {
            return m_bus->unsubscribe_ticks(m_sub_id, symbol_index, provider_index);
        }

    private:
        MarketDataBus* m_bus = nullptr;
        int            m_sub_id = -1;

    };

}

#endif // _DTH_MARKET_DATA_MEDIATOR_HPP_INCLUDED
