#pragma once

#ifndef TRADINGECOSYSTEM_SNAPSHOT_SYNTHESIZER_H
#define TRADINGECOSYSTEM_SNAPSHOT_SYNTHESIZER_H

#include "market_update.h"
#include "low-latency-components/types.h"
#include "low-latency-components/logging.h"
#include "low-latency-components/mem_pool.h"
#include "low-latency-components/mcast_socket.h"
#include "low-latency-components/lock_free_queue.h"

using namespace Common;

namespace Exchange {
    class SnapshotSynthesizer {
    public:
        SnapshotSynthesizer(MDPMarketUpdateLFQueue *market_updates, const std::string &iface, const std::string &snapshot_ip, int snapshot_port);
        auto start() -> void;
        auto stop()  -> void;
        ~SnapshotSynthesizer() { stop(); }
        auto addToSnapshot(const MDPMarketUpdate *market_update);
        auto publishSnapshot();
        auto run();

        SnapshotSynthesizer() = delete;
        SnapshotSynthesizer(const SnapshotSynthesizer & ) = delete;
        SnapshotSynthesizer(const SnapshotSynthesizer &&) = delete;
        SnapshotSynthesizer &operator=(const SnapshotSynthesizer & ) = delete;
        SnapshotSynthesizer &operator=(const SnapshotSynthesizer &&) = delete;

    private:
        Logger logger_;
        std::string time_str_;
        volatile bool run_ = false;
        std::thread *thread_ = nullptr;
        McastSocket snapshot_socket_;
        size_t last_inc_seq_num_ = 0;
        Nanos last_snapshot_time_ = 0;
        MemPool<MEMarketUpdate> order_pool_;
        MDPMarketUpdateLFQueue *snapshot_md_updates_ = nullptr;
        std::array<std::array<MEMarketUpdate *, ME_MAX_ORDER_IDS>, ME_MAX_TICKERS> ticker_orders_ = {};
    };
}

#endif //TRADINGECOSYSTEM_SNAPSHOT_SYNTHESIZER_H
