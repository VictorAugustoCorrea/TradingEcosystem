#pragma once

#ifndef TRADINGECOSYSTEM_TCP_SERVER_H
#define TRADINGECOSYSTEM_TCP_SERVER_H

#include "tcp_socket.h"
#include <sys/epoll.h>

namespace Common
{
    struct TCPServer
    {
        int efd_ = -1;
        TCPSocket listener_socket_;

        std::string time_str_;
        Logger &logger_;
        bool have_new_connection =  false;

        epoll_event events_[1024] {};
        std::vector<TCPSocket *> sockets_, receive_sockets_, send_sockets_, disconnected_sockets_;

        std::function<void(TCPSocket *s, Nanos rx_time)> recv_callback_;
        std::function<void()> recv_finished_callback_;

        explicit TCPServer(Logger &logger) : listener_socket_(logger), logger_(logger)
        {
            recv_callback_ = [this] (auto socket, auto rx_time)
            {
                defaultRecvCallback(socket, rx_time);
            };
            recv_finished_callback_ = [this]
            {
                defaultRecvFinishedCallback();
            };
        }

        void destroy()
        {
            close(efd_);
            efd_ = -1;
            listener_socket_.destroy();
        }

        bool epoll_add(TCPSocket *socket) const {
            epoll_event ev {};
            ev.events = EPOLLET | EPOLLIN;
            ev.data.ptr = reinterpret_cast<void *> (socket);
            return epoll_ctl(efd_, EPOLL_CTL_ADD, socket -> fd_,  &ev) != -1;
        }

        bool epoll_del(const TCPSocket *socket) const
        {
            return epoll_ctl(efd_, EPOLL_CTL_DEL, socket -> fd_, nullptr) != -1;
        }

        void del(const TCPSocket *socket)
        {
            ASSERT(epoll_del(socket), "epoll_del() failed. error: " + std::string(std::strerror(errno)));

            std::erase(sockets_, socket);
            std::erase(receive_sockets_, socket);
            std::erase(send_sockets_, socket);
        }

        void defaultRecvCallback(const TCPSocket *socket, const Nanos rx_time) noexcept
        {
            logger_.log("%:% %() % TCPServer::defaultRecvCallback() socket: %, len: %, rx: % \n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_),
                socket -> fd_,
                socket -> next_rcv_valid_index_,
                rx_time);
        }

        void defaultRecvFinishedCallback() noexcept
        {
            logger_.log("%:% %() % TCPServer()::defaultRecvFinishedCallback() \n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr(&time_str_));
        }

        void listen(const std::string &iface, const int port)
        {
            destroy();
            efd_ = epoll_create(1);

            ASSERT(efd_ >= 0,"epoll_create() failed error: " + std::string(std::strerror(errno)));
            ASSERT(listener_socket_.connect("", iface, port, true) >= 0,
                "Listener socket failed to connect, iface: "
                + iface + " port: " + std::to_string(port) + " error: " + std::string(std::strerror(errno)));

            ASSERT(epoll_add(&listener_socket_), "epoll_ctl() failed. error: " + std::string(std::strerror(errno)));
        }

        void poll() noexcept
        {
            const int max_events = static_cast<int>(sockets_.size()) + 1;

            for (const auto* socket: disconnected_sockets_)
            {
                del(socket);
            }

            const int n = epoll_wait(efd_, events_, max_events, 0);

            for (int i = 0; i < n; ++i)
            {
                auto [events, data] = events_[i];
                auto socket = static_cast<TCPSocket *>(data.ptr);
                if (events & EPOLLIN)
                {
                    if (socket == &listener_socket_)
                    {
                        logger_.log("%:% %() % EPOLL-IN listener socket: % \n",
                            __FILE__, __LINE__, __func__,
                            getCurrentTimeStr(&time_str_),
                            socket -> fd_),
                        have_new_connection = true;
                        continue;
                    }
                    logger_.log("%:% %() % EPOLL-IN socket: % \n",
                        __FILE__, __LINE__, __func__,
                        getCurrentTimeStr(&time_str_),
                        socket -> fd_);
                    if (std::find(receive_sockets_.begin(), receive_sockets_.end(), socket) == receive_sockets_.end())
                        receive_sockets_.push_back(socket);
                }
                if (events & EPOLLOUT)
                {
                    logger_.log("%:% %() % EPOLL-OUT socket: %\n",
                        __FILE__, __LINE__, __func__,
                        getCurrentTimeStr(&time_str_),
                        socket -> fd_);
                    if (std::find(send_sockets_.begin(), send_sockets_.end(), socket) == send_sockets_.end())
                        send_sockets_.push_back(socket);
                }
                if (events & (EPOLLERR | EPOLLHUP))
                {
                    logger_.log("%:% %() % EPOLL-ERR socket: % \n",
                        __FILE__, __LINE__, __func__,
                        getCurrentTimeStr(&time_str_),
                        socket -> fd_);
                    if (std::find(disconnected_sockets_.begin(), disconnected_sockets_.end(), socket)
                        == disconnected_sockets_.end())
                        disconnected_sockets_.push_back(socket);
                }
            }
            while (have_new_connection)
            {
                logger_.log("%:% %() % have_new_connection \n",
                    __FILE__, __LINE__, __func__,
                    getCurrentTimeStr(&time_str_));

                sockaddr_storage addr {};
                socklen_t addr_len = sizeof(addr);
                const int fd = accept(listener_socket_.fd_, reinterpret_cast<sockaddr *>(&addr), &addr_len);

                if (fd == -1)
                    break;

                ASSERT(setNonBlocking(fd) && setNoDelay(fd), "Failed to set non-blocking or no-delay on socket: "
                    + std::to_string(fd));
                logger_.log("%:% %() % accepted socket: % \n",
                    __FILE__, __LINE__, __func__,
                    getCurrentTimeStr(&time_str_),
                    fd);

                auto *socket = new TCPSocket(logger_);
                socket -> fd_ = fd;
                socket -> recv_callback_ = recv_callback_;
                ASSERT(epoll_add(socket), "Unable to add socket. error: " + std::string(std::strerror(errno)));

                if (std::find(sockets_.begin(), sockets_.end(), socket) == sockets_.end())
                    sockets_.push_back(socket);
                if (std::find(receive_sockets_.begin(), receive_sockets_.end(), socket) == receive_sockets_.end())
                    receive_sockets_.push_back(socket);
            }
        }

        void sendAndRecv() noexcept
        {
            bool recv = false;

            for (auto* socket : receive_sockets_)
                recv |= socket->sendAndRecv();

            if (recv)
                recv_finished_callback_();

            for (auto* socket : send_sockets_)
                socket->sendAndRecv();

            receive_sockets_.clear();
            send_sockets_.clear();
        }

        TCPServer() = delete;
        TCPServer(const TCPServer & ) = delete;
        TCPServer(const TCPServer &&) = delete;
        TCPServer &operator = (const TCPServer & ) = delete;
        TCPServer &operator = (const TCPServer &&) = delete;
    };
}

#endif //TRADINGECOSYSTEM_TCP_SERVER_H