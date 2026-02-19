cmake_minimum_required(VERSION 4.1)
project(TradingEcosystem)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(TradingEcosystem src/main.cpp
        src/logging.h
        src/lock_free_queue.h
        src/macros.h
        src/mem_pool.h
        src/socket_utils.h
        src/tcp_server.h
        src/tcp_socket.h
        src/thread_utils.h
        src/time_utils.h
        src/types.h)

target_link_libraries(TradingEcosystem PRIVATE Threads::Threads)