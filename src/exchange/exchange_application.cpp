#include "exchange_application.h"

namespace App {

ExchangeApplication::ExchangeApplication(
    Exchange::ClientRequestLFQueue*    client_requests,
    Exchange::MEClientResponseLFQueue* client_responses,
    Exchange::MEMarketUpdateLFQueue*   market_updates,
    Logger*                            logger)
    : client_requests_(client_requests)
    , client_responses_(client_responses)
    , market_updates_(market_updates)
    , logger_(logger)
{}

ExchangeApplication::~ExchangeApplication() {
    stop();
}

void ExchangeApplication::start() {
    std::string time_str;

    /** Matching Engine */
    logger_->log("%:% %() % Starting Matching Engine...\n",
                 __FILE__, __LINE__, __func__,
                 getCurrentTimeStr(&time_str));

    matching_engine_ = new Exchange::MatchingEngine(
        client_requests_, client_responses_, market_updates_);
    matching_engine_->start();

    /** Market Data Publisher */
    logger_->log("%:% %() % Starting Market Data Publisher...\n",
                 __FILE__, __LINE__, __func__,
                 getCurrentTimeStr(&time_str));

    const std::string mkt_pub_iface = MKT_PUB_IFACE;
    const std::string snap_pub_ip   = SNAP_PUB_IP;
    const std::string inc_pub_ip    = INC_PUB_IP;

    market_data_publisher_ = new Exchange::MarketDataPublisher(
        market_updates_,
        mkt_pub_iface,
        snap_pub_ip, SNAP_PUB_PORT,
        inc_pub_ip,  INC_PUB_PORT);
    market_data_publisher_->start();

    /** Order Server */
    logger_->log("%:% %() % Starting Order Server...\n",
                 __FILE__, __LINE__, __func__,
                 getCurrentTimeStr(&time_str));

    std::string order_gw_iface = ORDER_GW_IFACE;

    order_server_ = new Exchange::OrderServer(
        client_requests_, client_responses_,
        order_gw_iface, ORDER_GW_PORT);
    order_server_->start();
}

void ExchangeApplication::stop() {
    delete matching_engine_;       matching_engine_       = nullptr;
    delete market_data_publisher_; market_data_publisher_ = nullptr;
    delete order_server_;          order_server_          = nullptr;
}

} // namespace App