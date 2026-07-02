/**
MEXC Futures Private WebSocket Stream Manager

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2026 Vitezslav Kot <vitezslav.kot@stonky.cz>, Stonky s.r.o.
*/

#include "stonky/mexc/mexc_ws_private_stream_manager.h"
#include "stonky/mexc/mexc_futures_ws_client.h"
#include <fmt/format.h>

namespace stonky::mexc::futures {
static auto ORDER_CHANNEL = "push.personal.order";
static auto DEAL_CHANNEL = "push.personal.order.deal";

struct WSPrivateStreamManager::P {
    std::unique_ptr<WSClient> wsClient;
    onLogMessage logMessageCB;
    onOrderUpdate orderUpdateCB;
    onDeal dealCB;

    explicit P() : wsClient(std::make_unique<WSClient>()) {
        wsClient->setDataEventCallback([&](const Event &event) {
            if (event.channel == ORDER_CHANNEL) {
                dispatch<EventOrder>(event, orderUpdateCB);
            } else if (event.channel == DEAL_CHANNEL) {
                dispatch<EventDeal>(event, dealCB);
            }
        });
    }

    /// Personal pushes carry the payload in Event.data, either as a single
    /// object or (defensively) an array of them.
    template <typename EventType, typename Callback>
    void dispatch(const Event &event, const Callback &cb) const {
        if (!cb) {
            return;
        }

        const auto handleOne = [&](const nlohmann::json &item) {
            try {
                EventType typedEvent;
                typedEvent.fromJson(item);
                cb(typedEvent);
            } catch (std::exception &e) {
                if (logMessageCB) {
                    logMessageCB(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, e.what()));
                }
            }
        };

        if (event.data.is_array()) {
            for (const auto &item: event.data) {
                handleOne(item);
            }
        } else if (event.data.is_object()) {
            handleOne(event.data);
        }
    }
};

WSPrivateStreamManager::WSPrivateStreamManager(const std::string &apiKey, const std::string &apiSecret) : m_p(std::make_unique<P>()) {
    m_p->wsClient->setCredentials(apiKey, apiSecret);
}

WSPrivateStreamManager::~WSPrivateStreamManager() { m_p->wsClient.reset(); }

void WSPrivateStreamManager::setLoggerCallback(const onLogMessage &onLogMessageCB) const {
    m_p->logMessageCB = onLogMessageCB;
    m_p->wsClient->setLoggerCallback(onLogMessageCB);
}

void WSPrivateStreamManager::setOrderUpdateCallback(const onOrderUpdate &onOrderUpdateCB) const { m_p->orderUpdateCB = onOrderUpdateCB; }

void WSPrivateStreamManager::setDealCallback(const onDeal &onDealCB) const { m_p->dealCB = onDealCB; }

void WSPrivateStreamManager::connect() const { m_p->wsClient->connect(); }

bool WSPrivateStreamManager::isAuthenticated() const { return m_p->wsClient->isAuthenticated(); }
} // namespace stonky::mexc::futures
