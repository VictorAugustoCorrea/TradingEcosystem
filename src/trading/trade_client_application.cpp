#include "trade_client_application.h"

#include <unistd.h>
#include "low-latency-components/thread_utils.h"

using namespace std::literals::chrono_literals;

namespace App {
    TradeClientApplication::TradeClientApplication(
        const ClientId                     client_id,
        const AlgoType                     algo_type,
        const TradeEngineCfgHashMap&       ticker_cfg,
        Exchange::ClientRequestLFQueue*    client_requests,
        Exchange::MEClientResponseLFQueue* client_responses,
        Exchange::MEMarketUpdateLFQueue*   market_updates,
        const bool                         use_shared_queues,
        Logger*                            logger) :
    client_id_(client_id)
    , algo_type_(algo_type)
    , ticker_cfg_(ticker_cfg)
    , client_requests_(client_requests)
    , client_responses_(client_responses)
    , market_updates_(market_updates)
    , use_shared_queues_(use_shared_queues)
    , logger_(logger)
    {}

    TradeClientApplication::~TradeClientApplication() {
        stop();
    }

    void TradeClientApplication::start() {
        std::string time_str;

        if (!use_shared_queues_) {
            /**
            * Network mode: MarketDataConsumer receives market data via multicast
            * and writes into market_updates_, which TradeEngine reads from.
            *
            * We keep iface, IPs and ports as named local variables because the
            * MarketDataConsumer constructor takes some parameters by const-ref —
            * temporaries would dangle.
            */
            logger_->log("%:% %() % Starting Market Data Consumer...\n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str));

            const std::string mkt_iface = MKT_IFACE;
            const std::string snap_ip   = SNAP_IP;
            const std::string inc_ip    = INC_IP;
            constexpr int snap_port     = SNAP_PORT;
            constexpr int inc_port      = INC_PORT;

            market_data_consumer_ = new Trading::MarketDataConsumer(
                client_id_,
                market_updates_,
                mkt_iface,
                snap_ip,
                snap_port,
                inc_ip,
                inc_port);

            market_data_consumer_->start();

            /** Give the consumer time to join the multicast groups */
            usleep(2 * 1000 * 1000);
        }

        /** ---- Trade Engine ---- */
        logger_->log("%:% %() % Starting Trade Engine...\n",
            __FILE__, __LINE__, __func__,
            getCurrentTimeStr(&time_str));

        trade_engine_ = new Trading::TradeEngine(
            client_id_,
            algo_type_,
            ticker_cfg_,
            client_requests_,
            client_responses_,
            market_updates_);
        trade_engine_->start();

        /** ---- Order Gateway ---- */
        logger_->log("%:% %() % Starting Order Gateway...\n",
            __FILE__, __LINE__, __func__,
            getCurrentTimeStr(&time_str));

        const std::string order_gw_ip    = ORDER_GW_IP;
        const std::string order_gw_iface = ORDER_GW_IFACE;

        order_gateway_ = new Trading::OrderGateway(
            client_id_,
            client_requests_,
            client_responses_,
            order_gw_ip,
            order_gw_iface,
            ORDER_GW_PORT);
        order_gateway_->start();

        /**
        * Allow the full stack (consumer + engine + gateway) to stabilise
        * before the strategy starts sending orders.
        */
        usleep(10 * 1000 * 1000);
        trade_engine_->initLastEventTime();
    }

    void TradeClientApplication::stop() {
        if (trade_engine_)         trade_engine_->stop();
        if (market_data_consumer_) market_data_consumer_->stop();
        if (order_gateway_)        order_gateway_->stop();

        std::this_thread::sleep_for(2s);

        delete trade_engine_;         trade_engine_         = nullptr;
        delete market_data_consumer_; market_data_consumer_ = nullptr;
        delete order_gateway_;        order_gateway_        = nullptr;
    }

    void TradeClientApplication::runRandomAlgo() const {
        OrderId order_id = client_id_ * 1000;

        std::vector<Exchange::MEClientRequest> requests_sent;
        std::array<Price, ME_MAX_TICKERS>      base_price = {};

        for (size_t i = 0; i < ME_MAX_TICKERS; ++i)
            base_price[i] = random() % 100 + 100;

        for (size_t i = 0; i < 1000; ++i) {
            const TickerId ticker_id = random() % ME_MAX_TICKERS;
            const Price    price     = base_price[ticker_id] + (random() % 10 + 1);
            const Qty      qty       = 1 + (random() % 100 + 1);
            const Side     side      = random() % 2 ? Side::BUY : Side::SELL;

            Exchange::MEClientRequest new_request{
                Exchange::ClientRequestType::NEW,
                client_id_,
                ticker_id,
                order_id++,
                side,
                price,
                qty
            };

            trade_engine_->sendClientRequest(&new_request);
            usleep(SLEEP_TIME_US);

            requests_sent.push_back(new_request);

            auto cxl_request  = requests_sent[random() % requests_sent.size()];
            cxl_request.type_ = Exchange::ClientRequestType::CANCEL;

            trade_engine_->sendClientRequest(&cxl_request);
            usleep(SLEEP_TIME_US);
        }
    }

    bool TradeClientApplication::isSilent(const int seconds) const {
        return trade_engine_ && trade_engine_->silentSeconds() >= seconds;
    }

    void TradeClientApplication::waitUntilSilent(const int threshold_seconds, const int poll_seconds) const {
        std::string time_str;

        while (!isSilent(threshold_seconds)) {
            logger_->log("%:% %() % Waiting, silent for % seconds...\n",
                         __FILE__, __LINE__, __func__,
                         getCurrentTimeStr(&time_str),
                         trade_engine_->silentSeconds());

            std::this_thread::sleep_for(std::chrono::seconds(poll_seconds));
        }
    }
} // namespace App