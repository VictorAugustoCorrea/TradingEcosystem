#include "me_order.h"

namespace Exchange {
    auto MEOrder::toString() const -> std::string {
        std::stringstream ss;
        ss  << " MEOrder "
            << " [ "
            << " Ticker: " << tickerIdToString( ticker_id_ )
            << " Client_order_id: " << clientIdToString( client_id_ )
            << " Order_Id: " << orderIdToString( client_order_id_ )
            << " Market_order_id: " << orderIdToString( market_order_id_ )
            << " Side: " << sideToString( side_ )
            << " Price: " << priceToString( price_ )
            << " Qty: " << qtyToString( qty_ )
            << " Priority: " << priorityToString( priority_ )
            << " Prev: " << orderIdToString( prev_order_ ? prev_order_ -> market_order_id_ : OrderId_INVALID)
            << " Next: " << orderIdToString( next_order_ ? next_order_ -> market_order_id_ : OrderId_INVALID)
            << " ] ";
        return ss.str();
    }
}