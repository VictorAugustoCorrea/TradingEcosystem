# Trading Ecosystem

A low-latency trading system prototype in C++, focused on deterministic processing, high message throughput, and modular architecture.

## Overview

The project brings together, in a single executable (`TradingEcosystem`), the following components:

- Exchange (ingestion, matching, and market data)
- Trading client (gateway, market data consumer, and strategies)
- Low-latency infrastructure (lock-free queues, networking, logging, and timing utilities)

## Current Project Structure

```text
TradingEcosystem/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp
│   ├── exchange/
│   │   ├── exchange_application.{h,cpp}
│   │   ├── order_server/
│   │   │   ├── order_server.{h,cpp}
│   │   │   ├── client_request.h
│   │   │   ├── client_response.h
│   │   │   └── fifo_sequencer.h
│   │   ├── matcher/
│   │   │   ├── matching_engine.{h,cpp}
│   │   │   ├── me_order.{h,cpp}
│   │   │   └── me_order_book.{h,cpp}
│   │   └── market_data/
│   │       ├── market_update.h
│   │       ├── market_data_publisher.{h,cpp}
│   │       └── snapshot_synthesizer.{h,cpp}
│   ├── trading/
│   │   ├── trade_client_application.{h,cpp}
│   │   ├── order_gw/
│   │   │   └── order_gateway.{h,cpp}
│   │   ├── market_data/
│   │   │   └── market_data_consumer.{h,cpp}
│   │   └── strategy/
│   │       ├── trade_engine.{h,cpp}
│   │       ├── order_manager.{h,cpp}
│   │       ├── risk_manager.{h,cpp}
│   │       ├── market_maker.{h,cpp}
│   │       ├── liquidity_taker.{h,cpp}
│   │       ├── market_order_book.{h,cpp}
│   │       ├── feature_engine.h
│   │       ├── market_order.h
│   │       ├── om_order.h
│   │       └── position_keeper.h
│   ├── low-latency-components/
│   │   ├── lock_free_queue.h
│   │   ├── mem_pool.h
│   │   ├── tcp_server.h
│   │   ├── tcp_socket.h
│   │   ├── socket_utils.h
│   │   ├── mcast_socket.h
│   │   ├── logging.h
│   │   ├── perf_utils.h
│   │   ├── thread_utils.h
│   │   ├── time_utils.h
│   │   ├── macros.h
│   │   └── types.h
│   └── scripts/
│       ├── build.sh
│       ├── no_clean_build.sh
│       ├── run_exchange_and_clients.sh
│       └── run_clients.sh
├── build/
└── cmake-build-*/
```

Note: log files and build artifacts are generated in `build/`, `cmake-build-*`, and also under `src/scripts/` when scripts are executed.

## Core Components

- `order_server`: accepts client requests and sequences messages.
- `matcher`: maintains the order book and performs price-time-priority matching.
- `market_data`: publishes updates and snapshots derived from order book events.
- `trading`: runs the client, market data consumption, order gateway, and strategies (`MAKER`, `TAKER`, `RANDOM`).
- `low-latency-components`: reusable building blocks for communication and performance.

## Requirements

- Linux
- CMake >= 3.16
- C++ compiler with C++20 support (GCC/Clang)
- `ninja` (used by scripts in `src/scripts`)

## Build

### Option 1: Manual CMake

```bash
mkdir -p build
cmake -S . -B build
cmake --build build -j"$(nproc)"
```

### Option 2: Project Script (Debug + Release)

```bash
cd src/scripts
./build.sh
```

This script generates binaries at:

- `src/cmake-build-release/TradingEcosystem`
- `src/cmake-build-debug/TradingEcosystem`

## Running

With the `TradingEcosystem` binary, the modes supported by `src/main.cpp` are:

```text
--exchange-only
--exchange <client_id> <algo_type> [ticker params...]
<client_id> <algo_type> [ticker params...]
```

Examples:

```bash
# Exchange only
./TradingEcosystem --exchange-only

# Exchange + client in the same process
./TradingEcosystem --exchange 1 MAKER

# Client only
./TradingEcosystem 2 TAKER
```

To run the full automated scenario:

```bash
cd src/scripts
./run_exchange_and_clients.sh
```

## Purpose

Educational project for studying electronic exchange architecture and low-latency system engineering techniques. It is not production-ready.
