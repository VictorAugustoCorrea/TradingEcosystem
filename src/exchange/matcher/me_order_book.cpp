#include "me_order_book.h"
#include "exchange/matcher/matching_engine.h"

namespace Exchange {
    MEOrderBook::MEOrderBook(
        const TickerId ticker_id,
        Logger *logger,
        MatchingEngine * matching_engine) :
        ticker_id_(ticker_id),
        matching_engine_(matching_engine),
        orders_at_price_pool_(ME_MAX_PRICE_LEVELS),
        order_pool_(ME_MAX_ORDER_IDS),
        logger_(logger)
    {}

    MEOrderBook::~MEOrderBook() {
        logger_ -> log("%:% %() % OrderBook\n%\n",
            __FILE__, __LINE__, __func__,
            getCurrentTimeStr(&time_str_),
            toString(false, true));

        matching_engine_ = nullptr;
        bids_at_price_ = asks_at_price_ = nullptr;
        for (auto &itr : cid_oid_to_order_) {
            itr.fill(nullptr);
        }
    }

    auto MEOrderBook::match(const TickerId ticker_id, const ClientId client_id, const Side side, const OrderId client_order_id, const OrderId new_market_order_id, MEOrder* itr, Qty* leaves_qty) noexcept {
        const auto order = itr;
        const auto order_qty = order -> qty_;
        const auto fill_qty = std::min(*leaves_qty, order_qty);

        *leaves_qty -= fill_qty;
        order->qty_ -= fill_qty;

        client_response_ = {
            ClientResponseType::FILLED,
            client_id,
            ticker_id,
            client_order_id,
            new_market_order_id,
            side,
            itr -> price_,
            fill_qty,
            *leaves_qty
        };
        matching_engine_ -> sendClientResponse(&client_response_);

        client_response_ = {
            ClientResponseType::FILLED,
            order -> client_id_,
            ticker_id,
            order -> client_order_id_,
            order -> market_order_id_,
            order -> side_,
            itr -> price_,
            fill_qty,
            order -> qty_
        };
        matching_engine_ -> sendClientResponse(&client_response_);

        market_update_ = {
            MEMarketUpdateType::TRADE,
            OrderId_INVALID,
            ticker_id,
            side,
            itr -> price_,
            fill_qty,
            Priority_INVALID
        };
        matching_engine_ -> sendMarketUpdate(&market_update_);

        if (!order -> qty_) {
            market_update_ = {
                MEMarketUpdateType::CANCEL,
                order -> market_order_id_,
                ticker_id,
                order -> side_,
                order -> price_,
                order_qty,
                Priority_INVALID
            };
            matching_engine_ -> sendMarketUpdate(&market_update_);
            removeOrder(order);
        }
        else {
            market_update_ = {
                MEMarketUpdateType::MODIFY,
                order -> market_order_id_,
                ticker_id,
                order -> side_,
                order -> price_,
                order -> qty_,
                order -> priority_
            };
            matching_engine_ -> sendMarketUpdate(&market_update_);
        }
    }

    auto MEOrderBook::checkForMatch(const ClientId client_id, const OrderId client_order_id, const TickerId ticker_id, const Side side, const Price price, const Qty qty, const Qty new_market_order_id) noexcept{
        auto leaves_qty = qty;

        if (side == Side::BUY) {
            while (leaves_qty && asks_at_price_) {
                const auto ask_itr = asks_at_price_ -> first_me_order_;
                if (price < ask_itr -> price_) {
                    break;
                }
                match(ticker_id, client_id, side, client_order_id, new_market_order_id, ask_itr, &leaves_qty);
            }
        }
        if (side == Side::SELL) {
            while (leaves_qty && bids_at_price_) {
                const auto bid_itr = bids_at_price_ -> first_me_order_;
                if (price > bid_itr -> price_) {
                    break;
                }
                match(ticker_id, client_id, side, client_order_id, new_market_order_id, bid_itr, &leaves_qty);
            }
        }
        return leaves_qty;
    }

    auto MEOrderBook::add(const ClientId client_id, const OrderId client_order_id, const TickerId ticker_id, const Side side, const Price price, const Qty qty) noexcept -> void {
        const auto new_market_order_id = generateNewMarketOrderId();
        client_response_ = {
            ClientResponseType::ACCEPTED,
            client_id,
            ticker_id,
            client_order_id,
            new_market_order_id,
            side,
            price,
            0,
            qty
        };
        matching_engine_ -> sendClientResponse(&client_response_);
        if (const auto leaves_qty = checkForMatch(client_id, client_order_id, ticker_id, side, price, qty, new_market_order_id)) {
            const auto priority = getNextPriority(price);
            const auto order = order_pool_.allocate(
                ticker_id,
                client_id,
                client_order_id,
                new_market_order_id,
                side,
                price,
                leaves_qty,
                priority,
                nullptr,
                nullptr
                );
            addOrder(order);

            market_update_ = {
                MEMarketUpdateType::ADD,
                new_market_order_id,
                ticker_id,
                side,
                price,
                leaves_qty,
                priority
            };
            matching_engine_ -> sendMarketUpdate(&market_update_);
        }
    }
    auto MEOrderBook::cancel(const ClientId client_id, const OrderId order_id, const TickerId ticker_id) noexcept -> void {
        const auto is_cancelable = client_id < cid_oid_to_order_.size();
        MEOrder *exchange_order = nullptr;

        if (is_cancelable) {
            const auto &co_itr = cid_oid_to_order_.at(client_id);
            exchange_order = co_itr.at(order_id);
        }
        if (UNLIKELY(exchange_order == nullptr)) {
            client_response_ = {
                ClientResponseType::CANCEL_REJECTED,
                client_id,
                ticker_id,
                order_id,
                OrderId_INVALID,
                Side::INVALID,
                Price_INVALID,
                Qty_INVALID,
                Qty_INVALID
            };
        }
        else {
            client_response_ = {
                ClientResponseType::CANCELED,
                client_id,
                ticker_id,
                order_id,
                exchange_order -> market_order_id_,
                exchange_order -> side_,
                exchange_order -> price_,
                Qty_INVALID,
                exchange_order -> qty_
            };
            market_update_ = {
                MEMarketUpdateType::CANCEL,
                exchange_order -> market_order_id_,
                ticker_id,
                exchange_order -> side_,
                exchange_order -> price_,
                0,
                exchange_order -> priority_
            };
            removeOrder(exchange_order);
            matching_engine_ -> sendMarketUpdate(&market_update_);
        }
        matching_engine_ -> sendClientResponse(&client_response_);
    }
}
