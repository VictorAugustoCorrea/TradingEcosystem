#pragma once

#ifndef TRADINGECOSYSTEM_CLIENT_RESPONSE_H
#define TRADINGECOSYSTEM_CLIENT_RESPONSE_H

#include <sstream>
#include "low-latency-components/types.h"
#include "low-latency-components/lock_free_queue.h"

using namespace Common;

namespace Exchange {
    #pragma pack(push, 1)

    enum class ClientResponseType : uint8_t {
        INVALID = 0,
        ACCEPTED = 1,
        CANCELED = 2,
        FILLED = 3,
        CANCEL_REJECTED = 4
    };

    inline std::string clientResponseTypeToString(const ClientResponseType type) {
        switch ( type ) {
            case ClientResponseType::ACCEPTED:
                return "ACCEPTED";
            case ClientResponseType::CANCELED:
                return "CANCELED";
            case ClientResponseType::FILLED:
                return "FILLED";
            case ClientResponseType::CANCEL_REJECTED:
                return "CANCEL_REJECTED";
            case ClientResponseType::INVALID:
                return "INVALID";
        }
        return "UNKNOWN";
    }

    struct MEClientResponse {
        Side side_ = Side::INVALID;
        Qty exec_qty_ = Qty_INVALID;
        Price price_ = Price_INVALID;
        Qty leaves_qty_ = Qty_INVALID;
        ClientId client_id_ = ClientId_INVALID;
        TickerId ticker_id_ = TickerId_INVALID;
        OrderId client_order_id_ = OrderId_INVALID;
        OrderId market_order_id_ = OrderId_INVALID;
        ClientResponseType type_ = ClientResponseType::INVALID;

        auto toString() const {
            std::stringstream ss;
            ss  << "MEClientResponse"
                << " [ "
                << "type: " << clientResponseTypeToString( type_ )
                << " client: " << clientIdToString( client_id_ )
                << " ticket: " << tickerIdToString( ticker_id_ )
                << " client order id: " << orderIdToString( client_order_id_ )
                << " market order id: " << orderIdToString( market_order_id_ )
                << " side: " << sideToString( side_ )
                << " executed qty: " << qtyToString( exec_qty_ )
                << " leaves qty: " << qtyToString( leaves_qty_ )
                << " price: " << priceToString( price_ )
                << " ] ";
            return ss.str();
        }
    };
    #pragma pack(pop)

    typedef LFQueue<MEClientResponse> MEClientResponseLFQueue;

}

#endif //TRADINGECOSYSTEM_CLIENT_RESPONSE_H