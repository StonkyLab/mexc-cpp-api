/**
MEXC Futures Private WebSocket Stream Manager

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2026 Vitezslav Kot <vitezslav.kot@stonky.cz>, Stonky s.r.o.
*/

#ifndef INCLUDE_STONKY_MEXC_WS_PRIVATE_STREAM_MANAGER_H
#define INCLUDE_STONKY_MEXC_WS_PRIVATE_STREAM_MANAGER_H

#include "stonky/utils/log_utils.h"
#include "stonky/mexc/mexc_event_models.h"
#include <memory>
#include <string>

namespace stonky::mexc::futures {
using onOrderUpdate = std::function<void(const EventOrder&)>;
using onDeal = std::function<void(const EventDeal&)>;

/**
 * Authenticated stream of the account's private order and fill events
 * (wss://contract.mexc.com/edge). After login MEXC auto-pushes the personal
 * channels — no per-channel subscribe. Auth, heartbeat and reconnect with
 * re-login are handled by the underlying session.
 *
 * Delivery is push-only via callbacks — every element matters (fills, cancels,
 * rejects). Callbacks run on the WebSocket io thread and must be fast and
 * non-blocking (copy into your own queue under your own lock, then return).
 *
 * MEXC specifics the consumer must handle:
 *  - No replay after a disconnect — reconcile via REST when the stream was down.
 *  - The deal channel carries the venue orderId but NOT the externalOid; map
 *    orderId → your client order id yourself (order events carry both).
 *  - A cancelled order (state 4) may still carry a partial dealVol.
 */
class WSPrivateStreamManager {
    struct P;
    std::unique_ptr<P> m_p{};

public:
    WSPrivateStreamManager(const std::string& apiKey, const std::string& apiSecret);

    ~WSPrivateStreamManager();

    /**
     * Set logger callback, if not set then all errors are written to stderr only
     */
    void setLoggerCallback(const onLogMessage& onLogMessageCB) const;

    /**
     * Set the order-update callback (push.personal.order). Invoked on the io
     * thread. Set before connect().
     */
    void setOrderUpdateCallback(const onOrderUpdate& onOrderUpdateCB) const;

    /**
     * Set the fill callback (push.personal.order.deal). Invoked on the io
     * thread. Set before connect().
     */
    void setDealCallback(const onDeal& onDealCB) const;

    /**
     * Connect and log in. Returns immediately; personal events start flowing
     * once login completes. Set the callbacks BEFORE calling connect.
     */
    void connect() const;

    /**
     * True while the CURRENT connection is authenticated (drops during a
     * reconnect, returns after re-login).
     */
    [[nodiscard]] bool isAuthenticated() const;
};
} // namespace stonky::mexc::futures

#endif // INCLUDE_STONKY_MEXC_WS_PRIVATE_STREAM_MANAGER_H
