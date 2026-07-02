/**
MEXC Futures WebSocket Client

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@stonky.cz>, Stonky s.r.o.
*/

#ifndef INCLUDE_STONKY_MEXC_FUTURES_WS_CLIENT_H
#define INCLUDE_STONKY_MEXC_FUTURES_WS_CLIENT_H

#include <memory>

#include "stonky/utils/log_utils.h"
#include "stonky/utils/utils.h"
#include "mexc_futures_ws_session.h"

namespace stonky::mexc::futures {
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
     * Override the stream endpoint. Call before the first subscribe()/connect().
     * Defaults to contract.mexc.com:443, path /edge.
     */
    void setEndpoint(const std::string& host, const std::string& port, const std::string& path) const;

    /**
     * Set API credentials for a private stream. Call before connect(). The
     * session then logs in after every (re)connect.
     */
    void setCredentials(const std::string& apiKey, const std::string& apiSecret) const;

    /**
     * Start a login-only (private) session — no channel subscription; MEXC
     * auto-pushes the personal channels after login. Requires setCredentials.
     */
    void connect() const;

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
     * Unsubscribe a previously subscribed request. No-op when unknown.
     * @param subscriptionRequest
     */
    void unsubscribe(const nlohmann::json &subscriptionRequest) const;

    /**
     * Check if a stream is already subscribed
     * @param subscriptionRequest subscription request
     * @return True if subscribed
     */
    [[nodiscard]] bool isSubscribed(const nlohmann::json &subscriptionRequest) const;

    /**
     * True once the private session authenticated its CURRENT connection.
     * Always false without credentials.
     */
    [[nodiscard]] bool isAuthenticated() const;
};
};

#endif //INCLUDE_STONKY_MEXC_FUTURES_WS_CLIENT_H
