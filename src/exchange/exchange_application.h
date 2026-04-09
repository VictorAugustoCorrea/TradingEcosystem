#pragma once

#include "low-latency-components/logging.h"
#include "low-latency-components/lock_free_queue.h"
#include "exchange/matcher/matching_engine.h"
#include "exchange/order_server/order_server.h"
#include "exchange/market_data/market_data_publisher.h"

using namespace Common;

namespace App {

    class ExchangeApplication {
    public:
        ExchangeApplication(
            Exchange::ClientRequestLFQueue*    client_requests,
            Exchange::MEClientResponseLFQueue* client_responses,
            Exchange::MEMarketUpdateLFQueue*   market_updates,
            Logger*                            logger);

        ~ExchangeApplication();

        void start();
        void stop();

    private:
        Exchange::ClientRequestLFQueue*    client_requests_  = nullptr;
        Exchange::MEClientResponseLFQueue* client_responses_ = nullptr;
        Exchange::MEMarketUpdateLFQueue*   market_updates_   = nullptr;

        Logger* logger_ = nullptr;

        Exchange::MatchingEngine*      matching_engine_       = nullptr;
        Exchange::MarketDataPublisher* market_data_publisher_ = nullptr;
        Exchange::OrderServer*         order_server_          = nullptr;

        /** Network config */
        static constexpr const auto* MKT_PUB_IFACE   = "lo";
        static constexpr const auto* SNAP_PUB_IP     = "233.252.14.1";
        static constexpr const auto* INC_PUB_IP      = "233.252.14.3";
        static constexpr int SNAP_PUB_PORT           = 20000;
        static constexpr int INC_PUB_PORT            = 20001;
        static constexpr const auto* ORDER_GW_IFACE  = "lo";
        static constexpr int ORDER_GW_PORT           = 12345;
    };
} // namespace App