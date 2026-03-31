#pragma once

#ifndef TRADINGECOSYSTEM_TYPES_H
#define TRADINGECOSYSTEM_TYPES_H

#include <limits>
#include <cstdint>
#include "macros.h"

namespace Common {
    typedef uint64_t OrderId;
    constexpr auto OrderId_INVALID = std::numeric_limits<OrderId>::max();

    inline auto orderIdToString(const OrderId order_id) -> std::string {
        if (UNLIKELY(order_id == OrderId_INVALID)) {
            return "INVALID";
        }
        return std::to_string(order_id);
    }

    typedef uint32_t TickerId;
    constexpr auto TickerId_INVALID = std::numeric_limits<TickerId>::max();

    inline auto tickerIdToString(const TickerId ticker_id) -> std::string {
        if (UNLIKELY(ticker_id == TickerId_INVALID)) {
            return "INVALID";
        }
        return std::to_string(ticker_id);
    }

    typedef uint32_t ClientId;
    constexpr auto ClientId_INVALID = std::numeric_limits<ClientId>::max();

    auto inline clientIdToString(const ClientId client_id) -> std::string {
        if (UNLIKELY(client_id == ClientId_INVALID)) {
            return "INVALID";
        }
        return std::to_string(client_id);
    }

    typedef int64_t Price;
    constexpr auto Price_INVALID = std::numeric_limits<Price>::max();

    auto inline priceToString(const Price price) -> std::string {
        if (UNLIKELY(price == Price_INVALID)) {
            return "INVALID";
        }
        return std::to_string(price);
    }

    typedef uint32_t Qty;
    constexpr auto Qty_INVALID = std:: numeric_limits<Qty>::max();

    auto inline qtyToString(const Qty qty) -> std::string {
        if (UNLIKELY(qty == Qty_INVALID)) {
            return "INVALID";
        }
        return std::to_string(qty);
    }

    typedef uint64_t Priority;
    constexpr auto Priority_INVALID = std::numeric_limits<Priority>::max();

    auto inline priorityToString(const Priority priority) -> std::string {
        if (UNLIKELY(priority == Priority_INVALID)) {
            return "INVALID";
        }
        return std::to_string(priority);
    }

    enum class Side : int8_t {
        INVALID = 0,
        BUY = 1,
        SELL = -1,
        MAX = 2
    };

    inline auto sideToString(const Side side) -> std::string {
        switch (side) {
            case Side::BUY:
                return "BUY";
            case Side::SELL:
                return "SELL";
            case Side::INVALID:
                return "INVALID";
            case Side::MAX:
                return "MAX";
        }
        return "UNKNOWN";
    }

    constexpr auto sideToIndex(Side side) noexcept {
        return static_cast<size_t> (side) + 1;
    }

    constexpr auto sideToValue(Side side) noexcept {
        return static_cast<int> (side);
    }

    struct RiskCfg {
        Qty max_order_size_ = 0;
        Qty max_position_ = 0;
        double max_loss_ = 0;

        [[nodiscard]]
        auto toString() const {
            std::stringstream ss;
            ss  << "RiskCfg { "
                << "Max-order_size: " << qtyToString(max_order_size_)
                << ", Max-position: " << qtyToString(max_position_)
                << ", Max-loss: " << max_loss_
                << ". } ";
            return ss.str();
        }
    };

    struct TradeEngineCfg {
        Qty clip_ = 0;
        double threshold_ = 0;
        RiskCfg risk_cfg_;

        [[nodiscard]]
        auto toString() const {
            std::stringstream ss;
            ss  << "TradeEngineCfg { "
                << "Clip: " << qtyToString(clip_)
                << ", Threshold: " << threshold_
                << ", Risk: " << risk_cfg_.toString()
                << ". } ";
            return ss.str();
        }
    };

    enum class AlgoType : int8_t {
        INVALID = 0,
        RANDOM = 1,
        MAKER = 2,
        TAKER = 3,
        MAX = 4
    };

    inline auto algoTypeToString(const AlgoType type) -> std::string {
        switch (type) {
            case AlgoType::RANDOM:
                return "RANDOM";
            case AlgoType::MAKER:
                return "MAKER";
            case AlgoType::TAKER:
                return "TAKER";
            case AlgoType::INVALID:
                return "INVALID";
            case AlgoType::MAX:
                return "MAX";
        }
        return "UNKNOWN";
    }

    inline auto stringToAlgoType(const std::string &str) -> AlgoType {
        for (auto i = static_cast<int>(AlgoType::INVALID); i <= static_cast<int>(AlgoType::MAX); ++i) {
            if (const auto algo_type = static_cast<AlgoType>(i); algoTypeToString(algo_type) == str)
                return algo_type;
        }
        return AlgoType::INVALID;
    }

    constexpr size_t ME_MAX_TICKERS = 8;
    constexpr size_t ME_MAX_NUM_CLIENTS = 256;
    constexpr size_t ME_MAX_PRICE_LEVELS = 256;
    constexpr size_t ME_MAX_ORDER_IDS = 1024 * 64;
    constexpr size_t ME_MAX_CLIENT_UPDATES = 8192;
    constexpr size_t ME_MAX_MARKET_UPDATES = 8192;

    typedef std::array<TradeEngineCfg, ME_MAX_TICKERS> TradeEngineCfgHashMap;
}
#endif //TRADINGECOSYSTEM_TYPES_H