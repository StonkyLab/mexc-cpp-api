/**
MEXC Futures WebSocket Session

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include "vk/mexc/mexc_futures_ws_session.h"
#include "vk/utils/log_utils.h"
#include "vk/utils/json_utils.h"
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/strand.hpp>

namespace vk::mexc::futures {
static constexpr int PING_INTERVAL_IN_S = 20;

WebSocketSession::WebSocketSession(boost::asio::io_context &ioc, boost::asio::ssl::context &ctx,
                                   const onLogMessage &onLogMessageCB) : m_resolver(make_strand(ioc)),
                                                                         m_ws(make_strand(ioc), ctx),
                                                                         m_pingTimer(ioc, boost::asio::chrono::seconds(
                                                                             PING_INTERVAL_IN_S)) {
    m_logMessageCB = onLogMessageCB;
    m_logMessageCB(LogSeverity::Info, "WebSocketSession created");
}

WebSocketSession::~WebSocketSession() {
    m_pingTimer.cancel();
    m_logMessageCB(LogSeverity::Info, "WebSocketSession destroyed");
}

void WebSocketSession::writeSubscription(const nlohmann::json &subscriptionRequest) {
    std::lock_guard lk(m_subscriptionLocker);

    /// Check if already subscribed
    for (const auto &sub: m_subscriptions) {
        if (sub["method"] == subscriptionRequest["method"]) {
            if (sub["symbol"] == subscriptionRequest["symbol"]) {
                return;
            }
        }
    }

    m_subscriptionRequests.emplace_back(subscriptionRequest);
}

std::string WebSocketSession::readSubscription() {
    std::lock_guard lk(m_subscriptionLocker);
    std::string retVal;

    if (m_subscriptionRequests.empty()) {
        return "";
    }

    try {
        m_subscribing = m_subscriptionRequests.front();
        retVal = m_subscriptionRequests.front().dump();
    } catch (std::exception &e) {
        m_logMessageCB(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, e.what()));
    }

    m_subscriptionRequests.pop_front();
    return retVal;
}

bool WebSocketSession::isApiControlMsg(const nlohmann::json &json) {
    if (json.contains("channel")) {
        {
            if (const auto channel = json["channel"].get<std::string>(); channel.find("rs.") != std::string::npos) {
                return true;
            }
        }
    }

    return false;
}

void WebSocketSession::handleApiControlMsg(const nlohmann::json &json) {
    std::lock_guard lk(m_subscriptionLocker);

    if (json.contains("data")) {
        bool isError = false;
        isError = json["data"].get<std::string>() != "success";

        if (isError) {
            if (const auto it = std::ranges::find(m_subscriptions, m_subscribing); it != m_subscriptions.end()) {
                m_subscriptions.erase(it);
            }

            m_logMessageCB(LogSeverity::Error,
                           fmt::format("MEXC API Error, subscription failed {}", m_subscribing.dump()));
            m_subscribing = nlohmann::json();
            return;
        }
        m_subscriptions.push_back(m_subscribing);
        m_subscribing = nlohmann::json();
    }

    m_logMessageCB(LogSeverity::Info, fmt::format("MEXC API control msg: {}", json.dump()));
}

void WebSocketSession::subscribe(const nlohmann::json &subscriptionRequest) {
    writeSubscription(subscriptionRequest);
}

bool WebSocketSession::isSubscribed(const nlohmann::json &subscriptionRequest) const {
    std::lock_guard lk(m_subscriptionLocker);

    if (const auto it = std::ranges::find(m_subscriptions, subscriptionRequest); it != m_subscriptions.end()) {
        return true;
    }

    return false;
}

void WebSocketSession::run(const std::string &host, const std::string &port, const nlohmann::json &subscriptionRequest,
                           const onDataEvent &dataEventCB) {
    m_host = host;
    writeSubscription(subscriptionRequest);
    m_dataEventCB = dataEventCB;

    /// Look up the domain name
    m_resolver.async_resolve(host, port,
                             boost::beast::bind_front_handler(&WebSocketSession::onResolve, shared_from_this()));
}

void
WebSocketSession::onResolve(const boost::beast::error_code &ec,
                            const boost::asio::ip::tcp::resolver::results_type &results) {
    if (ec) {
        return m_logMessageCB(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, ec.message()));
    }

    /// Set a timeout on the operation
    get_lowest_layer(m_ws).expires_after(std::chrono::seconds(30));

    /// Make the connection on the IP address we get from a lookup
    get_lowest_layer(m_ws).async_connect(results,
                                         boost::beast::bind_front_handler(&WebSocketSession::onConnect,
                                                                          shared_from_this()));
}

void WebSocketSession::onConnect(boost::beast::error_code ec,
                                 const boost::asio::ip::tcp::resolver::results_type::endpoint_type &ep) {
    if (ec) {
        return m_logMessageCB(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, ec.message()));
    }

    /// Set a timeout on the operation
    get_lowest_layer(m_ws).expires_after(std::chrono::seconds(30));

    /// Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(m_ws.next_layer().native_handle(), m_host.c_str())) {
        ec = boost::beast::error_code(static_cast<int>(ERR_get_error()), boost::asio::error::get_ssl_category());
        return m_logMessageCB(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, ec.message()));
    }

    /// Update the host_ string. This will provide the value of the
    /// Host HTTP header during the WebSocket handshake.
    /// See https://tools.ietf.org/html/rfc7230#section-5.4
    m_host += ':' + std::to_string(ep.port());

    /// Perform the SSL handshake
    m_ws.next_layer().async_handshake(boost::asio::ssl::stream_base::client,
                                      boost::beast::bind_front_handler(&WebSocketSession::onSSLHandshake,
                                                                       shared_from_this()));
}

void WebSocketSession::onSSLHandshake(const boost::beast::error_code &ec) {
    if (ec) {
        return m_logMessageCB(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, ec.message()));
    }

    m_ws.control_callback([this](boost::beast::websocket::frame_type kind, boost::beast::string_view payload) {
        boost::ignore_unused(kind, payload);

        if (kind == boost::beast::websocket::frame_type::pong) {
            m_lastPongTime = std::chrono::system_clock::now();
        }
    });

    /// Turn off the timeout on the tcp_stream, because
    /// the websocket stream has its own timeout system.
    get_lowest_layer(m_ws).expires_never();

    /// Set suggested timeout settings for the websocket
    m_ws.set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));

    /// Set a decorator to change the User-Agent of the handshake
    m_ws.set_option(boost::beast::websocket::stream_base::decorator([](boost::beast::websocket::request_type &req) {
        req.set(boost::beast::http::field::user_agent,
                std::string(BOOST_BEAST_VERSION_STRING) + " mexc-client");
    }));

    /// Perform the websocket handshake
    m_ws.async_handshake(m_host, "/edge",
                         boost::beast::bind_front_handler(&WebSocketSession::onHandshake, shared_from_this()));
}

void WebSocketSession::onHandshake(const boost::beast::error_code &ec) {
    if (ec) {
        return m_logMessageCB(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, ec.message()));
    }

    /// Start reading the messages
    m_pingTimer.async_wait(boost::beast::bind_front_handler(&WebSocketSession::onPingTimer, shared_from_this()));

    /// Subscribe to the topic
    m_ws.async_write(boost::asio::buffer(readSubscription()),
                     boost::beast::bind_front_handler(&WebSocketSession::onWrite, shared_from_this()));
}

void WebSocketSession::onWrite(const boost::beast::error_code &ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        return m_logMessageCB(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, ec.message()));
    }

    /// Read a message into our buffer
    m_ws.async_read(m_buffer, boost::beast::bind_front_handler(&WebSocketSession::onRead, shared_from_this()));
}

void WebSocketSession::onRead(const boost::beast::error_code &ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if (ec) {
        m_pingTimer.cancel();
        return m_logMessageCB(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, ec.message()));
    }

    try {
        const auto size = m_buffer.size();
        std::string strBuffer;
        strBuffer.reserve(size);

        for (const auto &it: m_buffer.data()) {
            strBuffer.append(static_cast<const char *>(it.data()), it.size());
        }

        m_buffer.consume(m_buffer.size());

        if (const nlohmann::json json = nlohmann::json::parse(strBuffer); json.is_object()) {
            if (isApiControlMsg(json)) {
                handleApiControlMsg(json);
            } else {
                try {
                    Event dataEvent;
                    dataEvent.fromJson(json);

                    if (m_dataEventCB) {
                        m_dataEventCB(dataEvent);
                    }
                } catch (std::exception &e) {
                    m_logMessageCB(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, e.what()));
                }
            }
        }

        /// Subscribe to new filter - if any, or continue to read the messages
        if (const auto subscription = readSubscription(); !subscription.empty()) {
            m_ws.async_write(boost::asio::buffer(subscription),
                             boost::beast::bind_front_handler(&WebSocketSession::onWrite, shared_from_this()));
        } else {
            std::lock_guard lk(m_subscriptionLocker);
            if (m_subscriptions.empty()) {
                m_logMessageCB(LogSeverity::Warning,
                               fmt::format("No subscriptions, WebSocketSession quit: {}", MAKE_FILELINE));
                close();
            }
            m_ws.async_read(m_buffer, boost::beast::bind_front_handler(&WebSocketSession::onRead, shared_from_this()));
        }
    } catch (nlohmann::json::exception &e) {
        m_logMessageCB(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, e.what()));
        m_ws.async_close(boost::beast::websocket::close_code::normal,
                         boost::beast::bind_front_handler(&WebSocketSession::onClose, shared_from_this()));
    }
}

void WebSocketSession::ping() {
    if (const std::chrono::duration<double> elapsed = m_lastPingTime - m_lastPongTime; elapsed.count() >
        PING_INTERVAL_IN_S) {
        m_logMessageCB(LogSeverity::Warning, fmt::format("{}: {}", MAKE_FILELINE, "ping expired"));
    }

    if (m_ws.is_open()) {
        const boost::beast::websocket::ping_data pingWebSocketFrame;
        m_ws.async_ping(pingWebSocketFrame, [this](const boost::beast::error_code &ec) {
                            if (ec) {
                                m_logMessageCB(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, ec.message()));
                            } else {
                                m_lastPingTime = std::chrono::system_clock::now();
                            }
                        }
        );
    }
}

void WebSocketSession::close() {
    m_ws.async_close(boost::beast::websocket::close_code::normal,
                     boost::beast::bind_front_handler(&WebSocketSession::onClose, shared_from_this()));
}

void WebSocketSession::onClose(const boost::beast::error_code &ec) {
    m_pingTimer.cancel();

    if (ec) {
        return m_logMessageCB(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, ec.message()));
    }
}

void WebSocketSession::onPingTimer(const boost::beast::error_code &ec) {
    if (ec) {
        return m_logMessageCB(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, ec.message()));
    }

    ping();
    m_pingTimer.expires_after(boost::asio::chrono::seconds(PING_INTERVAL_IN_S));
    m_pingTimer.async_wait(boost::beast::bind_front_handler(&WebSocketSession::onPingTimer, shared_from_this()));
}
}
