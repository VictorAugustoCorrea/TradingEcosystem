#pragma once

#ifndef TRADINGECOSYSTEM_FEATURE_ENGINE_H
#define TRADINGECOSYSTEM_FEATURE_ENGINE_H

#include "market_order_book.h"
#include "low-latency-components/types.h"
#include "low-latency-components/logging.h"

using namespace Common;

namespace Trading {
    constexpr auto Feature_INVALID = std::numeric_limits<double>::quiet_NaN();

    class FeatureEngine {
    public:
        explicit FeatureEngine(Logger *logger) : logger_(logger) { }

        auto onOrderBookUpdate(const TickerId ticker_id, const Price price, const Side side, const MarketOrderBook *book) noexcept -> void {
            if (const auto bbo = book -> getBBO(); bbo -> bid_price_ != Price_INVALID && bbo -> ask_price_ != Price_INVALID) {
                mkt_price_ = (static_cast<double> (bbo -> bid_price_ )* bbo -> ask_qty_ + static_cast<double>(bbo -> ask_price_) * bbo -> bid_qty_) / static_cast<double> (bbo -> bid_qty_ + bbo -> ask_qty_);
            }

            logger_ -> log("%:% %() % ticker: %, price: %, side: %, mkt price: %, agg-trade-ratio: %. \n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_),
                ticker_id,
                priceToString(price).c_str(),
                sideToString(side).c_str(),
                mkt_price_,
                agg_trade_qty_ratio_);
        }

        auto onTradeUpdate(const Exchange::MEMarketUpdate *market_update, const MarketOrderBook *book) noexcept -> void {
            if (const auto bbo = book -> getBBO(); bbo -> bid_price_ != Price_INVALID && bbo -> ask_price_ != Price_INVALID) {
                agg_trade_qty_ratio_ = static_cast<double> (market_update -> qty_) / (market_update -> side_ == Side::BUY ? bbo -> ask_qty_ : bbo -> bid_qty_);
            }

            logger_ -> log("%:% %() % update: %, mkt price: %, agg-trade-ratio: %. \n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_),
                market_update -> toString().c_str(),
                mkt_price_,
                agg_trade_qty_ratio_);
        }

        [[nodiscard]] auto getMktPrice() const noexcept { return mkt_price_; }
        [[nodiscard]] auto getAggTradeQtyRatio() const noexcept { return agg_trade_qty_ratio_; }

        FeatureEngine() = delete;
        FeatureEngine(const FeatureEngine & ) = delete;
        FeatureEngine(const FeatureEngine &&) = delete;
        FeatureEngine & operator=(const FeatureEngine & ) = delete;
        FeatureEngine & operator=(const FeatureEngine &&) = delete;

    private:
        std::string time_str_;
        Logger *logger_ = nullptr;
        double mkt_price_ = Feature_INVALID;
        double agg_trade_qty_ratio_ = Feature_INVALID;
    };
}
#endif //TRADINGECOSYSTEM_FEATURE_ENGINE_H
