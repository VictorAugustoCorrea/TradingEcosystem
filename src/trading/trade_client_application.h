#pragma once

#include "low-latency-components/logging.h"
#include "low-latency-components/lock_free_queue.h"
#include "exchange/matcher/matching_engine.h"
#include "trading/strategy/trade_engine.h"
#include "trading/order_gw/order_gateway.h"
#include "trading/market_data/market_data_consumer.h"

using namespace Common;

namespace App {

class TradeClientApplication {
public:
    /**
     * @param client_id
     * @param algo_type
     * @param ticker_cfg
     * @param client_requests
     * @param client_responses
     * @param market_updates
     * @param use_shared_queues  true  → exchange runs in the same process (queues are valid)
     *                           false → client connects to a remote exchange via network
     * @param logger
     */
    TradeClientApplication(
        ClientId                           client_id,
        AlgoType                           algo_type,
        const TradeEngineCfgHashMap&       ticker_cfg,
        Exchange::ClientRequestLFQueue*    client_requests,
        Exchange::MEClientResponseLFQueue* client_responses,
        Exchange::MEMarketUpdateLFQueue*   market_updates,
        bool                               use_shared_queues,
        Logger*                            logger);

    ~TradeClientApplication();

    void start();
    void stop();

    /** Runs the RANDOM algo loop (blocking until done) */
    void runRandomAlgo() const;

    /** Returns true once the engine has been silent for >= seconds */
    [[nodiscard]] bool isSilent(int seconds = 30) const;

    void waitUntilSilent(int threshold_seconds = 30, int poll_seconds = 5) const;

private:
    ClientId                           client_id_;
    AlgoType                           algo_type_;
    TradeEngineCfgHashMap              ticker_cfg_;
    Exchange::ClientRequestLFQueue*    client_requests_   = nullptr;
    Exchange::MEClientResponseLFQueue* client_responses_  = nullptr;
    Exchange::MEMarketUpdateLFQueue*   market_updates_    = nullptr;
    bool                               use_shared_queues_ = false;
    Logger*                            logger_            = nullptr;

    Trading::TradeEngine*        trade_engine_         = nullptr;
    Trading::MarketDataConsumer* market_data_consumer_ = nullptr;
    Trading::OrderGateway*       order_gateway_        = nullptr;

    /** Network config (used when connecting to a remote exchange) */
    static constexpr auto ORDER_GW_IP    = "127.0.0.1";
    static constexpr auto ORDER_GW_IFACE = "lo";
    static constexpr int         ORDER_GW_PORT  = 12345;

    static constexpr auto MKT_IFACE      = "lo";
    static constexpr auto SNAP_IP         = "233.252.14.1";
    static constexpr int         SNAP_PORT       = 20000;
    static constexpr auto INC_IP          = "233.252.14.3";
    static constexpr int         INC_PORT        = 20001;

    static constexpr int         SLEEP_TIME_US   = 100 * 1000;
};

} // namespace App