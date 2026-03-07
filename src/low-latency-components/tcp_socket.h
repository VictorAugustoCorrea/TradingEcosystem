#pragma once

#ifndef TRADINGECOSYSTEM_TCP_SOCKET_H
#define TRADINGECOSYSTEM_TCP_SOCKET_H

#include <vector>
#include <cstring>
#include <unistd.h>
#include "logging.h"
#include <functional>
#include <sys/socket.h>
#include <netinet/in.h>
#include "socket_utils.h"

namespace Common
{
    constexpr size_t TCPBufferSize = 64 * 1024 * 1024;

    struct TCPSocket
    {
        explicit TCPSocket(Logger &logger) : logger_(logger)
        {
            outbound_data_.resize(TCPBufferSize);
            inbound_data_.resize(TCPBufferSize);
        }

        auto connect(const std::string &ip, const std::string &iface, const int port, const bool is_listening) -> int
        {
            const SocketCfg socket_cfg{ip, iface, port, false, is_listening, true};

            socket_fd_ = CreateSocket(logger_, socket_cfg);

            socket_attrib_.sin_addr.s_addr = INADDR_ANY;
            socket_attrib_.sin_port = htons(port);
            socket_attrib_.sin_family = AF_INET;

            return socket_fd_;
        }

        auto sendAndRecv() noexcept -> bool
        {
            char ctrl[CMSG_SPACE(sizeof(timeval))] {};

            iovec iov {};
            iov.iov_base = inbound_data_.data() + next_rcv_valid_index_;
            iov.iov_len  = TCPBufferSize - next_rcv_valid_index_;

            msghdr msg {};
            msg.msg_name       = &socket_attrib_;
            msg.msg_namelen    = sizeof(socket_attrib_);
            msg.msg_iov        = &iov;
            msg.msg_iovlen     = 1;
            msg.msg_control    = ctrl;
            msg.msg_controllen = sizeof(ctrl);

            const ssize_t read_size = recvmsg(socket_fd_, &msg, MSG_DONTWAIT);

            if (read_size > 0)
            {
                next_rcv_valid_index_ += read_size;

                Nanos kernel_time = 0;
                timeval time_kernel {};

                for (auto *cmsg = CMSG_FIRSTHDR(&msg);
                     cmsg != nullptr;
                     cmsg = CMSG_NXTHDR(&msg, cmsg))
                {
                    if (cmsg->cmsg_level == SOL_SOCKET &&
                        cmsg->cmsg_type  == SCM_TIMESTAMP)
                    {
                        memcpy(&time_kernel, CMSG_DATA(cmsg), sizeof(time_kernel));

                        kernel_time =
                            time_kernel.tv_sec * NANOS_TO_SECS +
                            time_kernel.tv_usec * NANOS_TO_MICROS;

                        break;
                    }
                }

                const auto user_time = getCurrentNanos();

                logger_.log("%:% %() % read socket: %, len: %, utime: %, k-time: %, diff: %.\n",
                    __FILE__,
                    __LINE__,
                    __func__,
                    getCurrentTimeStr(&time_str_),
                    socket_fd_,
                    next_rcv_valid_index_,
                    user_time,
                    kernel_time,
                    user_time - kernel_time);

                if (recv_callback_)
                    recv_callback_(this, kernel_time);
            }

            if (next_send_valid_index_ > 0)
            {
                const auto n = ::send(socket_fd_,
                                      outbound_data_.data(),
                                      next_send_valid_index_,
                                      MSG_DONTWAIT | MSG_NOSIGNAL);

                logger_.log("%:% %() % send socket:% len:%\n",
                    __FILE__,
                    __LINE__,
                    __func__,
                    getCurrentTimeStr(&time_str_),
                    socket_fd_,
                    n);
            }

            next_send_valid_index_ = 0;

            return read_size > 0;
        }

        auto send(const void *data, const size_t len) noexcept -> void
        {
            memcpy(outbound_data_.data() + next_send_valid_index_, data, len);
            next_send_valid_index_ += len;
        }

        TCPSocket() = delete;
        TCPSocket(const TCPSocket &) = delete;
        TCPSocket(const TCPSocket &&) = delete;
        TCPSocket &operator=(const TCPSocket &) = delete;
        TCPSocket &operator=(const TCPSocket &&) = delete;

        int socket_fd_ = -1;

        std::vector<char> outbound_data_;
        size_t next_send_valid_index_ = 0;

        std::vector<char> inbound_data_;
        size_t next_rcv_valid_index_ = 0;

        sockaddr_in socket_attrib_{};

        std::function<void(TCPSocket *s, Nanos rx_time)> recv_callback_ = nullptr;

        std::string time_str_;
        Logger &logger_;
    };
}

#endif // TRADINGECOSYSTEM_TCP_SOCKET_H