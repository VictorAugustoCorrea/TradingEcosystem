#include "order_server.h"

namespace Exchange {
    OrderServer::OrderServer(
        ClientRequestLFQueue *client_requests,
        MEClientResponseLFQueue *client_responses,
        std::string &iface,
        const int port) :
        logger_("exchange_order_server.log"),
        port_(port),
        tcp_server_(logger_),
        iface_(std::move(iface)),
        fifo_sequencer_(client_requests, &logger_),
        outgoing_responses_(client_responses)
        {
            cid_next_outgoing_seq_num_.fill(1);
            cid_next_exp_seq_num_.fill(1);
            cid_tcp_socket_.fill(nullptr);

            tcp_server_.recv_callback_ = [this](auto socket, auto rx_time) {
                recvCallback(socket, rx_time);
            };

            tcp_server_.recv_finished_callback_ = [this] {
                recvFinishedCallBack();
            };
        }

    auto OrderServer::start() -> void {
        run_ = true;
        tcp_server_.listen(iface_, port_);

        thread_ = createAndStartThread(-1, "Exchange/OrderServer", [this] { run(); } );
        ASSERT(thread_ != nullptr && thread_->joinable(), "Failed to start OrderServer thread.");
    }

    auto OrderServer::stop() -> void {
        run_ = false;
        if (thread_) {
            if (thread_->joinable())
                thread_->join();
            delete thread_;
            thread_ = nullptr;
        }
    }

    OrderServer::~OrderServer() {
        stop();
    }
}
