#pragma once

#ifndef TRADINGECOSYSTEM_THREAD_UTILS_H
#define TRADINGECOSYSTEM_THREAD_UTILS_H

#include <iostream>
#include <atomic>
#include <thread>

namespace Common
{
    /** Set affinity for current thread to be pinned to the provided core_id. */
    inline auto setThreadCore(const int core_id) noexcept
    {
        cpu_set_t cpu_set;

        CPU_ZERO(&cpu_set);
        CPU_SET(core_id, &cpu_set);

        return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set) == 0;
    }

    /**
     * Creates a thread instance, sets affinity on it, assigns it a name and;
     * passes the function to be run on that thread as well as the arguments to the function.
    */

    template <typename T, typename... A>
    auto createAndStartThread(const int core_id, const std::string &name, T &&func, A &&... args) noexcept
    {
        auto t = new std::thread([&] {
        if (core_id >= 0 && !setThreadCore(core_id)) {
            std::cerr << "Failed to set core affinity for " << name << " " << pthread_self() << " to " << core_id << std::endl;
            exit(EXIT_FAILURE);
         }
        std::cerr << "Set core affinity for " << name << " " << pthread_self() << " to " << core_id << std::endl;

        std::forward<T>(func)((std::forward<A>(args))...);
        });

        using namespace std::literals::chrono_literals;
        std::this_thread::sleep_for(1s);

        return t;
    }
}

#endif //TRADINGECOSYSTEM_THREAD_UTILS_H