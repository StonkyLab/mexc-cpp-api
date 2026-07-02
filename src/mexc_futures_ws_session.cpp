/**
MEXC Futures WebSocket Session

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@stonky.cz>, Stonky s.r.o.
*/

#include "stonky/mexc/mexc_futures_ws_session.h"
#include "stonky/utils/log_utils.h"
#include "stonky/utils/json_utils.h"
#include "stonky/utils/utils.h"
#include <fmt/format.h>
#include <nlohmann/json.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <atomic>
#include <list>

namespace stonky::mexc::futures {
static constexpr int PING_INTERVAL_IN_S = 15;
/// MEXC disconnects a contract WS with no ping for 60 s; reconnect earlier.
static constexpr int PONG_TIMEOUT_IN_S = 40;
static constexpr int RECONNECT_DELAY_MAX_IN_S = 30;
static constexpr int AUTH_ACK_TIMEOUT_IN_S = 15;

using WsStream = boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>;

namespace {
/// Two subscription requests are the same channel iff their JSON is equal
/// (nlohmann object equality is key-order-independent).
bool sameRequest(const nlohmann::json &a, const nlohmann::json &b) { return a == b; }

/// Turn a "sub.*" request into its "unsub.*" form.
nlohmann::json toUnsubscribe(const nlohmann::json &subscription) {
    nlohmann::json unsub = subscription;

    if (unsub.contains("method")) {
        if (auto method = unsub["method"].get<std::string>(); method.starts_with("sub.")) {
            unsub["method"] = "un" + method;
        }
    }

    return unsub;
}
} // namespace

struct WebSocketSession::P {
    boost::asio::io_context &ioc;
    boost::asio::ssl::context &ctx;
    boost::asio::strand<boost::asio::io_context::executor_type> strand;
    boost::asio::ip::tcp::resolver resolver;
    std::shared_ptr<WsStream> ws;
    boost::beast::multi_buffer buffer;

    std::string host;
    std::string port;
    std::string path{"/edge"};
    std::string hostHeader;

    std::string apiKey;
    std::string apiSecret;
    bool authRequired = false;

    onLogMessage logMessageCB;
    onDataEvent dataEventCB;

    /// Outbound write pump. One async_write in flight at most; login, subscribe
    /// and JSON pings all go through the queue so writes never depend on inbound
    /// traffic (fatal for a quiet private stream) and never race each other.
    std::list<std::string> outboundQueue;
    bool writeInFlight = false;
    std::string writeBuffer;

    /// Confirmed (sent) subscriptions and pending ones. On reconnect all
    /// subscriptions move back to pending and are replayed. Private streams keep
    /// both empty — personal channels auto-push after login.
    std::vector<nlohmann::json> subscriptions;
    std::list<nlohmann::json> pendingTopics;
    mutable std::recursive_mutex locker;

    boost::asio::steady_timer pingTimer;
    boost::asio::steady_timer reconnectTimer;
    int reconnectDelayS = 1;
    int generation = 0;
    bool connected = false;
    bool authenticated = false;
    std::atomic<bool> authenticatedFlag{false};
    bool reconnectScheduled = false;
    bool userClosed = false;
    std::chrono::steady_clock::time_point lastPongTime{};
    std::chrono::steady_clock::time_point authDeadline{std::chrono::steady_clock::time_point::max()};

    P(boost::asio::io_context &ioContext, boost::asio::ssl::context &sslCtx, const onLogMessage &onLogMessageCB) :
        ioc(ioContext), ctx(sslCtx), strand(boost::asio::make_strand(ioContext)), resolver(strand), logMessageCB(onLogMessageCB), pingTimer(strand),
        reconnectTimer(strand) {}

    void log(const LogSeverity severity, const std::string &message) const {
        if (logMessageCB) {
            logMessageCB(severity, message);
        }
    }

    [[nodiscard]] bool isUserClosed() const {
        std::lock_guard lk(locker);
        return userClosed;
    }

    // ── Outbound write pump ─────────────────────────────────────────

    void enqueueOp(const std::string &payload) {
        std::lock_guard lk(locker);
        outboundQueue.push_back(payload);
    }

