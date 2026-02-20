cmake_minimum_required(VERSION 3.16)
project(TradingEcosystem)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

file(GLOB_RECURSE PROJECT_SOURCES
        CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/src/*.cpp
)

add_executable(TradingEcosystem
        ${PROJECT_SOURCES}
)

target_include_directories(TradingEcosystem
        PRIVATE
        ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(TradingEcosystem
        PRIVATE
        Threads::Threads
)
