/**
MEXC Futures WebSocket Client

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@stonky.cz>, Stonky s.r.o.
*/

#include "stonky/mexc/mexc_futures_ws_client.h"

#include <boost/asio/ssl/context.hpp>
#include <boost/beast/core.hpp>
#include <mutex>
#include <thread>

namespace stonky::mexc::futures {
static auto MEXC_FUTURES_WS_HOST = "contract.mexc.com";
static auto MEXC_FUTURES_WS_PORT = "443";
static auto MEXC_FUTURES_WS_PATH = "/edge";

struct WSClient::P {
    /// Guards the lazy session/io-thread creation — subscribe()/run() are hit
    /// concurrently by per-leg worker threads on the first rebalance; without
    /// this, two racing threads could each create a session (one orphaned but
    /// connected) or double-assign a joinable std::thread (std::terminate).
    std::recursive_mutex m_clientLocker;
    boost::asio::io_context m_ioContext;
    boost::asio::ssl::context m_ctx;
    std::string m_host = {MEXC_FUTURES_WS_HOST};
    std::string m_port = {MEXC_FUTURES_WS_PORT};
    std::string m_path = {MEXC_FUTURES_WS_PATH};
    std::string m_apiKey;
    std::string m_apiSecret;
    /// Keeps the session alive across reconnect cycles.
    std::shared_ptr<WebSocketSession> m_session;
    std::thread m_ioThread;
    std::atomic<bool> m_isRunning = false;
    onLogMessage m_logMessageCB;
    onDataEvent m_dataEventCB;

    P() : m_ctx(boost::asio::ssl::context::sslv23_client), m_logMessageCB(defaultLogFunction) {
    }

    std::shared_ptr<WebSocketSession> ensureSession() {
        if (m_session) {
            return m_session;
        }

        m_session = std::make_shared<WebSocketSession>(m_ioContext, m_ctx, m_logMessageCB);

        if (!m_apiKey.empty() && !m_apiSecret.empty()) {
            m_session->setCredentials(m_apiKey, m_apiSecret);
        }

        return m_session;
    }
};

WSClient::WSClient() : m_p(std::make_unique<P>()) {
    m_p->m_logMessageCB(LogSeverity::Info, "WSClient created");
}

WSClient::~WSClient() {
    if (m_p->m_session) {
        m_p->m_session->close();
    }

    m_p->m_ioContext.stop();

    if (m_p->m_ioThread.joinable()) {
        m_p->m_ioThread.join();
    }

    m_p->m_logMessageCB(LogSeverity::Info, "WSClient destroyed");
}

void WSClient::run() const {
    std::lock_guard lk(m_p->m_clientLocker);

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

void WSClient::setEndpoint(const std::string &host, const std::string &port, const std::string &path) const {
    m_p->m_host = host;
    m_p->m_port = port;
    m_p->m_path = path;
}

void WSClient::setCredentials(const std::string &apiKey, const std::string &apiSecret) const {
    m_p->m_apiKey = apiKey;
    m_p->m_apiSecret = apiSecret;
}

void WSClient::connect() const {
    std::lock_guard lk(m_p->m_clientLocker);
    const auto session = m_p->ensureSession();
    session->run(m_p->m_host, m_p->m_port, m_p->m_path, nlohmann::json(), m_p->m_dataEventCB);
    run();
}

void WSClient::setLoggerCallback(const onLogMessage &onLogMessageCB) const {
    m_p->m_logMessageCB = onLogMessageCB;
}

void WSClient::setDataEventCallback(const onDataEvent &onDataEventCB) const {
    m_p->m_dataEventCB = onDataEventCB;
}

void WSClient::subscribe(const nlohmann::json &subscriptionRequest) const {
    std::lock_guard lk(m_p->m_clientLocker);
    const bool fresh = !m_p->m_session;
    const auto session = m_p->ensureSession();

    if (fresh) {
        session->run(m_p->m_host, m_p->m_port, m_p->m_path, subscriptionRequest, m_p->m_dataEventCB);
    } else {
        session->subscribe(subscriptionRequest);
    }

    run();
}

void WSClient::unsubscribe(const nlohmann::json &subscriptionRequest) const {
    std::lock_guard lk(m_p->m_clientLocker);

    if (m_p->m_session) {
        m_p->m_session->unsubscribe(subscriptionRequest);
    }
}

bool WSClient::isSubscribed(const nlohmann::json &subscriptionRequest) const {
    std::lock_guard lk(m_p->m_clientLocker);

    if (m_p->m_session) {
        return m_p->m_session->isSubscribed(subscriptionRequest);
    }

    return false;
}

bool WSClient::isAuthenticated() const {
    std::lock_guard lk(m_p->m_clientLocker);

    if (m_p->m_session) {
        return m_p->m_session->isAuthenticated();
    }

    return false;
}
};