    void pump(const std::shared_ptr<WebSocketSession> &self) {
        if (writeInFlight || !connected || !ws || !ws->is_open()) {
            return;
        }

        {
            std::lock_guard lk(locker);

            if (outboundQueue.empty()) {
                return;
            }

            writeBuffer = std::move(outboundQueue.front());
            outboundQueue.pop_front();
        }

        writeInFlight = true;
        ws->async_write(boost::asio::buffer(writeBuffer),
                        [this, self, gen = generation, wsRef = ws](const boost::beast::error_code &ec, const std::size_t bytesTransferred) {
                            boost::ignore_unused(bytesTransferred, wsRef);

                            if (gen != generation) {
                                return;
                            }

                            writeInFlight = false;

                            if (ec) {
                                return handleError(self, gen, fmt::format("{}: write: {}", MAKE_FILELINE, ec.message()));
                            }

                            pump(self);
                        });
    }

    // ── Subscription bookkeeping ────────────────────────────────────

    void addTopic(const nlohmann::json &topic) {
        std::lock_guard lk(locker);

        if (std::ranges::any_of(subscriptions, [&topic](const auto &s) { return sameRequest(s, topic); })) {
            return;
        }

        if (std::ranges::any_of(pendingTopics, [&topic](const auto &s) { return sameRequest(s, topic); })) {
            return;
        }

        pendingTopics.push_back(topic);
    }

    bool removeTopic(const nlohmann::json &topic) {
        std::lock_guard lk(locker);
        std::erase_if(pendingTopics, [&topic](const auto &s) { return sameRequest(s, topic); });

        if (const auto it = std::ranges::find_if(subscriptions, [&topic](const auto &s) { return sameRequest(s, topic); }); it != subscriptions.end()) {
            subscriptions.erase(it);
            return true;
        }

        return false;
    }

    [[nodiscard]] bool isSubscribed(const nlohmann::json &topic) const {
        std::lock_guard lk(locker);
        return std::ranges::any_of(subscriptions, [&topic](const auto &s) { return sameRequest(s, topic); }) ||
               std::ranges::any_of(pendingTopics, [&topic](const auto &s) { return sameRequest(s, topic); });
    }

    /// Send each pending subscription as its own op (MEXC accepts one method per
    /// message). No-op while disconnected or awaiting the login ack.
    void flushTopics(const std::shared_ptr<WebSocketSession> &self) {
        if (!connected || (authRequired && !authenticated)) {
            return;
        }

        std::vector<nlohmann::json> toSend;
        {
            std::lock_guard lk(locker);

            for (auto &topic: pendingTopics) {
                subscriptions.push_back(topic);
                toSend.push_back(topic);
            }

            pendingTopics.clear();
        }

        for (const auto &topic: toSend) {
            enqueueOp(topic.dump());
        }

        pump(self);
    }

    // ── Auth ────────────────────────────────────────────────────────

    void sendLogin(const std::shared_ptr<WebSocketSession> &self) {
        const auto reqTime = std::to_string(getMsTimestamp(currentTime()).count());
        const std::string payload = apiKey + reqTime;

        unsigned char digest[SHA256_DIGEST_LENGTH];
        unsigned int digestLength = SHA256_DIGEST_LENGTH;

        HMAC(EVP_sha256(), apiSecret.data(), static_cast<int>(apiSecret.size()), reinterpret_cast<const unsigned char *>(payload.data()), payload.length(), digest,
             &digestLength);

        nlohmann::json loginJson;
        loginJson["subscribe"] = true;
        loginJson["method"] = "login";
        loginJson["param"] = {{"apiKey", apiKey}, {"reqTime", reqTime}, {"signature", stringToHex(digest, sizeof(digest))}};

        authDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(AUTH_ACK_TIMEOUT_IN_S);
        enqueueOp(loginJson.dump());
        pump(self);
    }

    // ── Connect chain ───────────────────────────────────────────────

    void startConnect(const std::shared_ptr<WebSocketSession> &self) {
        if (isUserClosed()) {
            return;
        }

        ++generation;
        const int gen = generation;

        connected = false;
        authenticated = false;
        authenticatedFlag = false;
        writeInFlight = false;
        authDeadline = std::chrono::steady_clock::time_point::max();
        hostHeader = host;
        buffer.consume(buffer.size());
        ws = std::make_shared<WsStream>(strand, ctx);

        resolver.async_resolve(host, port, [this, self, gen](const boost::beast::error_code &ec, const boost::asio::ip::tcp::resolver::results_type &results) {
            if (gen != generation) {
                return;
            }

            onResolve(self, gen, ec, results);
        });
    }

