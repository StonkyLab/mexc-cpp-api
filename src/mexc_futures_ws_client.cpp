/**
MEXC Futures WebSocket Client

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include "vk/mexc/mexc_futures_ws_client.h"

#include <boost/asio/ssl/context.hpp>
#include <boost/beast/core.hpp>
#include <thread>

namespace vk::mexc::futures {
static auto MEXC_FUTURES_WS_HOST = "contract.mexc.com";
static auto MEXC_FUTURES_WS_PORT = "443";

struct WSClient::P {
    boost::asio::io_context m_ioContext;
    boost::asio::ssl::context m_ctx;
    std::string m_host = {MEXC_FUTURES_WS_HOST};
    std::string m_port = {MEXC_FUTURES_WS_PORT};
    std::weak_ptr<WebSocketSession> m_session;
    std::thread m_ioThread;
    std::atomic<bool> m_isRunning = false;
    onLogMessage m_logMessageCB;
    onDataEvent m_dataEventCB;

    P() : m_ctx(boost::asio::ssl::context::sslv23_client), m_logMessageCB(defaultLogFunction) {
    }
};

WSClient::WSClient() : m_p(std::make_unique<P>()) {
    m_p->m_logMessageCB(LogSeverity::Info, "WSClient created");
}

WSClient::~WSClient() {
    m_p->m_ioContext.stop();

    if (m_p->m_ioThread.joinable()) {
        m_p->m_ioThread.join();
    }

    m_p->m_logMessageCB(LogSeverity::Info, "WSClient destroyed");
}

void WSClient::run() const {
    if (m_p->m_isRunning) {
        return;
    }

    m_p->m_isRunning = true;

    if (m_p->m_ioThread.joinable()) {
        m_p->m_ioThread.join();
    }

    m_p->m_ioThread = std::thread([&] {
        for (;;) {
            try {
                m_p->m_isRunning = true;

                if (m_p->m_ioContext.stopped()) {
                    m_p->m_ioContext.restart();
                }
                m_p->m_ioContext.run();
                m_p->m_isRunning = false;
                break;
            } catch (std::exception &e) {
                if (m_p->m_logMessageCB) {
                    m_p->m_logMessageCB(LogSeverity::Error, fmt::format("{}: {}\n", MAKE_FILELINE, e.what()));
                }
            }
        }

        m_p->m_isRunning = false;
    });
}


void WSClient::setLoggerCallback(const onLogMessage &onLogMessageCB) const {
    m_p->m_logMessageCB = onLogMessageCB;
}

// void WSClient::setDataEventCallback(const onDataEvent &onDataEventCB) const {
//     m_p->m_dataEventCB = onDataEventCB;
// }

void WSClient::subscribe(const std::string &subscriptionRequest) const {
    if (const auto session = m_p->m_session.lock()) {
        session->subscribe(subscriptionRequest);
        return;
    }

    const auto ws = std::make_shared<WebSocketSession>(m_p->m_ioContext, m_p->m_ctx, m_p->m_logMessageCB);
    std::weak_ptr wp{ws};
    m_p->m_session = std::move(wp);
    ws->run(MEXC_FUTURES_WS_HOST, MEXC_FUTURES_WS_PORT, subscriptionRequest, m_p->m_dataEventCB);
}

bool WSClient::isSubscribed(const std::string &subscriptionRequest) const {
    if (const auto session = m_p->m_session.lock()) {
        return session->isSubscribed(subscriptionRequest);
    }

    return false;
}
};
