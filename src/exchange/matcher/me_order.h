#pragma once

#ifndef TRADINGECOSYSTEM_ME_ORDER_H
#define TRADINGECOSYSTEM_ME_ORDER_H

#include <array>
#include <sstream>
#include "low-latency-components/types.h"

using namespace Common;

namespace Exchange {
    struct MEOrder {
        TickerId ticker_id_ = TickerId_INVALID;
        ClientId client_id_ = ClientId_INVALID;
        OrderId client_order_id_ = OrderId_INVALID;
        OrderId market_order_id_ = OrderId_INVALID;
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        Qty  qty_ = Qty_INVALID;
        Priority priority_ = Priority_INVALID;
        MEOrder *prev_order_ = nullptr;
        MEOrder *next_order_ = nullptr;

        /** Only needed for use with MemPoll */
        MEOrder() = default;

        MEOrder(
            const TickerId ticker_id,
            const ClientId client_id,
            const OrderId client_order_id,
            const OrderId market_order_id,
            const Side side,
            const Price price,
            const Qty qty,
            const Priority priority,
            MEOrder *prev_order,
            MEOrder *next_order) noexcept :

            ticker_id_( ticker_id ),
            client_id_( client_id ),
            client_order_id_( client_order_id ),
            market_order_id_( market_order_id ),
            side_( side ),
            price_( price ),
            qty_( qty ),
            priority_( priority ),
            prev_order_( prev_order),
            next_order_( next_order)
        {}

        [[nodiscard]]
        auto toString() const -> std::string;
    };

    typedef std::array< MEOrder *, ME_MAX_ORDER_IDS > OrderHashMap;
    typedef std::array< OrderHashMap, ME_MAX_NUM_CLIENTS > ClientOrderHashMap;

    struct MEOrdersAtPrice {
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;

        MEOrder *first_me_order_ = nullptr;

        MEOrdersAtPrice *prev_entry_ = nullptr;
        MEOrdersAtPrice *next_entry_ = nullptr;

        MEOrdersAtPrice() = default;

        MEOrdersAtPrice(
            const Side side,
            const Price price,
            MEOrder *first_me_order,
            MEOrdersAtPrice *prev_entry,
            MEOrdersAtPrice * next_entry ) :

            side_( side ),
            price_( price ),
            first_me_order_( first_me_order ),
            prev_entry_( prev_entry ),
            next_entry_( next_entry )
        {}

        [[nodiscard]]
        auto toString() const -> std::string {
            std::stringstream ss;
            ss  << "MEOrdersAtPrice: "
                << " [ "
                << " Side: " << sideToString( side_ )
                << " Price: " << priceToString( price_ )
                << " First ME Order: " << ( first_me_order_ ? first_me_order_ -> toString() : "null" )
                << " Prev: " << priceToString( prev_entry_ ? prev_entry_ -> price_ : Price_INVALID )
                << " Next: " << priceToString( next_entry_ ? next_entry_ -> price_ : Price_INVALID )
                << " ] ";
            return ss.str();
        }
    };
}



#endif //TRADINGECOSYSTEM_ME_ORDER_H