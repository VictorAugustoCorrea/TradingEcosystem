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

        auto add(ClientId client_id, OrderId client_order_id, TickerId ticker_id, Side side, Price price, Qty qty) noexcept -> void;
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

        auto addOrdersAtPrice(MEOrdersAtPrice *new_orders_at_price) noexcept {
            price_orders_at_price_hash_map_.at(priceToIndex(new_orders_at_price -> price_)) = new_orders_at_price;

            if (const auto best_orders_by_price = new_orders_at_price -> side_ == Side::BUY ? bids_at_price_ : asks_at_price_;
                 UNLIKELY(!best_orders_by_price)) {
                new_orders_at_price -> side_ == Side::BUY ? bids_at_price_ : asks_at_price_ = new_orders_at_price;
                new_orders_at_price -> prev_entry_ = new_orders_at_price -> next_entry_ = new_orders_at_price;
            }
            else {
                auto target = best_orders_by_price;
                bool add_after =
                    new_orders_at_price -> side_ == Side::SELL && new_orders_at_price -> price_ > target -> price_ ||
                    new_orders_at_price -> side_ == Side::BUY  && new_orders_at_price -> price_ < target -> price_;
                if (add_after) {
                    target = target -> next_entry_;
                    add_after =
                        new_orders_at_price -> side_ == Side::SELL && new_orders_at_price -> price_ > target -> price_ ||
                        new_orders_at_price -> side_ == Side::BUY  && new_orders_at_price -> price_ < target -> price_;
                }
                while (add_after && target != best_orders_by_price) {
                    add_after =
                        new_orders_at_price -> side_ == Side::SELL && new_orders_at_price -> price_ > target -> price_ ||
                        new_orders_at_price -> side_ == Side::BUY  && new_orders_at_price -> price_ < target -> price_;
                    if (add_after)
                        target = target -> next_entry_;
                }
                if (add_after) {
                    /** add new_orders_at_price after target */
                    if (target == best_orders_by_price) {
                        target = best_orders_by_price -> prev_entry_;
                    }
                    new_orders_at_price -> prev_entry_ = target;
                    target -> next_entry_ -> prev_entry_ = new_orders_at_price;
                    target -> next_entry_ = new_orders_at_price;
                }
                else {
                    /** add new_orders_at_price before target */
                    new_orders_at_price -> prev_entry_ = target -> prev_entry_;
                    new_orders_at_price -> next_entry_ = target;
                    target -> prev_entry_ -> next_entry_ = new_orders_at_price;
                    target -> prev_entry_ = new_orders_at_price;
                    if (
                        new_orders_at_price -> side_ == Side::BUY  && new_orders_at_price -> price_ > best_orders_by_price -> price_ ||
                        new_orders_at_price -> side_ == Side::SELL && new_orders_at_price -> price_ < best_orders_by_price -> price_
                        ) {
                            target -> next_entry_ = target -> next_entry_ == best_orders_by_price ? new_orders_at_price : target -> next_entry_;
                            new_orders_at_price -> side_ == Side::BUY ? bids_at_price_ : asks_at_price_ = new_orders_at_price;
                    }
                }
            }
        }


        [[nodiscard]]
        auto getNextPriority(const Price price) const noexcept {
            const auto orders_at_price = getOrdersAtPrice(price);
            if (!orders_at_price)
                return 1lu;

            return orders_at_price -> first_me_order_ -> prev_order_ -> priority_ + 1;
        }

        auto addOrder(MEOrder *order) noexcept {
            if (const auto orders_at_price = getOrdersAtPrice(order -> price_); !orders_at_price) {
                order -> next_order_ = order -> prev_order_ = order;

                const auto new_orders_at_price = orders_at_price_pool_.allocate(
                    order -> side_,
                    order -> price_,
                    order,
                    nullptr,
                    nullptr);
                addOrdersAtPrice(new_orders_at_price);
            }
            else {
                const auto first_order = orders_at_price -> first_me_order_;

                first_order -> prev_order_ -> next_order_ = order;
                order -> prev_order_ = first_order -> prev_order_;
                order -> next_order_ = first_order;
                first_order -> prev_order_ = order;
            }
            cid_oid_to_order_.at(order -> client_id_).at(order -> client_order_id_) = order;
        }
    };
    typedef std::array<MEOrderBook *, ME_MAX_TICKERS> OrderBookHashMap;
}

#endif //TRADINGECOSYSTEM_ME_ORDER_BOOK_H