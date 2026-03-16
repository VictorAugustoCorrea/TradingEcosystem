#pragma once

#ifndef TRADINGECOSYSTEM_MARKET_DATA_CONSUMER_H
#define TRADINGECOSYSTEM_MARKET_DATA_CONSUMER_H

#include <map>
#include <functional>
#include "low-latency-components/macros.h"
#include "exchange/market_data/market_update.h"
#include "low-latency-components/thread_utils.h"
#include "low-latency-components/mcast_socket.h"
#include "low-latency-components/lock_free_queue.h"

namespace Trading {
    class MarketDataConsumer {
    public:
    private:
        size_t next_exp_inc_seq_num_ = 1;
        Exchange::MDPMarketUpdateLFQueue *incoming_md_updates_ = nullptr;
        volatile bool run_ = false;
        std::string time_str_;
        Logger logger_;
        McastSocket incremental_mcast_socket_;
        bool in_recovery_ = false;
        const std::string iface_, snapshot_ip_;
        const int snapshot_port_;
        typedef std::map<size_t, Exchange::MEMarketUpdate> QueuedMarketUpdates;
        QueuedMarketUpdates snapshot_queued_msgs_, incremental_queued_msgs_;
    };
}

#endif //TRADINGECOSYSTEM_MARKET_DATA_CONSUMER_H