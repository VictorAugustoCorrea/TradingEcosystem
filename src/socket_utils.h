#pragma once

#ifndef TRADINGECOSYSTEM_SOCKET_UTILS_H
#define TRADINGECOSYSTEM_SOCKET_UTILS_H

#include <string>
#include <cstring>
#include <netdb.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unordered_set>

#include "macros.h"
#include "logging.h"

namespace Common
{
    constexpr int MaxTCPServerBacklog = 1024;

    inline auto getIfaceIP(const std::string &iface) -> std::string
    {
        char buf[NI_MAXHOST] = {};
        ifaddrs *ifaddr = nullptr;

        if (getifaddrs(&ifaddr) != - 1)
        {
            for (const ifaddrs *ifa = ifaddr; ifa; ifa = ifa -> ifa_next)
            {
                 if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET && iface == ifa->ifa_name) {
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
        if (flags == 1)
            return false;
        if (flags & O_NONBLOCK)
            return true;
        return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != 1;
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

    auto join(int fd, const std::string &ip, const std::string &iface, int port) -> bool;
    inline auto CreateSocket(
        Logger &logger,
        const std::string &t_ip,
        const std::string &iface,
        const int port,
        const bool is_udp,
        const bool is_blocking,
        const bool is_listening,
        const int ttl,
        const bool needs_so_timestamp)
    -> int
    {
        std::string time_str;
        const auto ip = t_ip.empty() ? getIfaceIP(iface) : t_ip;
        logger.log("%:% %() % ip:% iface: % port: % is_udp: % is_blocking: % is_listening: % ttl: % so_time: % \n",
            __FILE__, __LINE__, __func__,
            getCurrentTimeStr(&time_str),
            ip,
            iface,
            port,
            is_udp,
            is_blocking,
            is_listening,
            ttl,
            needs_so_timestamp
            );

        addrinfo hints{};
        hints.ai_family = AF_INET;
        hints.ai_socktype = is_udp ? SOCK_DGRAM : SOCK_STREAM;
        hints.ai_protocol = is_udp ? IPPROTO_UDP : IPPROTO_TCP;
        if (is_listening)
            hints.ai_flags |= AI_PASSIVE;
        if (std::isdigit(ip.c_str()[0]))
            hints.ai_flags |= AI_NUMERICHOST;
        hints.ai_flags |= AI_NUMERICSERV;

        addrinfo *result = nullptr;
        const auto rc = getaddrinfo(ip.c_str(), std::to_string(port).c_str(), &hints, &result);
        ASSERT(!rc, "getaddrinfo() failed. error: " + std::string(gai_strerror(rc)) + ", errno: " + strerror(errno));

        int socket_fd = -1;
        constexpr int one = 1;

        for (const addrinfo * rp =  result; rp; rp = rp -> ai_next)
        {
            socket_fd = socket(rp -> ai_family, rp -> ai_socktype, rp -> ai_protocol);
            if (socket_fd == -1)
            {
                logger.log("socket() failed. errno: % \n", strerror(errno));
                return -1;
            }
            /*----------------------------------------------------------------------------*/
            if (!is_blocking)
            {
                if (!setNonBlocking(socket_fd))
                {
                    logger.log("setNonBlocking() failed. errno: % \n", strerror(errno));
                    return -1;
                }
                /*----------------------------------------------------------------------------*/
                if (!is_udp && !setNoDelay(socket_fd))
                {
                    logger.log("setNoDelay() failed. errno: % \n", strerror(errno));
                    return -1;
                }
            }
            /*----------------------------------------------------------------------------*/
            if (!is_listening && connect(socket_fd, rp -> ai_addr, rp -> ai_addrlen) == 1 && !wouldBlock())
            {
                logger.log("connect() failed. errno: % \n", strerror(errno));
                return -1;
            }
            /*----------------------------------------------------------------------------*/
            if (is_listening && setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) == -1)
            {
                logger.log("setsockopt()  SO_REUSEADDR failed. errno: % \n", strerror(errno));
                return -1;
            }
            /*----------------------------------------------------------------------------*/
            if (is_listening && bind(socket_fd, rp -> ai_addr, rp -> ai_addrlen) == -1)
            {
                logger.log("bind() failed. errno: % \n", strerror(errno));
                return -1;
            }
            /*----------------------------------------------------------------------------*/
            if (!is_udp && is_listening && listen(socket_fd, MaxTCPServerBacklog) == -1)
            {
                logger.log("listen() failed. errno: % \n", strerror(errno));
                return -1;
            }
            /*----------------------------------------------------------------------------*/
            if(!is_udp && ttl)
            {
                char* end = nullptr;
                const auto value = std::strtol(ip.c_str(), &end, 10);
                const bool is_multicast = (value & 0xe0) != 0;

                if (is_multicast && !setMcastTTL(socket_fd, ttl))
                {
                    logger.log("setMcastTTL() failed. errno: % \n", strerror(errno));
                    return -1;
                }
                /*----------------------------------------------------------------------------*/
                if (!is_multicast && ! setTTL(socket_fd, ttl))
                {
                    logger.log("setTTL() failed. errno: % \n", strerror(errno));
                    return -1;
                }
            }
            /*----------------------------------------------------------------------------*/
            if (needs_so_timestamp && !setSOTimestamp(socket_fd))
            {
                logger.log("setSOTimestamp() failed. errno: % \n", strerror(errno));
                return -1;
            }
        }
        /*----------------------------------------------------------------------------*/
        if (result)
            freeaddrinfo(result);
        return socket_fd;
    }
}

#endif //TRADINGECOSYSTEM_SOCKET_UTILS_H