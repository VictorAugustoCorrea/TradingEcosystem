#include "low-latency-components/thread_utils.h"
#include "low-latency-components/mem_pool.h"
#include "low-latency-components/lock_free_queue.h"
#include "low-latency-components/logging.h"
#include "low-latency-components/tcp_server.h"
#include "exchange/matcher/matching_engine.h"
#include "exchange/market_data/market_data_publisher.h"
#include "exchange/order_server/order_server.h"
#include <csignal>

using namespace Common;
using namespace std::literals::chrono_literals;

Logger* logger = nullptr;
Exchange::MatchingEngine* matching_engine = nullptr;
Exchange::MarketDataPublisher *market_data_publisher = nullptr;
Exchange::OrderServer *order_server = nullptr;

/** Signal handler */
void signal_handler(int) {
    std::this_thread::sleep_for(10s);

    delete logger;
    logger = nullptr;
    delete matching_engine;
    matching_engine = nullptr;
    delete market_data_publisher;
    market_data_publisher = nullptr;
    delete order_server;
    order_server = nullptr;

    std::this_thread::sleep_for(10s);
    exit(EXIT_SUCCESS);
}

int main(int, char **)
{
    logger = new Logger("exchange_main.log");
    std::signal(SIGINT, signal_handler);

    constexpr int sleep_time = 100 * 1000;

    Exchange::ClientRequestLFQueue client_requests(ME_MAX_CLIENT_UPDATES);
    Exchange::MEClientResponseLFQueue client_responses(ME_MAX_CLIENT_UPDATES);
    Exchange::MEMarketUpdateLFQueue market_updates(ME_MAX_CLIENT_UPDATES);

    std::string time_str;
    logger -> log("%:% %() % Starting Matching Engine ... \n",
        __FILE__, __LINE__, __func__,
        getCurrentTimeStr(&time_str));
    matching_engine = new Exchange::MatchingEngine(&client_requests, &client_responses, &market_updates);
    matching_engine -> start();

    const std::string mkt_pub_iface = "lo";
    const std::string snap_pub_ip = "233.252.14.1", inc_pub_ip = "233.252.14.3";
    constexpr int snap_pub_port = 20000, inc_pub_port = 20001;

    logger -> log("%:% %() % Starting Market Data Publisher... \n",
        __FILE__, __LINE__, __func__,
        getCurrentTimeStr(&time_str));
    market_data_publisher = new Exchange::MarketDataPublisher(&market_updates, mkt_pub_iface, snap_pub_ip, snap_pub_port, inc_pub_ip, inc_pub_port);
    market_data_publisher -> start();

    std::string order_gw_iface = "lo";
    constexpr int order_gw_port = 12345;

    logger -> log("%:% % () % Starting Order Server... \n",
        __FILE__, __LINE__, __func__,
        getCurrentTimeStr(&time_str));
    order_server = new Exchange::OrderServer(&client_requests, &client_responses, order_gw_iface, order_gw_port);
    order_server -> start();

    while (true){
        logger -> log("%:% %() % Sleeping for a few milliseconds ...\n",
            __FILE__, __LINE__, __func__,
            getCurrentTimeStr(&time_str));
        usleep(sleep_time * 100);
    }

    return 0;
}