/**
MEXC Futures WebSocket Client

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#ifndef INCLUDE_VK_MEXC_FUTURES_WS_CLIENT_H
#define INCLUDE_VK_MEXC_FUTURES_WS_CLIENT_H

#include <memory>

#include "vk/utils/log_utils.h"
#include "vk/utils/utils.h"
#include "mexc_futures_ws_session.h"

namespace vk::mexc::futures {
class WSClient : public noncopyable {
    struct P;
    std::unique_ptr<P> m_p{};

public:
    WSClient();

    ~WSClient();

    /**
    * Run the WebSocket IO Context asynchronously and returns immediately without blocking the thread execution
    */
    void run() const;

    /**
     * Set logger callback, if no set then all errors are writen to the stderr stream only
     * @param onLogMessageCB
     */
    void setLoggerCallback(const onLogMessage &onLogMessageCB) const;

    /**
     * Set Data Message callback
     * @param onDataEventCB
     */
    void setDataEventCallback(const onDataEvent &onDataEventCB) const;

    /**
     * Subscribe WebSocket according to the subscriptionRequest
     * @param subscriptionRequest
     */
    void subscribe(const nlohmann::json &subscriptionRequest) const;

    /**
     * Check if a stream is already subscribed
     * @param subscriptionRequest subscription request
     * @return True if subscribed
     */
    [[nodiscard]] bool isSubscribed(const nlohmann::json &subscriptionRequest) const;
};
};

#endif //INCLUDE_VK_MEXC_FUTURES_WS_CLIENT_H
