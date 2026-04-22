#pragma once

#ifndef TRADINGECOSYSTEM_MARKET_DATA_PUBLISHER_H
#define TRADINGECOSYSTEM_MARKET_DATA_PUBLISHER_H

#include "market_update.h"
#include "low-latency-components/logging.h"
#include "low-latency-components/mcast_socket.h"
#include "exchange/market_data/snapshot_synthesizer.h"

namespace Exchange {
    class MarketDataPublisher {
    public:
        MarketDataPublisher(
        MEMarketUpdateLFQueue *market_updates,
        const std::string &iface,
        const std::string &snapshot_ip,
        int snapshot_port,
        const std::string &incremental_ip,
        int incremental_port);

        auto run() noexcept -> void;

        auto start() -> void {
            run_ = true;
            thread_ = createAndStartThread(-1, "Exchange/MarketDataPublisher", [this] { run(); });
            ASSERT(thread_ != nullptr && thread_->joinable(),
                "Failed to start MarketData thread.");
            snapshot_synthesizer_ -> start();
        }

        auto stop() -> void {
            run_ = false;
            if (thread_) {
                if (thread_->joinable())
                    thread_->join();
                delete thread_;
                thread_ = nullptr;
            }
            snapshot_synthesizer_ -> stop();
        }

        ~MarketDataPublisher() {
            stop();
            delete snapshot_synthesizer_;
            snapshot_synthesizer_ = nullptr;
        }

        MarketDataPublisher() = delete;
        MarketDataPublisher(const MarketDataPublisher & ) = delete;
        MarketDataPublisher(const MarketDataPublisher &&) = delete;
        MarketDataPublisher &operator=(const MarketDataPublisher & ) = delete;
        MarketDataPublisher &operator=(const MarketDataPublisher &&) = delete;

    private:
        size_t next_inc_seq_num_ = 1;
        MEMarketUpdateLFQueue *outgoing_md_updates_ = nullptr;
        MDPMarketUpdateLFQueue snapshot_md_updates_;
        volatile bool run_ = false;
        std::thread *thread_ = nullptr;
        std::string time_str_;
        Logger logger_;
        McastSocket incremental_socket_;
        SnapshotSynthesizer *snapshot_synthesizer_ = nullptr;
    };
}

#endif //TRADINGECOSYSTEM_MARKET_DATA_PUBLISHER_H
