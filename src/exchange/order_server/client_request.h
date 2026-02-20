#pragma once

#ifndef TRADINGECOSYSTEM_CLIENT_REQUEST_H
#define TRADINGECOSYSTEM_CLIENT_REQUEST_H

#include <sstream>

#include "low-latency-components/types.h"
#include "low-latency-components/lock_free_queue.h"

using namespace Common;

namespace Exchange {
    #pragma pack(push, 1)

    enum class ClientRequestType : uint8_t {
        INVALID = 0,
        NEW = 1,
        CANCEL =2
    };

    inline std::string clientRequestTypeToString(const ClientRequestType type) {
        switch (type) {
            case ClientRequestType::NEW:
                return "NEW";
            case ClientRequestType::CANCEL:
                return "CANCEL";
            case ClientRequestType::INVALID:
                return "INVALID";
        }
        return "UNKNOWN";
    }

    struct MEClientRequest {
        ClientRequestType type_ = ClientRequestType::INVALID;

        Qty qty_ = Qty_INVALID;
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        OrderId order_id_ = OrderId_INVALID;
        ClientId client_id_ = ClientId_INVALID;
        TickerId ticker_id_ = TickerId_INVALID;

        auto toString() const {
            std::stringstream ss;
            ss << "MEClientRequest "
            << " [ "
            << "Type: " << clientRequestTypeToString( type_ )
            << " Client: " << clientIdToString( client_id_ )
            << " ticker: " << tickerIdToString( ticker_id_ )
            << " oid: " << orderIdToString( order_id_ )
            << " side: " << sideToString( side_ )
            << " qty: " << qtyToString( qty_ )
            << " price: " << priceToString( price_ )
            << " ] ";
            return ss.str();
        }
    };

    #pragma pack(pop)

    typedef LFQueue<MEClientRequest> ClientRequestLFQueue;
}

#endif //TRADINGECOSYSTEM_CLIENT_REQUEST_H