#pragma once

#ifndef TRADINGECOSYSTEM_MATCHING_ENGINE_H
#define TRADINGECOSYSTEM_MATCHING_ENGINE_H

#include "me_order_book.h"
#include "low-latency-components/macros.h"
#include "low-latency-components/logging.h"
#include "exchange/market_data/market_update.h"
#include "exchange/order_server/client_request.h"
#include "exchange/order_server/client_response.h"
#include "low-latency-components/lock_free_queue.h"

namespace Exchange {
    class MatchingEngine final {
    public:
        MatchingEngine(
            ClientRequestLFQueue *client_requests,
            MEClientResponseLFQueue *client_responses,
            MEMarketUpdateLFQueue *market_updates
            );
        ~MatchingEngine();

        auto start() -> void;
        auto stop()  -> void;

         auto processClientRequest(const MEClientRequest *client_request) noexcept{
            auto order_book = ticker_order_book_[client_request -> ticker_id_];
            switch (client_request -> type_) {
                case ClientRequestType::NEW: {
                    order_book -> add(
                        client_request -> client_id_,
                        client_request->order_id_,
                        client_request -> ticker_id_,
                        client_request -> side_,
                        client_request -> price_,
                        client_request -> qty_);
                } break;

                case ClientRequestType::CANCEL: {
                    order_book -> cancel(
                        client_request -> client_id_,
                        client_request -> order_id_,
                        client_request -> ticker_id_);
                } break;

                default: {
                    FATAL("Received invalid client-request-type: " +
                        clientRequestTypeToString(client_request -> type_ ));
                } break;
            }
        }

        auto sendClientResponse(const MEClientResponse *client_response) noexcept {
            logger_.log("%:% %() % Sending: %. \n.",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr( &time_str_ ),
                client_response -> toString());
            const auto next_write = outgoing_ogw_responses_ -> getNextToWriteTo();
            *next_write = *client_response;
            outgoing_ogw_responses_ -> updateWriteIndex();
        }

        auto sendMarketUpdate(const MEMarketUpdate *market_update) noexcept {
            logger_.log("%:% %() % Sending: %. \n",
                __FILE__, __LINE__, __func__,
                getCurrentTimeStr( &time_str_ ),
                market_update -> toString());
            const auto next_write = outgoing_md_updates_ -> getNextToWriteTo();
            *next_write = *market_update;
            outgoing_md_updates_ -> updateWriteIndex();
        }

        auto run() noexcept {
            logger_.log("%:% %() %. \n", __FILE__, __LINE__, __func__,
                getCurrentTimeStr( &time_str_ ));

            while ( run_ ) {
                if (const auto me_client_request = incoming_requests_ -> getNextToRead()) {
                    logger_.log("%:% %() % Processing %. \n",
                        __FILE__, __LINE__, __func__,
                        getCurrentTimeStr( &time_str_ ),
                        me_client_request -> toString());

                    processClientRequest(me_client_request);
                    incoming_requests_ -> updateReadIndex();
                }
            }
        }

        MatchingEngine() = delete;
        MatchingEngine(const MatchingEngine & ) = delete;
        MatchingEngine(const MatchingEngine &&) = delete;
        MatchingEngine &operator = (const MatchingEngine & ) = delete;
        MatchingEngine &operator = (const MatchingEngine &&) = delete;

    private:
        OrderBookHashMap ticker_order_book_ = {};
        ClientRequestLFQueue *incoming_requests_ = nullptr;
        MEClientResponseLFQueue *outgoing_ogw_responses_ = nullptr;
        MEMarketUpdateLFQueue *outgoing_md_updates_ = nullptr;
        volatile bool run_ = false;
        std::string time_str_;
        Logger logger_;
    };
}


#endif //TRADINGECOSYSTEM_MATCHING_ENGINE_H