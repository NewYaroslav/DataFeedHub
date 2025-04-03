#ifndef SUBSCRIBER_HPP_INCLUDED
#define SUBSCRIBER_HPP_INCLUDED

/// \file MarketDataBuffer.hpp
/// \brief

namespace dfh::core {

    class MarketEventListener {
    public:

        template<typename EventType>
        void subscribe(uint32_t symbol_index, uint32_t provider_index, EventType type) {

        }

    private:
        std::vector<std::shared_ptr<Event>>
    };

};

#endif // SUBSCRIBER_HPP_INCLUDED
