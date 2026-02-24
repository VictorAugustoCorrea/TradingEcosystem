#pragma once

#ifndef TRADINGECOSYSTEM_MARKET_UPDATE_H
#define TRADINGECOSYSTEM_MARKET_UPDATE_H

#include <sstream>
#include "low-latency-components/types.h"
#include "low-latency-components/lock_free_queue.h"

using namespace Common;

namespace Exchange {
    #pragma pack(push, 1)
    enum class MEMarketUpdateType : uint8_t {
        INVALID = 0,
        ADD = 1,
        MODIFY = 2,
        CANCEL = 3,
        TRADE = 4
    };

    inline std::string marketUpdateTypeToString( const MEMarketUpdateType type ) {
        switch ( type ) {
            case MEMarketUpdateType::ADD:
                return "ADD";
            case MEMarketUpdateType::MODIFY:
                return "MODIFY";
            case MEMarketUpdateType::CANCEL:
                return "CANCEL";
            case MEMarketUpdateType::TRADE:
                return "TRADE";
            case MEMarketUpdateType::INVALID:
                return "INVALID";
        }
        return "UNKNOWN";
    }

    struct MEMarketUpdate {
        MEMarketUpdateType type_ = MEMarketUpdateType::INVALID;
        OrderId order_id_ = OrderId_INVALID;
        TickerId ticker_id_ = TickerId_INVALID;
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        Qty qty_ = Qty_INVALID;
        Priority priority_ = Priority_INVALID;

        [[nodiscard]]
        auto toString() const {
            std::stringstream ss;
            ss  << "MEMarketUpdate: "
                << " [ "
                << " Type: " << marketUpdateTypeToString( type_ )
                << " Ticker: " << tickerIdToString( ticker_id_ )
                << " Order_id: " << orderIdToString( order_id_ )
                << " Side: " << sideToString ( side_ )
                << " Qty: " << qtyToString( qty_ )
                << " Price: " << priceToString( price_ )
                << " Priority: " << priorityToString( priority_ )
                << " ] ";
            return ss.str();
        }
    };
    #pragma pack(pop)

    typedef LFQueue<MEMarketUpdate> MEMarketUpdateLFQueue;
}

#endif //TRADINGECOSYSTEM_MARKET_UPDATE_H