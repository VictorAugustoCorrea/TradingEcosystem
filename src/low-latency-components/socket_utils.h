#pragma once

#ifndef TRADINGECOSYSTEM_SOCKET_UTILS_H
#define TRADINGECOSYSTEM_SOCKET_UTILS_H

#include <string>
#include <cstring>
#include <netdb.h>
#include <fcntl.h>
#include <sstream>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unordered_set>

#include "macros.h"
#include "logging.h"

namespace Common
{
    constexpr int MaxTCPServerBacklog = 1024;

    struct SocketCfg {
        std::string ip_;
        std::string iface_;
        int port_ = -1;
        bool is_udp_ = false;
        bool is_listening_ = false;
        bool needs_so_timestamp_ =  false;

        [[nodiscard]]
        auto toString() const {
            std::stringstream ss;
            ss  << "SocketCfg [ "
                << " ip: " << ip_
                << " iface: " << iface_
                << " port: " << port_
                << " is_udp: " << is_udp_
                << " is_listening: " << is_listening_
                << " needs_SO_timestamp: " << needs_so_timestamp_
                << " ] ";
            return ss.str();
        }
    };

    inline auto getIfaceIP(const std::string &iface) -> std::string
    {
        char buf[NI_MAXHOST] = {};
        ifaddrs *ifaddr = nullptr;

        if (getifaddrs(&ifaddr) != - 1)
        {
            for (const ifaddrs *ifa = ifaddr; ifa; ifa = ifa -> ifa_next)
            {
                 if (ifa -> ifa_addr && ifa -> ifa_addr -> sa_family == AF_INET && iface == ifa -> ifa_name) {
                     getnameinfo(ifa->ifa_addr,
                         sizeof(sockaddr_in),
                         buf, sizeof(buf),
                         nullptr,
                         0,
                         NI_NUMERICHOST);
                     break;
                 }
            }
            freeifaddrs(ifaddr);
        }
        return buf;
    }

    inline auto setNonBlocking(const int fd) -> bool
    {
        const auto flags = fcntl(fd, F_GETFL, 0);
        if (flags == -1)
            return false;
        if (flags & O_NONBLOCK)
            return true;
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
    }

    inline auto setNoDelay(const int fd) -> bool
    {
        constexpr int one = 1;
        return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) != -1;
    }

    inline auto setSOTimestamp(const int fd) -> bool
    {
        constexpr int one = 1;
        return setsockopt(fd, SOL_SOCKET, SO_TIMESTAMP, &one, sizeof(one)) != -1;
    }

    inline auto wouldBlock() -> bool
    {
        return errno == EWOULDBLOCK || errno == EINPROGRESS;
    }

    inline auto setMcastTTL(const int fd, const int mcast_ttl) -> bool
    {
        return setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL, &mcast_ttl, sizeof(mcast_ttl)) != -1;
    }

    inline auto setTTL(const int fd, const int ttl) -> bool
    {
        return setsockopt(fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) != -1;
    }

    inline auto disableNagle(const int fd) -> bool {
        constexpr int one = 1;
        return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one)) != -1;
    }


    inline auto join(const int fd, const std::string &ip) -> bool {
        const ip_mreq mreq{
            { inet_addr(ip.c_str()) },
            { htonl(INADDR_ANY) }
        };
        return setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) != -1;
    }

    inline auto CreateSocket(Logger &logger, const SocketCfg& socket_cfg) -> int {
        std::string time_str;
        const auto ip = socket_cfg.ip_.empty() ? getIfaceIP(socket_cfg.iface_) : socket_cfg.ip_;
        logger.log("%:% %() % cfg: % \n",
            __FILE__, __LINE__, __func__,
            getCurrentTimeStr(&time_str),
            socket_cfg.toString());

        const int input_flags = (socket_cfg.is_listening_ ? AI_PASSIVE : 0) | (AI_NUMERICHOST | AI_NUMERICSERV);
        const addrinfo hints {
            input_flags,
            AF_INET,
            socket_cfg.is_udp_ ? SOCK_DGRAM : SOCK_STREAM,
            socket_cfg.is_udp_ ? IPPROTO_UDP : IPPROTO_TCP,
            0,
            nullptr,
            nullptr,
            nullptr};
        addrinfo *result = nullptr;
        const auto rc = getaddrinfo(ip.c_str(), std::to_string(socket_cfg.port_).c_str(), &hints, &result);
        ASSERT(!rc, "getaddrinfo() failed. error:" + std::string(gai_strerror(rc)) + "errno:" + strerror(errno));

        int socket_fd = -1;
        constexpr int one = 1;
        for (const addrinfo *rp = result; rp; rp = rp -> ai_next) {
            ASSERT((socket_fd = socket(rp -> ai_family, rp -> ai_socktype, rp -> ai_protocol)) != -1, "socket() failed. errno:" + std::string(strerror(errno)));
            ASSERT(setNonBlocking(socket_fd), "setNonBlocking() failed. errno:" + std::string(strerror(errno)));

            if (!socket_cfg.is_udp_) {
                ASSERT(disableNagle(socket_fd), "disableNagle() failed. errno:" + std::string(strerror(errno)));
            }

            if (!socket_cfg.is_listening_) {
                ASSERT(connect(socket_fd, rp->ai_addr, rp->ai_addrlen) != 1, "connect() failed. errno:" + std::string(strerror(errno)));
            }

            if (socket_cfg.is_listening_) {
                ASSERT(setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == 0, "setsockopt() SO_REUSEADDR failed. errno:" + std::string(strerror(errno)));
            }

            if (socket_cfg.is_listening_) {
                const sockaddr_in addr {
                    AF_INET,
                    htons(socket_cfg.port_),
                    { htonl(INADDR_ANY) },
                    {}
                };
                ASSERT(bind(socket_fd, socket_cfg.is_udp_ ? reinterpret_cast<const sockaddr *>(&addr) : rp -> ai_addr, sizeof(addr)) == 0, "bind() failed. errno:%" + std::string(strerror(errno)));
            }

            if (!socket_cfg.is_udp_ && socket_cfg.is_listening_) {
                ASSERT(listen(socket_fd, MaxTCPServerBacklog) == 0, "listen() failed. errno:" + std::string(strerror(errno)));
            }

            if (socket_cfg.needs_so_timestamp_) {
                ASSERT(setSOTimestamp(socket_fd), "setSOTimestamp() failed. errno:" + std::string(strerror(errno)));
            }
        }
        return socket_fd;
    }
}

#endif //TRADINGECOSYSTEM_SOCKET_UTILS_H