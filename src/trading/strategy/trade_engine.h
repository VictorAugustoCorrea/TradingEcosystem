#pragma once

#ifndef TRADINGECOSYSTEM_TRADING_ENGINE_H
#define TRADINGECOSYSTEM_TRADING_ENGINE_H

/** Low latency components */
#include <functional>
#include "low-latency-components/macros.h"
#include "low-latency-components/logging.h"
#include "low-latency-components/time_utils.h"
#include "low-latency-components/thread_utils.h"
#include "low-latency-components/lock_free_queue.h"

/** Exchange components */
#include "exchange/market_data/market_update.h"
#include "exchange/order_server/client_request.h"
#include "exchange/order_server/client_response.h"

/** Trading components */
#include "market_maker.h"
#include "risk_manager.h"
#include "order_manager.h"
#include "feature_engine.h"
#include "position_keeper.h"
#include "liquidity_taker.h"
#include "market_order_book.h"

namespace Trading {
    class TradeEngine {
    public:
        TradeEngine(
            ClientId client_id,
            AlgoType algo_type,
            const TradeEngineCfgHashMap &ticker_cfg,
            Exchange::ClientRequestLFQueue *client_requests,
            Exchange::MEClientResponseLFQueue *client_responses,
            Exchange::MEMarketUpdateLFQueue *market_updates
            );

        auto run() noexcept -> void;
        auto start() -> void {
            run_ = true;
            ASSERT(createAndStartThread(-1, "Trading/TradeEngine", [this]  { run(); }) != nullptr,
                "Failed to start TradeEngine thread.");
        }

        auto sendClientRequest(const Exchange::MEClientRequest *client_request) noexcept -> void;
        auto onOrderBookUpdate(TickerId ticker_id, Price price, Side side, MarketOrderBook *book) noexcept -> void;
        auto onTradeUpdate(const Exchange::MEMarketUpdate * market_update, MarketOrderBook *book) noexcept -> void;
        auto onOrderUpdate(const Exchange::MEClientResponse *client_response) noexcept -> void;

        auto stop() -> void {
            while (incoming_ogw_responses_ -> size() || incoming_md_updates_ -> size()) {
                logger_.log("%:% %() % Sleeping till all updates are consumed. Ogw-size: %, Md-size: %.\n",
                    __FILE__, __LINE__, __func__,
                    getCurrentTimeStr(&time_str_),
                    incoming_ogw_responses_ -> size(),
                    incoming_md_updates_ -> size());

                using namespace std::literals::chrono_literals;
                std::this_thread::sleep_for(10ms);
            }
            logger_.log("%:% %() % POSITIONS\n%\n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_),
                position_keeper_.toString());

            run_ = false;
        }

        auto initLastEventTime() {
            last_event_time_ = getCurrentNanos();
        }

        auto silentSeconds() const {
            return (getCurrentNanos() - last_event_time_) / NANOS_TO_SECS;
        }

        auto clientId() const {
            return client_id_;
        }

        std::function<void(TickerId ticker_id, Price price, Side side, MarketOrderBook *book)> algoOnOrderBookUpdate_;
        std::function<void(const Exchange::MEMarketUpdate *market_update, MarketOrderBook *book)> algoOnTradeUpdate_;
        std::function<void(const Exchange::MEClientResponse *client_response)> algoOnOrderUpdate_;

        ~TradeEngine();
        TradeEngine() = delete;
        TradeEngine(const TradeEngine& ) = delete;
        TradeEngine(const TradeEngine&&) = delete;
        TradeEngine& operator=(const TradeEngine& ) = delete;
        TradeEngine& operator=(const TradeEngine&&) = delete;

    private:
        /* Variables components */
        const ClientId client_id_;
        MarketOrderBookHashMap ticker_order_book_ = {};
        Exchange::ClientRequestLFQueue *outgoing_ogw_requests_ = nullptr;
        Exchange::MEClientResponseLFQueue *incoming_ogw_responses_ = nullptr;
        Exchange::MEMarketUpdateLFQueue *incoming_md_updates_ = nullptr;
        Nanos last_event_time_ = 0;
        volatile bool run_ = false;
        std::string time_str_;
        Logger logger_;

        /* Instance Components */
        FeatureEngine feature_engine_;
        PositionKeeper position_keeper_;
        OrderManager order_manager_;
        RiskManager risk_manager_;
        MarketMaker *mm_algo_ = nullptr;
        LiquidityTaker *taker_algo_ = nullptr;

        /** Default methods */
        auto defaultAlgoOnOrderBookUpdate(const TickerId ticker_id, const Price price, const Side side, MarketOrderBook * /* book */) noexcept -> void {
            logger_.log("%:% %() % Ticker: %, Price: %, Side: %.\n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_),
                ticker_id,
                priceToString(price).c_str(),
                sideToString(side).c_str());
        }

        auto defaultAlgoOnTradeUpdate(const Exchange::MEMarketUpdate *market_update, MarketOrderBook * /* book */ ) noexcept -> void {
            logger_.log("%:% %() % %.\n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_),
                market_update -> toString().c_str());
        }

        auto defaultAlgoOnOrderUpdate(const Exchange::MEClientResponse *client_response) noexcept -> void {
            logger_.log("%:% %() % %.\n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_),
                client_response -> toString().c_str());
        }
    };
}



#endif //TRADINGECOSYSTEM_TRADING_ENGINE_H