    void onResolve(const std::shared_ptr<WebSocketSession> &self, const int gen, const boost::beast::error_code &ec,
                   const boost::asio::ip::tcp::resolver::results_type &results) {
        if (ec) {
            return handleError(self, gen, fmt::format("{}: {}", MAKE_FILELINE, ec.message()));
        }

        get_lowest_layer(*ws).expires_after(std::chrono::seconds(30));

        get_lowest_layer(*ws).async_connect(results, [this, self, gen, wsRef = ws](const boost::beast::error_code &e,
                                                                                   const boost::asio::ip::tcp::resolver::results_type::endpoint_type &ep) {
            boost::ignore_unused(wsRef);

            if (gen != generation) {
                return;
            }

            onConnect(self, gen, e, ep);
        });
    }

    void onConnect(const std::shared_ptr<WebSocketSession> &self, const int gen, boost::beast::error_code ec,
                   const boost::asio::ip::tcp::resolver::results_type::endpoint_type &ep) {
        if (ec) {
            return handleError(self, gen, fmt::format("{}: {}", MAKE_FILELINE, ec.message()));
        }

        get_lowest_layer(*ws).expires_after(std::chrono::seconds(30));

        if (!SSL_set_tlsext_host_name(ws->next_layer().native_handle(), host.c_str())) {
            ec = boost::beast::error_code(static_cast<int>(ERR_get_error()), boost::asio::error::get_ssl_category());
            return handleError(self, gen, fmt::format("{}: {}", MAKE_FILELINE, ec.message()));
        }

        hostHeader = host + ':' + std::to_string(ep.port());

        ws->next_layer().async_handshake(boost::asio::ssl::stream_base::client, [this, self, gen, wsRef = ws](const boost::beast::error_code &e) {
            boost::ignore_unused(wsRef);

            if (gen != generation) {
                return;
            }

            onSSLHandshake(self, gen, e);
        });
    }

    void onSSLHandshake(const std::shared_ptr<WebSocketSession> &self, const int gen, const boost::beast::error_code &ec) {
        if (ec) {
            return handleError(self, gen, fmt::format("{}: {}", MAKE_FILELINE, ec.message()));
        }

        ws->control_callback([this](const boost::beast::websocket::frame_type kind, boost::beast::string_view payload) {
            boost::ignore_unused(payload);

            if (kind == boost::beast::websocket::frame_type::pong) {
                lastPongTime = std::chrono::steady_clock::now();
            }
        });

        get_lowest_layer(*ws).expires_never();

        ws->set_option(boost::beast::websocket::stream_base::timeout::suggested(boost::beast::role_type::client));

        ws->set_option(boost::beast::websocket::stream_base::decorator(
                [](boost::beast::websocket::request_type &req) { req.set(boost::beast::http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " mexc-client"); }));

        ws->async_handshake(hostHeader, path, [this, self, gen, wsRef = ws](const boost::beast::error_code &e) {
            boost::ignore_unused(wsRef);

            if (gen != generation) {
                return;
            }

            onHandshake(self, gen, e);
        });
    }

    void onHandshake(const std::shared_ptr<WebSocketSession> &self, const int gen, const boost::beast::error_code &ec) {
        if (ec) {
            return handleError(self, gen, fmt::format("{}: {}", MAKE_FILELINE, ec.message()));
        }

        if (isUserClosed()) {
            return;
        }

        connected = true;
        lastPongTime = std::chrono::steady_clock::now();

        pingTimer.expires_after(boost::asio::chrono::seconds(PING_INTERVAL_IN_S));
        pingTimer.async_wait([this, self, gen](const boost::beast::error_code &e) { onPingTimer(self, gen, e); });

        if (authRequired) {
            sendLogin(self);
        } else {
            flushTopics(self);
        }

        ws->async_read(buffer, [this, self, gen, wsRef = ws](const boost::beast::error_code &e, const std::size_t transferred) {
            boost::ignore_unused(wsRef);

            if (gen != generation) {
                return;
            }

            onRead(self, gen, e, transferred);
        });
    }

    // ── Inbound ─────────────────────────────────────────────────────

