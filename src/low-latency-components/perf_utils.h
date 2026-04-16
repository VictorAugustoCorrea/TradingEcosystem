#pragma once

#ifndef TRADINGECOSYSTEM_PERF_UTILS_H
#define TRADINGECOSYSTEM_PERF_UTILS_H
#include <cstdint>

namespace Common {
    inline auto rdtsc() noexcept {
        unsigned int lo, hi;
        __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
        return static_cast<uint64_t>(hi) << 32 | lo;
    }
}

#define START_MEASURE(TAG) const auto TAG = Common::rdtsc()
#define END_MEASURE(TAG, LOGGER) \
    do{\
        const auto end = Common::rdtsc(); \
        LOGGER.log("% RDTSC "#TAG" %.\n", \
        Common::getCurrentTimeStr((&time_str_)), \
        TAG); \
    } while (false)

#define TTT_MEASURE(TAG, LOGGER) \
    do {\
        const auto TAG = Common::getCurrentNanos(); \
        LOGGER.log("% TTT "#TAG" %.\n", \
        Common::getCurrentTimeStr((&time_str_)), \
        TAG); \
    } while (false)

#endif //TRADINGECOSYSTEM_PERF_UTILS_H
