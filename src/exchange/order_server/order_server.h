#pragma once

#ifndef TRADINGECOSYSTEM_ORDER_SERVER_H
#define TRADINGECOSYSTEM_ORDER_SERVER_H

#include <functional>
#include "low-latency-components/macros.h"
#include "low-latency-components/tcp_server.h"
#include "low-latency-components/thread_utils.h"
#include "exchange/order_server/fifo_sequencer.h"
#include "exchange/order_server/client_request.h"
#include "exchange/order_server/client_response.h"

namespace Exchange {
    class OrderServer {
    public:
        OrderServer(ClientRequestLFQueue *client_requests, MEClientResponseLFQueue *client_responses, const std::string &iface, int port);
        ~OrderServer();
        auto start() -> void;
        auto stop()  -> void;

    private:
        Logger logger_;
        const int port_ = 0;
        TCPServer tcp_server_;
        std::string time_str_;
        const std::string iface_;
        volatile bool run_ =  false;
        FIFOSequencer fifo_sequencer_;
        MEClientResponseLFQueue * outgoing_responses_ = nullptr;
        std::array<size_t, ME_MAX_NUM_CLIENTS> cid_next_exp_seq_num_ = {};
        std::array<TCPSocket *, ME_MAX_NUM_CLIENTS> cid_tcp_socket_  = {};
        std::array<size_t, ME_MAX_NUM_CLIENTS> cid_next_outgoing_seq_num_ = {};
    };
}

#endif //TRADINGECOSYSTEM_ORDER_SERVER_H