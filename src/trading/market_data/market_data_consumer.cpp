#include "market_data_consumer.h"

namespace Trading {
    MarketDataConsumer::MarketDataConsumer(
        const ClientId client_id,
        Exchange::MEMarketUpdateLFQueue *market_updates,
        const std::string &iface,
        const std::string &snapshot_ip,
        const int &snapshot_port,
        const std::string &incremental_ip,
        const int &incremental_port) :
    incoming_md_updates_(market_updates),
    run_( false ),
    logger_("trading_market_data_consumer_" + std::to_string(client_id) + ".log"),
    incremental_mcast_socket_(logger_),
    snapshot_mcast_socket_(logger_),
    iface_(iface),
    snapshot_ip_(snapshot_ip),
    snapshot_port_(snapshot_port) {

        auto recv_callback = [this](auto socket) -> void {
            recvCallback(socket);
        };

        incremental_mcast_socket_.recv_callback_ = recv_callback;
        ASSERT(incremental_mcast_socket_.init(incremental_ip, iface, incremental_port, true) >= 0,
            "Unable to create incremental mcast socket. Error: " + std::string(strerror(errno)));

        ASSERT(incremental_mcast_socket_.join(incremental_ip),
            "Join failed on: " + std::to_string(incremental_mcast_socket_.socket_fd_) + " error: " + std::string(strerror(errno)));
    }

    auto MarketDataConsumer::run() noexcept -> void {
        logger_.log("%:% %() %. \n",
            __FILE__, __LINE__, __func__,
            getCurrentTimeStr(&time_str_));
        while (run_) {
            incremental_mcast_socket_.sendAndRecv();
            snapshot_mcast_socket_.sendAndRecv();
        }
    }
}
