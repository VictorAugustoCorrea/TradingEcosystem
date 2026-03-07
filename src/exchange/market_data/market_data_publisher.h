#pragma once

#ifndef TRADINGECOSYSTEM_MARKET_DATA_PUBLISHER_H
#define TRADINGECOSYSTEM_MARKET_DATA_PUBLISHER_H

#include <functional>

#include "market_update.h"
#include "low-latency-components/logging.h"
#include "low-latency-components/mcast_socket.h"
#include "exchange/market_data/snapshot_synthesizer.h"

namespace Exchange {
    class MarketDataPublisher {
    private:
        size_t next_inc_seq_num_ = 1;
        MEMarketUpdateLFQueue *outgoing_md_updates_ = nullptr;
        MDPMarketUpdateLFQueue snapshot_md_updates_;
        volatile bool run_ = false;
        std::string time_str_;
        Logger logger_;
        McastSocket incremental_socket_;
        SnapshotSynthesizer *snapshot_synthesizer_ = nullptr;
    };
}

#endif //TRADINGECOSYSTEM_MARKET_DATA_PUBLISHER_H