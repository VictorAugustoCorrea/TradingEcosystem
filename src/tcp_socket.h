#pragma once

#ifndef TRADINGECOSYSTEM_TCP_SOCKET_H
#define TRADINGECOSYSTEM_TCP_SOCKET_H

#include <unistd.h>
#include "logging.h"
#include <functional>
#include "socket_utils.h"

namespace Common
{
    constexpr size_t TCPBufferSize = 64 * 1024 * 1024;

    struct TCPSocket
    {
        sockaddr_in socket_attribute_ {};
        std::function<void(TCPSocket *s, Nanos rx_time)> recv_callback_ = nullptr;
        std::string time_str_;
        Logger &logger_;

        int fd_ = -1;
        char *send_buffer_ = nullptr;
        char *rcv_buffer_  = nullptr;

        size_t next_send_valid_index_ = 0;
        size_t next_rcv_valid_index_  = 0;

        bool send_disconnected_ = false;
        bool recv_disconnected_ = false;

        void defaultRecvCallBack(const TCPSocket *socket, const Nanos rx_time) noexcept
        {
            logger_.log("%:% %() % TCPSocket::defaultRecvCallBack() socket: % len: % rx: % \n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_),
                socket -> fd_, socket -> next_rcv_valid_index_, rx_time);
        }

        explicit TCPSocket(Logger &logger) : logger_(logger)
        {
            send_buffer_ = new char[TCPBufferSize];
            rcv_buffer_  = new char[TCPBufferSize];
            recv_callback_ = [this] (auto socket, auto rx_time)
            {
                defaultRecvCallBack(socket, rx_time);
            };
        }
        void destroy() noexcept
        {
            close(fd_);
            fd_ = -1;
        }

        ~TCPSocket()
        {
            destroy();
            delete[] send_buffer_; send_buffer_ = nullptr;
            delete[] rcv_buffer_ ; rcv_buffer_  = nullptr;
        }

        auto connect(const std::string &ip, const std::string &iface, const int port, const bool is_listening) -> int
        {
            destroy();
            fd_ = CreateSocket(logger_, ip, iface, port, false, false, is_listening, 0, true);

            socket_attribute_.sin_addr.s_addr = INADDR_ANY;
            socket_attribute_.sin_port = htons(port);
            socket_attribute_.sin_family = AF_INET;

            return fd_;
        }

        void send(const void * data, const size_t len) noexcept
        {
            if (len > 0)
            {
                memcpy(send_buffer_ + next_send_valid_index_, data, len);
                next_send_valid_index_ += len;
            }
        }

        auto sendAndRecv() noexcept -> bool
        {
            char ctrl[CMSG_SPACE(sizeof( timeval ))] = {};
            const auto* cmsg = reinterpret_cast<struct cmsghdr *> (ctrl);

            iovec iov = {};
            iov.iov_base = rcv_buffer_ + next_rcv_valid_index_;
            iov.iov_len = TCPBufferSize - next_rcv_valid_index_;

            msghdr msg {};
            msg.msg_control = ctrl;
            msg.msg_controllen = sizeof(ctrl);
            msg.msg_name = &socket_attribute_;
            msg.msg_namelen = sizeof(socket_attribute_);
            msg.msg_iov = &iov;
            msg.msg_iovlen = 1;

            const auto n_rcv = recvmsg(fd_, &msg, MSG_DONTWAIT);
            if (n_rcv > 0)
            {
                next_rcv_valid_index_ += n_rcv;

                Nanos kernel_time = 0;
                timeval time_kernel = {};
                if (cmsg -> cmsg_level == SOL_SOCKET &&
                    cmsg -> cmsg_type == SCM_TIMESTAMP &&
                    cmsg -> cmsg_len == CMSG_LEN(sizeof(time_kernel)))
                {
                    memcpy(&time_kernel, CMSG_DATA(cmsg), sizeof(time_kernel));
                    kernel_time = time_kernel.tv_sec * NANOS_TO_SECS + time_kernel.tv_usec * NANOS_TO_MICROS;
                }

                const auto user_time = getCurrentNanos();

                logger_.log("%:% %() % read socket: %, len: %, utime: %, k_time: %, diff: % \n",
                    __FILE__, __LINE__, __func__,
                    getCurrentTimeStr(&time_str_),
                    fd_,
                    next_rcv_valid_index_,
                    user_time,
                    kernel_time,
                    user_time - kernel_time);
                recv_callback_(this, kernel_time);
            }

            size_t n_send = std::min(TCPBufferSize, next_send_valid_index_);
            while (n_send > 0)
            {
                const auto n_send_this_msg = std::min(static_cast<size_t>(next_send_valid_index_), n_send);
                const int flags = MSG_DONTWAIT | MSG_NOSIGNAL | (n_send_this_msg < n_send? MSG_MORE : 0);
                const auto n = ::send(fd_, send_buffer_, n_send_this_msg, flags);
                if (UNLIKELY(n < 0))
                {
                    if (!wouldBlock())
                        send_disconnected_ = true;
                    break;
                }

                logger_.log("%:% %() % send socket: %, len: % \n",
                    __FILE__, __LINE__, __func__,
                    getCurrentTimeStr(&time_str_),
                    fd_, n );

                n_send -= n;
                ASSERT(n == n_send_this_msg, "Don't support partial send lengths yet.");
            }
            next_send_valid_index_ = 0;
            return n_rcv > 0;
        }

        TCPSocket() = delete;
        TCPSocket(const TCPSocket & ) = delete;
        TCPSocket(const TCPSocket &&) = delete;
        TCPSocket &operator = (const TCPSocket & ) = delete;
        TCPSocket &operator = (const TCPSocket &&) = delete;
    };
}

#endif //TRADINGECOSYSTEM_TCP_SOCKET_H