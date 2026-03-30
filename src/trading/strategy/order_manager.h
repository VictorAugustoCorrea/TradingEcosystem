#pragma once

#ifndef TRADINGECOSYSTEM_ORDER_MANAGER_H
#define TRADINGECOSYSTEM_ORDER_MANAGER_H

#include "om_order.h"
#include "risk_manager.h"
#include "low-latency-components/logging.h"
#include "exchange/order_server/client_response.h"

using namespace Common;

namespace Trading {
    class TradeEngine;
    class OrderManager {
    public:
        OrderManager(Logger *logger, TradeEngine *trade_engine, RiskManager& risk_manager) :
        trade_engine_(trade_engine), risk_manager_(risk_manager), logger_(logger) {}

        auto getOMOrderSideHashMap(const TickerId ticker_id) const {
            return &ticker_side_order_.at(ticker_id);
        }

        auto newOrder(OMOrder *order, TickerId ticker_id, Price price, Side side, Qty qty) noexcept -> void;
        auto cancelOrder(OMOrder *order) noexcept -> void;
        auto moveOrder(OMOrder *order, const TickerId ticker_id, const Price price, const Side side, const Qty qty) noexcept {
            switch (order -> order_state_) {
                case OMOrderState::LIVE: {
                    if (order -> price_ != price || order -> qty_ != qty)
                        cancelOrder(order);
                } break;

                case OMOrderState::INVALID:

                case OMOrderState::DEAD: {
                    if (price != Price_INVALID) {
                        if (const auto risk_result = risk_manager_.checkPreTradeRisk(ticker_id, side, qty); risk_result == RiskCheckResult::ALLOWED)
                            newOrder(order, ticker_id, price, side, qty);
                        else
                            logger_ -> log("%:% %() % Ticker:  %, Side: %, Qty: %, RiskCheckResult: %. \n",
                                __FILE__, __LINE__, __func__,
                                getCurrentTimeStr(&time_str_),
                                tickerIdToString(ticker_id),
                                sideToString(side),
                                qtyToString(qty),
                                riskCheckResultToString(risk_result));
                    }
                } break;

                case OMOrderState::PENDING_NEW:
                case OMOrderState::PENDING_CANCEL:
                    break;
            }
        }

        auto moveOrders(const TickerId ticker_id, const Price bid_price, const Price ask_price, const Qty clip) noexcept {
            const auto bid_order = &ticker_side_order_.at(ticker_id).at(sideToIndex(Side::BUY));
            moveOrder(bid_order, ticker_id, bid_price, Side::BUY, clip);

            const auto ask_order = &ticker_side_order_.at(ticker_id).at(sideToIndex(Side::SELL));
            moveOrder(ask_order, ticker_id, ask_price, Side::SELL, clip);
        }

        auto onOrderUpdate(const Exchange::MEClientResponse *client_response) noexcept -> void {
            logger_ -> log("%:% %() % %.\n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_),
                client_response -> toString().c_str());

            const auto order = &ticker_side_order_.at(client_response -> ticker_id_).at(sideToIndex(client_response -> side_));
            logger_ -> log("%:% %() % %.\n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_),
                client_response -> toString().c_str());

            switch (client_response -> type_) {
                case Exchange::ClientResponseType::ACCEPTED: {
                    order -> order_state_ = OMOrderState::LIVE;
                } break;

                case Exchange::ClientResponseType::CANCELED: {
                    order -> order_state_ = OMOrderState::DEAD;
                } break;

                case Exchange::ClientResponseType::FILLED: {
                    order -> qty_ = client_response -> leaves_qty_;
                    if (!order -> qty_)
                        order -> order_state_ = OMOrderState::DEAD;
                } break;

                case Exchange::ClientResponseType::CANCEL_REJECTED:
                case Exchange::ClientResponseType::INVALID: {}
                    break;
            }
        }

        OrderManager() = delete;
        OrderManager(const OrderManager& ) = delete;
        OrderManager(const OrderManager&&) = delete;
        OrderManager& operator=(const OrderManager& ) = delete;
        OrderManager& operator=(const OrderManager&&) = delete;

    private:
        TradeEngine *trade_engine_ = nullptr;
        const RiskManager& risk_manager_;
        std::string time_str_;
        Logger *logger_ = nullptr;
        OMOrderTickerSideHashMap ticker_side_order_;
        OrderId next_order_id_ = 1;
    };
}


#endif //TRADINGECOSYSTEM_ORDER_MANAGER_H
