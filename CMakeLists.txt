cmake_minimum_required(VERSION 3.16)
project(TradingEcosystem)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# --------------------------
# Unified Executable
# CMakeLists.txt vive em TradingEcosystem/ (raiz).
# Fontes ficam em TradingEcosystem/src/.
# --------------------------
file(GLOB_RECURSE EXCHANGE_SOURCES
        ${PROJECT_SOURCE_DIR}/src/exchange/*.cpp
)

file(GLOB_RECURSE TRADING_SOURCES
        ${PROJECT_SOURCE_DIR}/src/trading/*.cpp
)

add_executable(TradingEcosystem
        ${PROJECT_SOURCE_DIR}/src/main.cpp
        ${EXCHANGE_SOURCES}
        ${TRADING_SOURCES}
)

target_include_directories(TradingEcosystem PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(TradingEcosystem PRIVATE
        Threads::Threads
)