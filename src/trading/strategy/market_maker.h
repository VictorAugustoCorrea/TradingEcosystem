#pragma once

#ifndef TRADINGECOSYSTEM_MARKET_MAKER_H
#define TRADINGECOSYSTEM_MARKET_MAKER_H

#include "order_manager.h"
#include "feature_engine.h"
#include "low-latency-components/logging.h"

using namespace Common;

namespace Trading {
    class MarketMaker {
    public:
        MarketMaker(
            Logger *logger,
            TradeEngine *trade_engine,
            const FeatureEngine *feature_engine,
            OrderManager *order_manager,
            const TradeEngineCfgHashMap &ticker_cfg
            );

        auto onOrderBookUpdate(const TickerId ticker_id, const Price price, const Side side, const MarketOrderBook *book) noexcept -> void {
            logger_ -> log("%:% %() % Ticker: %, Price: %, Side: %.\n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_),
                ticker_id,
                priceToString(price).c_str(),
                sideToString(side).c_str());

            const auto bbo = book -> getBBO();

            if (const auto fair_price = feature_engine_ -> getMktPrice();
                bbo -> bid_price_ != Price_INVALID && bbo -> ask_price_ != Price_INVALID && fair_price != Feature_INVALID) {
                logger_ -> log("%:% %() % % fair-price: %.\n",
                    __FILE__, __LINE__, __func__,
                    getCurrentTimeStr(&time_str_),
                    bbo -> toString().c_str(),
                    fair_price);

                const auto clip = ticker_cfg_.at(ticker_id).clip_;
                const auto threshold = ticker_cfg_.at(ticker_id).threshold_;
                const auto bid_price = bbo -> bid_price_ - (fair_price - static_cast<double>(bbo -> bid_price_) >= threshold ? 0 : 1);
                const auto ask_price = bbo -> ask_price_ + (static_cast<double>(bbo -> ask_price_) - fair_price >= threshold ? 0 : 1);
                START_MEASURE(Trading_OrderManager_moveOrders);
                order_manager_ -> moveOrders(ticker_id, bid_price, ask_price, clip);
                END_MEASURE(Trading_OrderManager_moveOrders, (*logger_));
            }
        }

        auto onTradeUpdate(const Exchange::MEMarketUpdate *market_update, MarketOrderBook * /* book */ ) noexcept -> void {
            logger_ -> log("%:% %() % %.\n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_),
                market_update -> toString().c_str());
        }

        auto onOrderUpdate(const Exchange::MEClientResponse *client_response) noexcept -> void {
            logger_ -> log("%:% %() % %.\n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_),
                client_response -> toString().c_str());
            START_MEASURE(Trading_OrderManager_onOrderUpdate);
            order_manager_ -> onOrderUpdate(client_response);
            END_MEASURE(Trading_OrderManager_onOrderUpdate, (*logger_));
        }

        MarketMaker() = delete;
        MarketMaker(const MarketMaker& ) = delete;
        MarketMaker(const MarketMaker&&) = delete;
        MarketMaker& operator=(const MarketMaker& ) = delete;
        MarketMaker& operator=(const MarketMaker&&) = delete;

    private:
        const FeatureEngine *feature_engine_ = nullptr;
        OrderManager *order_manager_ = nullptr;
        std::string time_str_;
        Logger *logger_ = nullptr;
        const TradeEngineCfgHashMap ticker_cfg_;
    };
}


#endif //TRADINGECOSYSTEM_MARKET_MAKER_H
