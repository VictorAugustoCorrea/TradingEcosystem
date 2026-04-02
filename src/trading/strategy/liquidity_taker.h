#pragma once

#ifndef TRADINGECOSYSTEM_LIQUIDITY_TAKER_H
#define TRADINGECOSYSTEM_LIQUIDITY_TAKER_H

#include "order_manager.h"
#include "feature_engine.h"
#include "low-latency-components/logging.h"

using namespace Common;

namespace Trading {
    class LiquidityTaker {
    public:
        LiquidityTaker(
            Logger *logger,
            TradeEngine *trade_engine,
            const FeatureEngine *feature_engine,
            OrderManager *order_manager,
            const TradeEngineCfgHashMap &ticker_cfg);

        auto onTradeUpdate(const Exchange::MEMarketUpdate *market_update, const MarketOrderBook *book) noexcept -> void {
            logger_ -> log("%:% %() % %.\n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_),
                market_update -> toString().c_str());
            const auto bbo = book -> getBBO();

            if (const auto agg_qty_ratio = feature_engine_ -> getAggTradeQtyRatio();
                bbo -> bid_price_ != Price_INVALID && bbo -> ask_price_ != Price_INVALID && agg_qty_ratio != Feature_INVALID) {
                logger_ -> log("%:% %() % % agg-qty-ratio: %.\n",
                    __FILE__, __LINE__, __func__,
                    getCurrentTimeStr(&time_str_),
                    bbo -> toString().c_str(),
                    agg_qty_ratio);

                const auto clip = ticker_cfg_.at(market_update -> ticker_id_).clip_;

                if (const auto threshold = ticker_cfg_.at(market_update -> ticker_id_).threshold_;
                    agg_qty_ratio >= threshold) {
                    if (market_update -> side_ == Side::BUY)
                        order_manager_ -> moveOrders(market_update -> ticker_id_, bbo -> ask_price_, Price_INVALID, clip);
                    else
                        order_manager_ -> moveOrders(market_update -> ticker_id_, Price_INVALID, bbo -> bid_price_, clip);
                }
            }
        }

        auto onOrderBookUpdate(const TickerId ticker_id, const Price price, const Side side, MarketOrderBook * /* book */) noexcept -> void {
            logger_ -> log("%:% %() % Ticker: %, Price: %, Side: %.\n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_),
                ticker_id,
                priceToString(price).c_str(),
                sideToString(side).c_str());
        }

        auto onOrderUpdate(const Exchange::MEClientResponse *client_response) noexcept -> void {
            logger_ -> log("%:% %() % %.\n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_),
                client_response -> toString().c_str());

            order_manager_ -> onOrderUpdate(client_response);
        }

        LiquidityTaker() = delete;
        LiquidityTaker(const LiquidityTaker& ) = delete;
        LiquidityTaker(const LiquidityTaker&&) = delete;
        LiquidityTaker& operator=(const LiquidityTaker& ) = delete;
        LiquidityTaker& operator=(const LiquidityTaker&&) = delete;

    private:
        const FeatureEngine *feature_engine_ = nullptr;
        OrderManager *order_manager_ = nullptr;
        std::string time_str_;
        Logger *logger_ = nullptr;
        const TradeEngineCfgHashMap ticker_cfg_;
    };
}


#endif //TRADINGECOSYSTEM_LIQUIDITY_TAKER_H
