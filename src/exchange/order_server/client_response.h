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

    struct MEClientResponse {                                    /**   size alignment    */
        ClientResponseType type_ = ClientResponseType::INVALID;  /**  uint8_t  (1) byte  */
        Side side_ = Side::INVALID;                              /**  uint8_t  (1) byte  */
        TickerId ticker_id_ = TickerId_INVALID;                  /**  uint32_t (4) bytes */
        ClientId client_id_ = ClientId_INVALID;                  /**  uint64_t (8) bytes */
        OrderId client_order_id_ = OrderId_INVALID;              /**  uint64_t (8) bytes */
        OrderId market_order_id_ = OrderId_INVALID;              /**  uint64_t (8) bytes */
        Price price_ = Price_INVALID;                            /**  uint64_t (8) bytes */
        Qty exec_qty_ = Qty_INVALID;                             /**  uint64_t (8) bytes */
        Qty leaves_qty_ = Qty_INVALID;                           /**  uint64_t (8) bytes */

        [[nodiscard]]
        auto toString() const {
            std::stringstream ss;
            ss  << "MEClientResponse"
                << " [ "
                << " Type: " << clientResponseTypeToString( type_ )
                << " Client: " << clientIdToString( client_id_ )
                << " Ticker " << tickerIdToString( ticker_id_ )
                << " Client order id: " << orderIdToString( client_order_id_ )
                << " Market order id: " << orderIdToString( market_order_id_ )
                << " Side: " << sideToString( side_ )
                << " Executed qty: " << qtyToString( exec_qty_ )
                << " Leaves qty: " << qtyToString( leaves_qty_ )
                << " Price: " << priceToString( price_ )
                << " ] ";
            return ss.str();
        }
    };
    #pragma pack(pop)

    typedef LFQueue<MEClientResponse> MEClientResponseLFQueue;

}

#endif //TRADINGECOSYSTEM_CLIENT_RESPONSE_H