#pragma once

#ifndef TRADINGECOSYSTEM_TCP_SERVER_H
#define TRADINGECOSYSTEM_TCP_SERVER_H

#include "tcp_socket.h"
#include <sys/epoll.h>
#include <algorithm>

namespace Common
{
    struct TCPServer
    {
        explicit TCPServer(Logger &logger)
            : listener_socket_(logger), logger_(logger)
        {}
        auto addToEpollList(TCPSocket *socket) const {
            epoll_event ev{};
            ev.events = static_cast<uint32_t>(EPOLLET | EPOLLIN);
            ev.data.ptr = reinterpret_cast<void *>(socket);
            return !epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, socket->socket_fd_, &ev);
        }

        void listen(const std::string &iface, const int port)
        {
            epoll_fd_ = epoll_create(1);

            ASSERT(epoll_fd_ >= 0,
                   "epoll_create() failed error:" + std::string(std::strerror(errno)));

            ASSERT(listener_socket_.connect("", iface, port, true) >= 0,
                   "Listener socket failed to connect. iface:" + iface +
                   " port:" + std::to_string(port) +
                   " error:" + std::string(std::strerror(errno)));

            ASSERT(addToEpollList(&listener_socket_),
                   "epoll_ctl() failed. error:" + std::string(std::strerror(errno)));
        }

        void sendAndRecv() noexcept
        {
            bool recv = false;

            std::for_each(receive_sockets_.begin(), receive_sockets_.end(),
                [&recv](auto socket)
                {
                    recv |= socket->sendAndRecv();
                });

            if (recv)
                recv_finished_callback_();

            std::for_each(send_sockets_.begin(), send_sockets_.end(),
                [](auto socket)
                {
                    socket->sendAndRecv();
                });
        }

        void poll() noexcept
        {
            const int max_events = 1 + static_cast<int>(send_sockets_.size()) + static_cast<int>(receive_sockets_.size());

            const int n = epoll_wait(epoll_fd_, events_, max_events, 0);

            bool have_new_connection = false;

            for (int i = 0; i < n; ++i)
            {
                const auto &[events, data] = events_[i];
                auto socket = static_cast<TCPSocket *>(data.ptr);

                if (events & EPOLLIN)
                {
                    if (socket == &listener_socket_)
                    {
                        logger_.log("%:% %() % EPOLL-IN listener_socket: %.\n",
                            __FILE__, __LINE__, __func__,
                            getCurrentTimeStr(&time_str_),
                            socket->socket_fd_);

                        have_new_connection = true;
                        continue;
                    }

                    logger_.log("%:% %() % EPOLL-IN socket: %.\n",
                        __FILE__, __LINE__, __func__,
                        getCurrentTimeStr(&time_str_),
                        socket->socket_fd_);

                    if (std::find(receive_sockets_.begin(),
                                  receive_sockets_.end(),
                                  socket) == receive_sockets_.end())
                    {
                        receive_sockets_.push_back(socket);
                    }
                }

                if (events & EPOLLOUT)
                {
                    logger_.log("%:% %() % EPOLL-OUT socket: %.\n",
                        __FILE__, __LINE__, __func__,
                        getCurrentTimeStr(&time_str_),
                        socket->socket_fd_);

                    if (std::find(send_sockets_.begin(),
                                  send_sockets_.end(),
                                  socket) == send_sockets_.end())
                    {
                        send_sockets_.push_back(socket);
                    }
                }

                if (events & (EPOLLERR | EPOLLHUP))
                {
                    logger_.log("%:% %() % EPOLL-ERR socket: % \n",
                        __FILE__, __LINE__, __func__,
                        getCurrentTimeStr(&time_str_),
                        socket->socket_fd_);

                    if (std::find(receive_sockets_.begin(),
                                  receive_sockets_.end(),
                                  socket) == receive_sockets_.end())
                    {
                        receive_sockets_.push_back(socket);
                    }
                }
            }

            while (have_new_connection)
            {
                logger_.log("%:% %() % have_new_connection\n",
                    __FILE__, __LINE__, __func__,
                    getCurrentTimeStr(&time_str_));

                sockaddr_storage addr{};
                socklen_t addr_len = sizeof(addr);

                const int fd = accept(listener_socket_.socket_fd_,
                                reinterpret_cast<sockaddr *>(&addr),
                                &addr_len);

                if (fd == -1)
                    break;

                ASSERT(setNonBlocking(fd) && disableNagle(fd),
                    "Failed to set non-blocking or no-delay on socket: " +
                    std::to_string(fd));

                logger_.log("%:% %() % accepted socket: %.\n",
                    __FILE__, __LINE__, __func__,
                    getCurrentTimeStr(&time_str_),
                    fd);

                auto socket = new TCPSocket(logger_);
                socket -> socket_fd_ = fd;
                socket -> recv_callback_ = recv_callback_;

                ASSERT(addToEpollList(socket),
                    "Unable to add socket. error:" +
                    std::string(std::strerror(errno)));

                if (std::find(receive_sockets_.begin(),
                              receive_sockets_.end(),
                              socket) == receive_sockets_.end())
                {
                    receive_sockets_.push_back(socket);
                }
            }
        }

        TCPServer() = delete;
        TCPServer(const TCPServer &) = delete;
        TCPServer(const TCPServer &&) = delete;
        TCPServer &operator=(const TCPServer &) = delete;
        TCPServer &operator=(const TCPServer &&) = delete;

        int epoll_fd_ = -1;
        TCPSocket listener_socket_;

        epoll_event events_[1024]{};

        std::vector<TCPSocket *> receive_sockets_;
        std::vector<TCPSocket *> send_sockets_;

        std::function<void(TCPSocket *s, Nanos rx_time)> recv_callback_ = nullptr;
        std::function<void()> recv_finished_callback_ = nullptr;

        std::string time_str_;
        Logger &logger_;
    };
}

#endif // TRADINGECOSYSTEM_TCP_SERVER_H