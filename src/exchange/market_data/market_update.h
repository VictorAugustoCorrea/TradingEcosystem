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
        CLEAR = 1,
        ADD = 2,
        MODIFY = 3,
        CANCEL = 4,
        TRADE = 5,
        SNAPSHOT_START = 6,
        SNAPSHOT_END = 7
    };

    inline std::string marketUpdateTypeToString( const MEMarketUpdateType type ) {
        switch ( type ) {
            case MEMarketUpdateType::ADD:
                return "ADD";
            case MEMarketUpdateType::CLEAR:
                return "CLEAR";
            case MEMarketUpdateType::MODIFY:
                return "MODIFY";
            case MEMarketUpdateType::CANCEL:
                return "CANCEL";
            case MEMarketUpdateType::TRADE:
                return "TRADE";
            case MEMarketUpdateType::SNAPSHOT_START:
                return "SNAPSHOT_START";
            case MEMarketUpdateType::SNAPSHOT_END:
                return "SNAPSHOT_END";
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

    struct MDPMarketUpdate {
        size_t seq_num_ = 0;
        MEMarketUpdate me_market_update_;

        [[nodiscard]]
        auto toString() const {
            std::stringstream ss;
            ss  << "MDPMarketUpdate: "
                << " [ "
                << " seq: " << seq_num_
                << " " << me_market_update_.toString()
                << " ] ";
            return ss.str();
        }
    };
    #pragma pack(pop)

    typedef LFQueue<MEMarketUpdate> MEMarketUpdateLFQueue;
    typedef LFQueue<MDPMarketUpdate> MDPMarketUpdateLFQueue;
}

#endif //TRADINGECOSYSTEM_MARKET_UPDATE_H