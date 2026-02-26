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

    auto MEOrderBook::add(ClientId client_id, OrderId client_order_id, TickerId ticker_id, Side side, Price price, Qty qty) noexcept -> void {
        const auto new_market_order_id = generateNewMarketOrderId();
        client_response_ = {
            ClientResponseType::ACCEPTED,
            side,
            client_id,
            ticker_id,
            client_order_id,
            new_market_order_id,
            price,
            0,
            qty
        };
        matching_engine_ -> sendClientResponse(&client_response_);
        if (const auto leaves_qty = checkForMatch(client_id, client_order_id, ticker_id, side, price, qty, new_market_order_id)) {
            const auto priority = getNextPriority(price);
            auto order = order_pool_.allocate(
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
}
