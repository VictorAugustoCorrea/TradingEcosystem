#pragma once

#ifndef TRADINGECOSYSTEM_ME_ORDER_BOOK_H
#define TRADINGECOSYSTEM_ME_ORDER_BOOK_H

#include "me_order.h"
#include "low-latency-components/types.h"
#include "low-latency-components/logging.h"
#include "low-latency-components/mem_pool.h"
#include "exchange/market_data/market_update.h"
#include "exchange/order_server/client_response.h"

using namespace Common;

namespace Exchange {
    class MatchingEngine;
    class MEOrderBook final {
    public:
        explicit MEOrderBook(TickerId ticker_id, Logger *logger, MatchingEngine *matching_engine);
        ~MEOrderBook();

        MEOrderBook() = delete;
        MEOrderBook(const MEOrderBook & ) = delete;
        MEOrderBook(const MEOrderBook &&) = delete;
        MEOrderBook & operator = (const MEOrderBook & ) = delete;
        MEOrderBook & operator = (const MEOrderBook &&) = delete;

    private:
            TickerId ticker_id_ = TickerId_INVALID;
            MatchingEngine *matching_engine_ = nullptr;
            ClientOrderHashMap cid_oid_to_order_ = {};
            MemPool<MEOrdersAtPrice> orders_at_price_pool_;
            MEOrdersAtPrice *bids_at_price_ = nullptr;
            MEOrdersAtPrice *asks_at_price_ = nullptr;
            OrdersAtPriceHashMap price_orders_at_price_hash_map_ = {};
            MemPool<MEOrder> order_pool_;
            MEClientResponse client_response_;
            MEMarketUpdate market_update_;
            OrderId next_market_order_id_ = 1;
            std::string time_str_;
            Logger *logger_ = nullptr;

        auto generateNewMarketOrderId() noexcept -> OrderId {
            return next_market_order_id_ ++;
        }

        [[nodiscard]]
        static auto priceToIndex(const Price price) noexcept {
            return price % ME_MAX_PRICE_LEVELS;
        }

        [[nodiscard]]
        auto getOrdersAtPrice(const Price price) const noexcept -> MEOrdersAtPrice* {
            return price_orders_at_price_hash_map_.at(priceToIndex(price));
        }
    };
    typedef std::array<MEOrderBook *, ME_MAX_TICKERS> OrderBookHashMap;
}

#endif //TRADINGECOSYSTEM_ME_ORDER_BOOK_H