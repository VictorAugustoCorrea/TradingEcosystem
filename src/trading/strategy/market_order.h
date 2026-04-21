#pragma once

#ifndef TRADINGECOSYSTEM_MARKET_ORDER_H
#define TRADINGECOSYSTEM_MARKET_ORDER_H

#include <array>
#include <sstream>
#include "low-latency-components/types.h"

using namespace Common;

namespace Trading {
    struct MarketOrder {
        OrderId order_id_ = OrderId_INVALID;
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        Qty qty_ = Qty_INVALID;
        Priority priority_ = Priority_INVALID;

        MarketOrder *prev_order_ = nullptr;
        MarketOrder *next_order_ = nullptr;
        MarketOrder() = default;

        MarketOrder(
            const OrderId order_id,
            const Side side,
            const Price price,
            const Qty qty,
            const Priority priority,
            MarketOrder *prev_order,
            MarketOrder *next_order) noexcept :
        order_id_(order_id),
        side_(side),
        price_(price),
        qty_(qty),
        priority_(priority),
        prev_order_(prev_order),
        next_order_(next_order)
        {}

        [[nodiscard]]
        auto toString() const -> std::string {
            std::stringstream ss;
            ss  << "MarketOrder"
                << " [ "
                << " Order id: " << orderIdToString(order_id_) << " "
                << " Side: " << sideToString(side_) << " "
                << " Price: " << priceToString(price_) << " "
                << " Qty: " << qtyToString(qty_) << " "
                << " Priority: " << priorityToString(priority_) << " "
                << " Previous order: " << orderIdToString(prev_order_ ? prev_order_ -> order_id_ : OrderId_INVALID) << " "
                << " Next order: " << orderIdToString(next_order_ ? next_order_ -> order_id_ : OrderId_INVALID) << "]";
            return ss.str();
        }
    };

    typedef std::array<MarketOrder *, ME_MAX_ORDER_IDS> OrderHashMap;


    struct MarketOrdersAtPrice {
        Side side_ = Side::INVALID;
        Price price_ = Price_INVALID;
        MarketOrder *first_mkt_order_ = nullptr;
        MarketOrdersAtPrice *prev_entry_ = nullptr;
        MarketOrdersAtPrice *next_entry_ = nullptr;
        MarketOrdersAtPrice() = default;

        MarketOrdersAtPrice(
            const Side side,
            const Price price,
            MarketOrder *first_mkt_order,
            MarketOrdersAtPrice *prev_entry,
            MarketOrdersAtPrice *next_entry) noexcept :
        side_(side),
        price_(price),
        first_mkt_order_(first_mkt_order),
        prev_entry_(prev_entry),
        next_entry_(next_entry)
        {}

        [[nodiscard]]
        auto toString() const -> std::string {
            std::stringstream ss;
            ss  << "MarketOrdersAtPrice"
                << " [ "
                << " Side: " << sideToString(side_) << " "
                << " Price: " << priceToString(price_) << " "
                << " First market order: " << (first_mkt_order_ ? first_mkt_order_->toString() : "null") << " "
                << " Previous entry: " << priceToString(prev_entry_ ? prev_entry_ -> price_ : Price_INVALID) << " "
                << " Next entry: " << priceToString(next_entry_ ? next_entry_ -> price_ : Price_INVALID)
                << " ] ";
            return ss.str();
        }
    };

    typedef std::array<MarketOrdersAtPrice *, ME_MAX_PRICE_LEVELS> OrdersAtPriceHashMap;

    struct BBO {
        Price bid_price_ = Price_INVALID;
        Price ask_price_ = Price_INVALID;
        Qty bid_qty_ = Qty_INVALID;
        Qty ask_qty_ = Qty_INVALID;

        [[nodiscard]]
        auto toString() const -> std::string {
            std::stringstream ss;
            ss  << "BBO"
                << " [ "
                << qtyToString(bid_qty_) << "@" << priceToString(bid_price_)
                << " X "
                << priceToString(ask_price_) << "@" << qtyToString(ask_qty_)
                << " ] ";
            return ss.str();
        }
    };
}


#endif //TRADINGECOSYSTEM_MARKET_ORDER_H