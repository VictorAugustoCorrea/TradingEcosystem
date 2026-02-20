#pragma once

#ifndef TRADINGECOSYSTEM_MACROS_H
#define TRADINGECOSYSTEM_MACROS_H

#define LIKELY  (x) __builtin_expect(!!(x), 1)
#define UNLIKELY(x) __builtin_expect(!!(x), 0)

#include <iostream>

inline auto ASSERT(const bool cond, const std::string& msg) noexcept{
    if (UNLIKELY(!cond))
    {
        std::cerr << msg << std::endl;
        exit(EXIT_FAILURE);
    }
}
inline auto FATAL(const std::string& msg) noexcept
{
    std::cerr << msg << std::endl;
    exit(EXIT_FAILURE);
}
#endif //TRADINGECOSYSTEM_MACROS_H