#pragma once

#ifndef TRADINGECOSYSTEM_ORDER_SERVER_H
#define TRADINGECOSYSTEM_ORDER_SERVER_H

#include <functional>
#include "low-latency-components/tcp_server.h"
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

        auto recvCallback(TCPSocket *socket, const Nanos rx_time) noexcept {
            logger_.log("%:% %() % Received socket: %, len: %, rx: %. \n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_),
                socket -> fd_,
                socket -> next_rcv_valid_index_,
                rx_time);
            if (socket -> next_rcv_valid_index_ >= sizeof(OMClientRequest)) {
                size_t i = 0;
                for ( ; i + sizeof(OMClientRequest) <= socket -> next_rcv_valid_index_; i += sizeof(OMClientRequest)) {
                    const auto request = reinterpret_cast<const OMClientRequest*> (socket -> rcv_buffer_ + 1);
                    logger_.log("%:% %() % Received: % \n",
                        __FILE__, __LINE__, __func__,
                        getCurrentTimeStr(&time_str_),
                        request -> toString());
                    if (UNLIKELY(cid_tcp_socket_[request -> me_client_request_.client_id_] == nullptr)) {
                        cid_tcp_socket_[request -> me_client_request_.client_id_] = socket;
                    }
                    if (cid_tcp_socket_[request -> me_client_request_.client_id_] != socket) {
                        logger_.log("%:% %() % Received ClientRequest from Client_id: % on different socket: %. Expected: %. \n",
                            __FILE__, __LINE__, __func__,
                            getCurrentTimeStr(&time_str_),
                            request -> me_client_request_.client_id_,
                            socket -> fd_,
                            cid_tcp_socket_[request -> me_client_request_.client_id_] -> fd_);
                        continue;
                    }
                    auto &next_exp_seq_num = cid_next_exp_seq_num_[request -> me_client_request_.client_id_];
                    if (request -> seq_num_ != next_exp_seq_num) {
                        logger_.log("%:% %() % Incorrect sequence number. Client_id: %, SeqNum expected: %, received: %. \n",
                            __FILE__, __LINE__, __func__,
                            getCurrentTimeStr(&time_str_),
                            request -> me_client_request_.client_id_,
                            next_exp_seq_num,
                            request -> seq_num_);
                        continue;
                    }
                    ++next_exp_seq_num;
                    fifo_sequencer_.addClientRequest(rx_time, request -> me_client_request_);
                }
                memcpy(socket -> rcv_buffer_, socket -> rcv_buffer_ + 1, socket -> next_rcv_valid_index_ - i);
                socket -> next_rcv_valid_index_ -= i;
            }
        }

        auto recvFinishedCallBack() noexcept {
            fifo_sequencer_.sequenceAndPublish();
        }

        OrderServer() = delete;
        OrderServer(const OrderServer & ) = delete;
        OrderServer(const OrderServer &&) = delete;
        OrderServer &operator = (const OrderServer & ) = delete;
        OrderServer &operator = (const OrderServer &&) = delete;

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