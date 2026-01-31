/**
MEXC Futures WebSocket Session

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#ifndef INCLUDE_VK_MEXC_WS_SESSION_V5_H
#define INCLUDE_VK_MEXC_WS_SESSION_V5_H

#include "vk/utils/log_utils.h"
#include "vk/mexc/mexc_event_models.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <memory>

namespace vk::mexc::futures {
using onDataEvent = std::function<void(const Event& event)>;

class WebSocketSession final : public std::enable_shared_from_this<WebSocketSession> {
    struct P;
    std::unique_ptr<P> m_p;

public:
    explicit WebSocketSession(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx,
                              const onLogMessage& onLogMessageCB);

    ~WebSocketSession();

    /**
     * Run the session.
     * @param host
     * @param port
     * @param subscriptionRequest
     * @param dataEventCB Data Message callback
     */
    void run(const std::string& host, const std::string& port, const nlohmann::json &subscriptionRequest,
             const onDataEvent& dataEventCB);

    /**
     * Close the session asynchronously
     */
    void close() const;

    /**
     * Subscribe WebSocket according to the subscriptionFilter
     * @param subscriptionRequest
     * @see https://mexcdevelop.github.io/apidocs/contract_v1_en/#public-channels
     */
    void subscribe(const nlohmann::json &subscriptionRequest) const;

    /**
     * Check if a stream is already subscribed
     * @param subscriptionRequest
     * @return True if subscribed
     */
    [[nodiscard]] bool isSubscribed(const nlohmann::json &subscriptionRequest) const;
};
}
#endif //INCLUDE_VK_MEXC_WS_SESSION_V5_H