    void onRead(const std::shared_ptr<WebSocketSession> &self, const int gen, const boost::beast::error_code &ec, std::size_t bytesTransferred) {
        boost::ignore_unused(bytesTransferred);

        if (ec) {
            return handleError(self, gen, fmt::format("{}: {}", MAKE_FILELINE, ec.message()));
        }

        try {
            const auto size = buffer.size();
            std::string strBuffer;
            strBuffer.reserve(size);

            for (const auto &it: buffer.data()) {
                strBuffer.append(static_cast<const char *>(it.data()), it.size());
            }

            buffer.consume(buffer.size());

            if (const nlohmann::json json = nlohmann::json::parse(strBuffer); json.is_object()) {
                if (isControlMsg(json)) {
                    handleControlMsg(self, gen, json);
                } else {
                    try {
                        Event dataEvent;
                        dataEvent.fromJson(json);

                        if (dataEventCB) {
                            dataEventCB(dataEvent);
                        }
                    } catch (std::exception &e) {
                        log(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, e.what()));
                    }
                }
            }

            if (gen != generation || reconnectScheduled || !connected) {
                return;
            }

            if (!authRequired || authenticated) {
                reconnectDelayS = 1;
            }

            ws->async_read(buffer, [this, self, gen, wsRef = ws](const boost::beast::error_code &e, const std::size_t transferred) {
                boost::ignore_unused(wsRef);

                if (gen != generation) {
                    return;
                }

                onRead(self, gen, e, transferred);
            });
        } catch (nlohmann::json::exception &exc) {
            handleError(self, gen, fmt::format("{}: {}", MAKE_FILELINE, exc.what()));
        }
    }

    /// Control message = an ack/heartbeat, channel starting with "rs." or the
    /// "pong" channel. Data pushes have a "push.*" channel.
    static bool isControlMsg(const nlohmann::json &json) {
        if (const auto it = json.find("channel"); it != json.end() && it->is_string()) {
            const auto channel = it->get<std::string>();
            return channel.starts_with("rs.") || channel == "pong" || channel == "clientId";
        }

        return false;
    }

    void handleControlMsg(const std::shared_ptr<WebSocketSession> &self, const int gen, const nlohmann::json &json) {
        std::string channel;
        readValue<std::string>(json, "channel", channel);

        if (channel == "pong") {
            lastPongTime = std::chrono::steady_clock::now();
            return;
        }

        if (channel == "rs.login") {
            std::string data;
            readValue<std::string>(json, "data", data);

            if (data == "success") {
                authenticated = true;
                authenticatedFlag = true;
                authDeadline = std::chrono::steady_clock::time_point::max();
                reconnectDelayS = 1;
                log(LogSeverity::Info, "MEXC WS authenticated");
                flushTopics(self);
            } else {
                handleError(self, gen, fmt::format("MEXC WS login failed: {}", json.dump()));
            }
            return;
        }

        if (channel == "rs.error") {
            log(LogSeverity::Error, fmt::format("MEXC WS error: {}", json.dump()));
            return;
        }

        /// rs.sub.* / rs.unsub.* — subscribe acks. data == "success" on ok.
        std::string data;
        readValue<std::string>(json, "data", data);

        if (!data.empty() && data != "success") {
            log(LogSeverity::Error, fmt::format("MEXC WS subscribe failed: {}", json.dump()));
        }
    }

    // ── Heartbeat ───────────────────────────────────────────────────

    void onPingTimer(const std::shared_ptr<WebSocketSession> &self, const int gen, const boost::beast::error_code &ec) {
        if (ec || gen != generation) {
            return;
        }

        if (isUserClosed()) {
            return;
        }

        if (connected) {
            const auto now = std::chrono::steady_clock::now();

            if (const auto sincePong = std::chrono::duration_cast<std::chrono::seconds>(now - lastPongTime).count(); sincePong > PONG_TIMEOUT_IN_S) {
                return handleError(self, gen, fmt::format("{}: no pong for {} s", MAKE_FILELINE, sincePong));
            }

            if (authRequired && !authenticated && now > authDeadline) {
                return handleError(self, gen, fmt::format("{}: login ack timeout", MAKE_FILELINE));
            }

            enqueueOp(nlohmann::json{{"method", "ping"}}.dump());
            pump(self);
        }

        pingTimer.expires_after(boost::asio::chrono::seconds(PING_INTERVAL_IN_S));
        pingTimer.async_wait([this, self, gen](const boost::beast::error_code &e) { onPingTimer(self, gen, e); });
    }

    // ── Error / reconnect / close ───────────────────────────────────

