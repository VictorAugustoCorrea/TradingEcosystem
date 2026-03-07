#pragma once

#ifndef TRADINGECOSYSTEM_MCAST_SOCKET_H
#define TRADINGECOSYSTEM_MCAST_SOCKET_H

#include <vector>
#include <string>
#include <functional>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include "low-latency-components/socket_utils.h"
#include "low-latency-components/logging.h"

namespace Common {
    constexpr size_t McastBufferSize = 64 * 1024 * 1024;

    struct McastSocket {
        explicit McastSocket(Logger &logger)
        : logger_(logger) {
            outbound_data_.resize(McastBufferSize);
            inbound_data_.resize(McastBufferSize);
        }

        auto init(const std::string &ip, const std::string &iface, const int port, const bool is_listening) -> int {
            const SocketCfg socket_cfg{ip, iface, port, true, is_listening, false};
            socket_fd_ = CreateSocket(logger_, socket_cfg);
            return socket_fd_;
        }

        auto join(const std::string &ip) const -> bool {
            return Common::join(socket_fd_, ip);
        }

        auto leave(const std::string &, int) -> void {
            close(socket_fd_);
            socket_fd_ = -1;
        }

        auto sendAndRecv() noexcept -> bool {
            const ssize_t n_rcv = recv(
                socket_fd_,
                inbound_data_.data() + next_rcv_valid_index_,
                McastBufferSize - next_rcv_valid_index_,
                MSG_DONTWAIT);

            if (n_rcv > 0) {
                next_rcv_valid_index_ += n_rcv;

                logger_.log("%:% %() % read socket: %, len: %\n",
                    __FILE__, __LINE__, __func__,
                    getCurrentTimeStr(&time_str_),
                    socket_fd_,
                    next_rcv_valid_index_);
                recv_callback_(this);
            }

            if (next_send_valid_index_ > 0) {
                const ssize_t n = ::send(socket_fd_,
                    outbound_data_.data(),
                    next_send_valid_index_,
                    MSG_DONTWAIT | MSG_NOSIGNAL);
                logger_.log("%:% %() % send socket: % , len: %\n",
                    __FILE__, __LINE__, __func__,
                    getCurrentTimeStr(&time_str_),
                    socket_fd_,
                    n);
            }
            next_send_valid_index_ = 0;
            return n_rcv > 0;
        }

        auto send(const void *data, const size_t len) noexcept -> void {
            memcpy(outbound_data_.data() + next_send_valid_index_, data, len);
            next_send_valid_index_ += len;
            ASSERT(next_send_valid_index_ < McastBufferSize,
                "Mcast socket buffer filled up and sendAndRecv() not called.");
        }

        int socket_fd_ = -1;
        std::vector<char> outbound_data_;
        size_t next_send_valid_index_ = 0;
        std::vector<char> inbound_data_;
        size_t next_rcv_valid_index_ = 0;
        std::function<void(McastSocket *s)> recv_callback_ = nullptr;
        std::string time_str_;
        Logger &logger_;
    };
}

#endif