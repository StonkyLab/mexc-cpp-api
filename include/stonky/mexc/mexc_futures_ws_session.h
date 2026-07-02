/**
MEXC Futures WebSocket Session

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@stonky.cz>, Stonky s.r.o.
*/

#ifndef INCLUDE_STONKY_MEXC_WS_SESSION_V5_H
#define INCLUDE_STONKY_MEXC_WS_SESSION_V5_H

#include "stonky/utils/log_utils.h"
#include "stonky/mexc/mexc_event_models.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ssl/context.hpp>
#include <memory>

namespace stonky::mexc::futures {
using onDataEvent = std::function<void(const Event& event)>;

/**
 * One TLS WebSocket connection to the MEXC contract stream (wss://contract.mexc.com/edge).
 *
 * Outbound messages (login, subscribe, JSON pings) go through an internal write
 * pump, so they are sent even on a quiet stream with no inbound traffic.
 * On any transport error, ping expiry or auth failure the socket is torn down
 * and reconnected with exponential backoff (1 s → 30 s); credentialed sessions
 * re-login and all subscriptions are replayed. Only close() stops the loop.
 */
class WebSocketSession final : public std::enable_shared_from_this<WebSocketSession> {
    struct P;
    std::unique_ptr<P> m_p;

public:
    explicit WebSocketSession(boost::asio::io_context& ioc, boost::asio::ssl::context& ctx,
                              const onLogMessage& onLogMessageCB);

    ~WebSocketSession();

    /**
     * Set API credentials BEFORE run(). When set, the session sends a `login`
     * op right after the WS handshake (HMAC-SHA256 of apiKey+reqTime) and holds
     * all subscriptions back until the login ack. After login MEXC auto-pushes
     * the personal channels (order/deal/position) — no per-channel subscribe.
     */
    void setCredentials(const std::string& apiKey, const std::string& apiSecret) const;

    /**
     * Run the session.
     * @param host e.g. contract.mexc.com
     * @param port e.g. 443
     * @param path WebSocket upgrade path, e.g. /edge
     * @param subscriptionRequest first subscription; pass an empty/null json for
     *        a login-only (private) stream that relies on auto-push
     * @param dataEventCB Data Message callback — invoked on the io thread
     */
    void run(const std::string& host, const std::string& port, const std::string& path,
             const nlohmann::json& subscriptionRequest, const onDataEvent& dataEventCB);

    /**
     * Close the session asynchronously and disable automatic reconnect.
     */
    void close() const;

    /**
     * Subscribe according to the request. Safe from any thread; queued until the
     * connection is up (and authenticated, when credentials are set).
     * @param subscriptionRequest e.g. {"method":"sub.ticker","param":{"symbol":"BTC_USDT"}}
     */
    void subscribe(const nlohmann::json& subscriptionRequest) const;

    /**
     * Unsubscribe a previously subscribed request (the "sub.*" method is sent to
     * the venue as its "unsub.*" form). No-op when not subscribed.
     * @param subscriptionRequest the same request that was passed to subscribe()
     */
    void unsubscribe(const nlohmann::json& subscriptionRequest) const;

    /**
     * True once the login ack of the CURRENT connection was accepted. Always
     * false for credential-less sessions and while reconnecting.
     */
    [[nodiscard]] bool isAuthenticated() const;

    /**
     * Check if a request is already subscribed (sent or pending).
     * @param subscriptionRequest
     * @return True if subscribed
     */
    [[nodiscard]] bool isSubscribed(const nlohmann::json& subscriptionRequest) const;
};
}
#endif //INCLUDE_STONKY_MEXC_WS_SESSION_V5_H
