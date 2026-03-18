#include "market_data_consumer.h"

#include <strstream>

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

    auto MarketDataConsumer::startSnapshotSync() -> void {
        snapshot_queued_msgs_.clear();
        incremental_queued_msgs_.clear();

        ASSERT(snapshot_mcast_socket_.init(snapshot_ip_, iface_, snapshot_port_, true) >= 0,
            "Unable to create snapshot mcast socket. Error: " + std::string(std::strerror(errno)));

        ASSERT(snapshot_mcast_socket_.join(snapshot_ip_),
            "Join failed on: " + std::to_string(snapshot_mcast_socket_.socket_fd_) + " error: " + std::string(strerror(errno)));
    }

    auto MarketDataConsumer::recvCallback(McastSocket *socket) noexcept -> void {
        const auto is_snapshot = socket -> socket_fd_ == snapshot_mcast_socket_.socket_fd_;
        if (UNLIKELY(is_snapshot && !in_recovery_)) {
            socket -> next_rcv_valid_index_ = 0;

            logger_.log("%:% %() % WARN! Not expecting snapshot messages. \n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_));
            return;
        }
        if (socket -> next_rcv_valid_index_ >= sizeof(Exchange::MDPMarketUpdate)) {
            size_t i = 0;
            for ( ; i + sizeof(Exchange::MDPMarketUpdate) <= socket -> next_rcv_valid_index_; i += sizeof(Exchange::MDPMarketUpdate)) {
                const auto request = reinterpret_cast<const Exchange::MDPMarketUpdate *> (socket -> inbound_data_.data() + i);
                logger_.log("%:% %() % Received: % . Socket len: %, \n",
                    __FILE__, __LINE__, __func__,
                    getCurrentTimeStr(&time_str_),
                    is_snapshot ? "snapshot" : "incremental",
                    sizeof(Exchange::MDPMarketUpdate),
                    request -> toString());

                const bool already_in_recovery = in_recovery_;
                in_recovery_ = already_in_recovery || request -> seq_num_ != next_exp_inc_seq_num_;

                if (UNLIKELY(in_recovery_)) {
                    if (UNLIKELY(!already_in_recovery)) {
                        logger_.log("%:% %() % Packet drops on % socket. Seq Num expected: %, received: %. \n",
                            __FILE__, __LINE__, __func__,
                            getCurrentTimeStr(&time_str_),
                            is_snapshot ? "snapshot" : "incremental",
                            next_exp_inc_seq_num_,
                            request -> seq_num_);
                        startSnapshotSync();
                    }
                    queueMessage(is_snapshot, request);
                }
                else if (!is_snapshot) {
                    logger_.log("%:% %() % %. \n",
                        __FILE__, __LINE__, __func__,
                        getCurrentTimeStr(&time_str_),
                        request -> toString());
                    ++next_exp_inc_seq_num_;

                    const auto next_write = incoming_md_updates_ -> getNextToWriteTo();
                    *next_write = std::move(request -> me_market_update_);
                    incoming_md_updates_ -> updateWriteIndex();
                }
            }
            memcpy(socket -> inbound_data_.data(), socket -> inbound_data_.data() + i, socket -> next_rcv_valid_index_ - i);
            socket -> next_rcv_valid_index_ -= i;
        }
    }

}
