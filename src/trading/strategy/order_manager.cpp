#include "order_manager.h"
#include "trade_engine.h"
#include "exchange/order_server/client_request.h"

namespace Trading {
    auto OrderManager::newOrder(OMOrder *order, const TickerId ticker_id, const Price price, const Side side, const Qty qty) noexcept -> void {
        const Exchange::MEClientRequest new_request  {
            Exchange::ClientRequestType::NEW,
            trade_engine_ -> clientId(),
            ticker_id,
            next_order_id_,
            side,
            price,
            qty
        };
        trade_engine_ -> sendClientRequest(&new_request);
        *order = { ticker_id, next_order_id_, side, price, qty, OMOrderState::PENDING_NEW };
        ++next_order_id_;

        logger_ -> log("%:% %() % Sent new order: % for %.\n",
            __FILE__, __LINE__, __func__,
            getCurrentTimeStr(&time_str_),
            new_request.toString().c_str(),
            order -> toString().c_str());
    }

    auto OrderManager::cancelOrder(OMOrder *order) noexcept -> void {
        const Exchange::MEClientRequest cancel_request{
            Exchange::ClientRequestType::CANCEL,
            trade_engine_ -> clientId(),
            order -> ticker_id_,
            order -> order_id_,
            order -> side_,
            order -> price_,
            order -> qty_
        };
        trade_engine_ -> sendClientRequest(&cancel_request);
        order -> order_state_ = OMOrderState::PENDING_CANCEL;

        logger_ -> log("%:% %() % Sent cancel % for %.\n",
            __FILE__, __LINE__, __func__,
            getCurrentTimeStr(&time_str_),
            cancel_request.toString().c_str(),
            order -> toString().c_str());
    }

}
