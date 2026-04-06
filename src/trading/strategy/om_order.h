#pragma once

#ifndef TRADINGECOSYSTEM_OM_ORDER_H
#define TRADINGECOSYSTEM_OM_ORDER_H

#include <array>
#include <sstream>
#include "low-latency-components/types.h"

using namespace Common;

namespace Trading {
    enum class OMOrderState : int8_t {
        INVALID = 0,
        PENDING_NEW = 1,
        LIVE = 2,
        PENDING_CANCEL = 3,
        DEAD = 4
    };

    inline auto OMOrderStateToString(const OMOrderState side) -> std::string {
        switch (side) {
            case OMOrderState::PENDING_NEW:
                return "PENDING_NEW";
            case OMOrderState::LIVE:
                return "LIVE";
            case OMOrderState::PENDING_CANCEL:
                return "PENDING_CANCEL";
            case OMOrderState::DEAD:
                return "DEAD";
            case OMOrderState::INVALID:
                return "INVALID";
        }
        return "UNKNOWN";
    }

    struct OMOrder {
        TickerId ticker_id_ = TickerId_INVALID;
        OrderId order_id_ = OrderId_INVALID;
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        Qty qty_ = Qty_INVALID;
        OMOrderState order_state_ = OMOrderState::INVALID;

        [[nodiscard]]
        auto toString() const {
            std::stringstream ss;
            ss  << "OMOrder: "
                << "[ "
                << "Ticker id: " << tickerIdToString(ticker_id_) << " "
                << "Order id: " << orderIdToString(order_id_) << " "
                << "Side: " << sideToString(side_) << " "
                << "Price: " << priceToString(price_) << " "
                << "Qty: " << qtyToString(qty_) << " "
                << "State: " << OMOrderStateToString(order_state_) << " "
                << " ] ";
            return ss.str();
        }
    };

    typedef std::array<OMOrder, sideToIndex(Side::MAX) + 1> OMOrderSideHashMap;
    typedef std::array<OMOrderSideHashMap, ME_MAX_TICKERS> OMOrderTickerSideHashMap;
}

#endif //TRADINGECOSYSTEM_OM_ORDER_H
