#pragma once

#ifndef TRADINGECOSYSTEM_ORDER_GATEWAY_H
#define TRADINGECOSYSTEM_ORDER_GATEWAY_H

#include <functional>
#include "low-latency-components/macros.h"
#include "low-latency-components/tcp_server.h"
#include "low-latency-components/thread_utils.h"
#include "exchange/order_server/client_request.h"
#include "exchange/order_server/client_response.h"

namespace Trading {
    class OrderGateway {
    public:
        OrderGateway(
            ClientId client_id,
            Exchange::ClientRequestLFQueue *client_requests,
            Exchange::MEClientResponseLFQueue *client_responses,
            std::string ip,
            std::string iface,
            int port);

        auto run() noexcept -> void;
        auto recvCallback(TCPSocket *socket, Nanos rx_time) noexcept -> void;

        auto start() -> void {
            run_ = true;
            ASSERT(tcp_socket_.connect(ip_, iface_, port_, false ) >= 0,
                "Unable to connect to ip: " + ip_ + ", port: " + std::to_string(port_) + " on iface: " + iface_ + ". Error: " + std::to_string(errno));
            ASSERT(createAndStartThread(-1, "Trading/OrderGateway", [this] { run(); }) != nullptr,
                "Failed to start OrderGateway thread.");
        }

        auto stop() -> void {
            run_ = false;
        }

        ~OrderGateway() {
            stop();
            using namespace std::literals::chrono_literals;
            std::this_thread::sleep_for(5s);
        }

        OrderGateway() = delete;
        OrderGateway(const OrderGateway & ) = delete;
        OrderGateway(const OrderGateway &&) = delete;
        OrderGateway & operator=(const OrderGateway & ) = delete;
        OrderGateway & operator=(const OrderGateway &&) = delete;

    private:
        const ClientId client_id_;
        std::string ip_;
        const std::string iface_;
        const int port_ = 0;
        Exchange::ClientRequestLFQueue *outgoing_requests_ = nullptr;
        Exchange::MEClientResponseLFQueue *incoming_responses_ = nullptr;
        volatile bool run_ = false;
        std::string time_str_;
        Logger logger_;
        size_t next_outgoing_seq_num_ = 1;
        size_t next_exp_seq_num_ = 1;
        TCPSocket tcp_socket_;
    };
}

#endif //TRADINGECOSYSTEM_ORDER_GATEWAY_H