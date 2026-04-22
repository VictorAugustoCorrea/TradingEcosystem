#include <csignal>
#include <cstdlib>
#include <iostream>

#include "low-latency-components/logging.h"
#include "low-latency-components/lock_free_queue.h"
#include "exchange/matcher/matching_engine.h"

#include "exchange/exchange_application.h"
#include "trading/trade_client_application.h"

using namespace Common;
using namespace std::literals::chrono_literals;

/** ------------------------------------------------------------------ *
 *  Globals (needed by the signal handler)
 * ------------------------------------------------------------------ */
static Logger*                      exchange_logger  = nullptr;
static Logger*                      trading_logger   = nullptr;
static App::ExchangeApplication*    exchange_app     = nullptr;
static App::TradeClientApplication* trade_client_app = nullptr;
static volatile std::sig_atomic_t   stop_requested   = 0;

/** ------------------------------------------------------------------ *
 *  Signal handler — graceful shutdown on Ctrl-C
 * ------------------------------------------------------------------ */
static void signal_handler(int) {
    stop_requested = 1;
}

/** ------------------------------------------------------------------ *
 *  Argument parsing
 * ------------------------------------------------------------------ */
static TradeEngineCfgHashMap parseTickers(const int argc, char** argv, const int start_idx) {
    TradeEngineCfgHashMap cfg;
    size_t ticker_id = 0;

    for (int i = start_idx; i + 4 < argc; i += 5, ++ticker_id) {
        if (ticker_id >= ME_MAX_TICKERS) {
            std::cerr << "Too many ticker configs. Max supported is " << ME_MAX_TICKERS
                      << ", ignoring extra arguments.\n";
            break;
        }

        cfg[ticker_id] = {
            static_cast<Qty>(strtol(argv[i],     nullptr, 10)),
            std::strtod(argv[i + 1], nullptr),
            {
                static_cast<Qty>(strtol(argv[i + 2], nullptr, 10)),
                static_cast<Qty>(strtol(argv[i + 3], nullptr, 10)),
                std::strtod(argv[i + 4], nullptr)
            }
        };
    }

    return cfg;
}

/** ------------------------------------------------------------------ *
 *  main
 *
 *  Exchange + local client:
 *                    <binary> --exchange <client_id> <algo_type> [ticker params...]
 *  Exchange only:    <binary> --exchange-only
 *  Client only:      <binary>            <client_id> <algo_type> [ticker params...]
 * ------------------------------------------------------------------ */
int main(const int argc, char** argv) {

    if (argc < 2) {
        std::cerr << "Usage: [--exchange] <client_id> <algo_type> [ticker params (5 each)...]\n";
        std::cerr << "   or: --exchange-only\n";
        return EXIT_FAILURE;
    }

    std::signal(SIGINT, signal_handler);

    /** Detect --exchange flag */
    int  arg_offset     = 1;
    bool start_exchange = false;
    bool exchange_only  = false;

    if (std::string(argv[1]) == "--exchange-only") {
        exchange_only = true;
        start_exchange = true;
    } else if (std::string(argv[1]) == "--exchange") {
        start_exchange = true;
        arg_offset     = 2;

        if (argc < 4) {
            std::cerr << "Usage: --exchange <client_id> <algo_type> [ticker params (5 each)...]\n";
            return EXIT_FAILURE;
        }
    } else if (argc < 3) {
        std::cerr << "Usage: <client_id> <algo_type> [ticker params (5 each)...]\n";
        return EXIT_FAILURE;
    }

    const auto client_id  = exchange_only ? ClientId{0} : static_cast<ClientId>(strtol(argv[arg_offset], nullptr, 10));
    srand(client_id);

    const auto algo_type  = exchange_only ? AlgoType::INVALID : stringToAlgoType(argv[arg_offset + 1]);
    const auto ticker_cfg = exchange_only ? TradeEngineCfgHashMap{} : parseTickers(argc, argv, arg_offset + 2);

    /** Loggers */
    if (start_exchange) {
        exchange_logger = new Logger("exchange_main.log");
    }
    if (!exchange_only) {
        trading_logger  = new Logger("trading_main_" + std::to_string(client_id) + ".log");
    }

    /**
     * Every process needs its own queues:
     *   --exchange mode : queues are shared between ExchangeApplication and TradeEngine
     *                     (same process, zero-copy via lock-free queues)
     *   client-only mode: queues are local — MarketDataConsumer → market_updates → TradeEngine
     *                     and TradeEngine ↔ OrderGateway via client_requests/responses
     */
    Exchange::ClientRequestLFQueue    client_requests(ME_MAX_CLIENT_UPDATES);
    Exchange::MEClientResponseLFQueue client_responses(ME_MAX_CLIENT_UPDATES);
    Exchange::MEMarketUpdateLFQueue   market_updates(ME_MAX_CLIENT_UPDATES);

    /** Start exchange (only the designated host process) */
    if (start_exchange) {
        exchange_app = new App::ExchangeApplication(
            &client_requests, &client_responses, &market_updates,
            exchange_logger);
        exchange_app->start();
    }

    if (exchange_only) {
        while (!stop_requested) {
            std::this_thread::sleep_for(1s);
        }

        delete exchange_app;     exchange_app     = nullptr;
        delete exchange_logger;  exchange_logger  = nullptr;
        return EXIT_SUCCESS;
    }

    /** Start trading client — always receives real queue pointers */
    trade_client_app = new App::TradeClientApplication(
        client_id, algo_type, ticker_cfg,
        &client_requests,
        &client_responses,
        &market_updates,
        start_exchange,
        trading_logger);
    trade_client_app->start();

    /** Run strategy-specific logic */
    if (algo_type == AlgoType::RANDOM)
        trade_client_app->runRandomAlgo();

    /** Wait for the engine to go quiet, then shut down cleanly */
    trade_client_app->waitUntilSilent(/*threshold_seconds=*/30, /*poll_seconds=*/5);
    trade_client_app->stop();

    delete trade_client_app; trade_client_app = nullptr;
    delete exchange_app;     exchange_app     = nullptr;
    delete trading_logger;   trading_logger   = nullptr;
    delete exchange_logger;  exchange_logger  = nullptr;

    std::this_thread::sleep_for(2s);
    return EXIT_SUCCESS;
}
