#pragma once

#ifndef TRADINGECOSYSTEM_POSITION_KEEPER_H
#define TRADINGECOSYSTEM_POSITION_KEEPER_H

#include "market_order_book.h"
#include "low-latency-components/types.h"
#include "low-latency-components/macros.h"
#include "low-latency-components/logging.h"
#include "exchange/order_server/client_response.h"

using namespace Common;
namespace Trading {
    struct PositionInfo {
        int32_t position_ = 0;
        double real_pnl = 0, unreal_pnl = 0, total_pnl = 0;
        std::array<double, sideToIndex(Side::MAX) + 1> open_vwap;
        Qty volume_ = 0;
        const BBO *bbo_ = nullptr;
    };

    inline auto toString() {
        std::stringstream ss;
        ss  << "Position:"
            << " { "
            << "Position: " << position_
            << "u-pnl: " << unreal_pnl_
            << "r-pnl: " << real_pnl_
            << "t-pnl: " << total_pnl_
            << "vol: " << volume_
            << "v-waps: [" << (position_ ? open_vwap_.at(sideToIndex(Side::BUY))/ std::abs(position_) : 0) << " ] "
            << " X "
            << (postion_ ? open_vwap_.at(sideToIndex(Side::SELL))/ std::abs(postion_) : 0) << " ] "
            << (bbo_ ? bbo_ -> toString() : " ") << " } ";
        return ss.str();
    }

    auto addFill(const Exchange::MEClientResponse *client_response, Logger *logger) noexcept {
        const auto old_position = position_;
        const auto side_index = sideToIndex(client_response -> side_);
        const auto opp_side_index = sideToIndex(client_response -> side_ == Side::BUY ? Side::SELL : Side::BUY);
        const auto side_value = sideToValue(client_response -> side_);
        position_ += client_response -> exec_qty_ * side_value;
        volume_ += client_response -> exec_qty_;

        if (old_position * sideToValue(client_response -> side_) >= 0) {
            open_vwap_[side_index] += client_response -> price_ * client_response -> exec_qty_;
        }
        else {
            const auto opp_side_vwap = open_vwap_[opp_side_index] = opp_side_vwap * std::abs(position_);
            real_pnl += std::min(static_cast<int32_t> (client_response -> exec_qty_), std::abs(old_position))
                        * (opp_side_vwap - client_response -> price_) * sideToValue(client_response -> side_);
            if (position_ * old_position < 0) {
                open_vwap_[side_index] = client_response -> price_ * std::abs(position_);
                open_vwap_[opp_side_index] = 0;
            }
        }
        if (!position_) {
            open_vwap_[sideToIndex(Side::BUY)] = open_vwap_[sideToIndex(Side::SELL)] = 0;
            unreal_pnl_ = 0;
        }
        else {
            if (position_ > 0)
                unreal_pnl_ = (client_response -> price_ - open_vwap_[sideToIndex(Side::BUY)] / std::abs(position_)) * std::abs(position_);
            else
                unreal_pnl_ = (open_vwap_[sidetoIndex(Side::SELL)] / std::abs(position_) - client_response -> price_) * std::abs(position_);
        }
        total_pnl = unreal_pnl_ + real_pnl_;
        std::string time_str;
        logger -> log("%:% %() % % %.\n",
            __FILE__, __LINE__, __func__,
            getCurrentTimeStr(&time_str),
            toString(),
            client_response -> toString().c_str());
    }
}

#endif //TRADINGECOSYSTEM_POSITION_KEEPER_H