    void handleError(const std::shared_ptr<WebSocketSession> &self, const int gen, const std::string &message) {
        if (gen != generation || reconnectScheduled) {
            return;
        }

        log(LogSeverity::Error, message);

        if (isUserClosed()) {
            return;
        }

        reconnectScheduled = true;
        connected = false;
        authenticated = false;
        authenticatedFlag = false;
        writeInFlight = false;
        authDeadline = std::chrono::steady_clock::time_point::max();
        pingTimer.cancel();

        {
            std::lock_guard lk(locker);
            outboundQueue.clear();

            for (auto it = subscriptions.rbegin(); it != subscriptions.rend(); ++it) {
                pendingTopics.push_front(*it);
            }

            subscriptions.clear();
        }

        if (ws) {
            boost::beast::error_code ignored;
            get_lowest_layer(*ws).socket().close(ignored);
        }

        log(LogSeverity::Warning, fmt::format("MEXC WS reconnecting in {} s", reconnectDelayS));
        reconnectTimer.expires_after(boost::asio::chrono::seconds(reconnectDelayS));
        reconnectDelayS = std::min(reconnectDelayS * 2, RECONNECT_DELAY_MAX_IN_S);

        reconnectTimer.async_wait([this, self, gen](const boost::beast::error_code &e) {
            if (gen != generation) {
                return;
            }

            reconnectScheduled = false;

            if (e || isUserClosed()) {
                return;
            }

            startConnect(self);
        });
    }

    void closeByUser() {
        {
            std::lock_guard lk(locker);

            if (userClosed) {
                return;
            }

            userClosed = true;
        }

        pingTimer.cancel();
        reconnectTimer.cancel();
        resolver.cancel();

        if (!ws) {
            return;
        }

        if (ws->is_open() && !writeInFlight) {
            ws->async_close(boost::beast::websocket::close_code::normal, [this, wsRef = ws](const boost::beast::error_code &ec) {
                boost::ignore_unused(wsRef);

                if (ec) {
                    log(LogSeverity::Error, fmt::format("{}: {}", MAKE_FILELINE, ec.message()));
                }
            });
        } else {
            boost::beast::error_code ignored;
            get_lowest_layer(*ws).socket().close(ignored);
        }
    }
};

WebSocketSession::WebSocketSession(boost::asio::io_context &ioc, boost::asio::ssl::context &ctx, const onLogMessage &onLogMessageCB) :
    m_p(std::make_unique<P>(ioc, ctx, onLogMessageCB)) {}

WebSocketSession::~WebSocketSession() = default;

void WebSocketSession::setCredentials(const std::string &apiKey, const std::string &apiSecret) const {
    m_p->apiKey = apiKey;
    m_p->apiSecret = apiSecret;
    m_p->authRequired = !apiKey.empty() && !apiSecret.empty();
}

void WebSocketSession::subscribe(const nlohmann::json &subscriptionRequest) const {
    m_p->addTopic(subscriptionRequest);

    auto self = const_cast<WebSocketSession *>(this)->shared_from_this();
    post(m_p->strand, [self] { self->m_p->flushTopics(self); });
}

void WebSocketSession::unsubscribe(const nlohmann::json &subscriptionRequest) const {
    if (!m_p->removeTopic(subscriptionRequest)) {
        return;
    }

    auto self = const_cast<WebSocketSession *>(this)->shared_from_this();
    post(m_p->strand, [self, subscriptionRequest] {
        if (!self->m_p->connected) {
            return;
        }

        self->m_p->enqueueOp(toUnsubscribe(subscriptionRequest).dump());
        self->m_p->pump(self);
    });
}

bool WebSocketSession::isAuthenticated() const { return m_p->authenticatedFlag.load(); }

bool WebSocketSession::isSubscribed(const nlohmann::json &subscriptionRequest) const { return m_p->isSubscribed(subscriptionRequest); }

void WebSocketSession::run(const std::string &host, const std::string &port, const std::string &path, const nlohmann::json &subscriptionRequest,
                           const onDataEvent &dataEventCB) {
    m_p->host = host;
    m_p->port = port;
    m_p->path = path;
    m_p->dataEventCB = dataEventCB;

    /// Login-only (private) streams pass an empty request — personal channels
    /// auto-push after the login ack.
    if (!subscriptionRequest.is_null() && !subscriptionRequest.empty()) {
        m_p->addTopic(subscriptionRequest);
    }

    auto self = shared_from_this();
    post(m_p->strand, [self] { self->m_p->startConnect(self); });
}

void WebSocketSession::close() const {
    auto self = const_cast<WebSocketSession *>(this)->shared_from_this();
    post(m_p->strand, [self] { self->m_p->closeByUser(); });
}
} // namespace stonky::mexc::futures
