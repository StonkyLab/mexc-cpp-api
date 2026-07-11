/**
MEXC Futures HTTPS Session

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@stonky.cz>, Stonky s.r.o.
*/

#include "stonky/mexc/mexc_http_futures_session.h"
#include "stonky/mexc/tls_verify.h"
#include "nlohmann/json.hpp"
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/beast/version.hpp>
#include <spdlog/spdlog.h>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
#include "date.h"
#include "stonky/utils/utils.h"

namespace stonky::mexc::futures {
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

const auto API_URI_FUTURES = "contract.mexc.com";

struct HTTPSession::P {
	std::string apiKey;
	int receiveWindow = 25000;
	std::string apiSecret;
	std::string uri;
	const EVP_MD *evpMd;
	std::function<void()> retryPacingHook;

	/// One keep-alive connection: TLS stream plus its own io_context (all ops
	/// are blocking; a per-connection ioc keeps concurrent requests on
	/// different connections fully independent).
	struct Conn {
		net::io_context ioc;
		ssl::context ctx{ssl::context::sslv23_client};
		std::unique_ptr<ssl::stream<tcp::socket>> stream;
		std::chrono::steady_clock::time_point lastUsed{};
	};

	/// Idle keep-alive connections. Every request previously paid a fresh
	/// DNS+TCP+TLS handshake (~300-600 ms) — the dominant per-action latency
	/// of a chase re-price. Reuse is capped by idle age (the server/edge
	/// silently closes idle connections, and a request written into a
	/// half-closed socket cannot be safely retried when non-idempotent) and
	/// by pool size (fd hygiene).
	std::mutex poolM;
	std::vector<std::unique_ptr<Conn>> pool;
	static constexpr auto maxConnIdle = std::chrono::seconds(25);
	static constexpr std::size_t maxPoolSize = 4;

	P() : evpMd(EVP_sha256()) {
		uri = API_URI_FUTURES;
	}

	[[nodiscard]] std::unique_ptr<Conn> takeConn() {
		std::lock_guard lk(poolM);
		const auto now = std::chrono::steady_clock::now();

		while (!pool.empty()) {
			auto conn = std::move(pool.back());
			pool.pop_back();

			if (now - conn->lastUsed <= maxConnIdle) {
				return conn;
			}
			/// aged out — dropped; the destructor closes the socket
		}

		return nullptr;
	}

	void returnConn(std::unique_ptr<Conn> conn) {
		conn->lastUsed = std::chrono::steady_clock::now();
		std::lock_guard lk(poolM);

		if (pool.size() < maxPoolSize) {
			pool.push_back(std::move(conn));
		}
	}

	/// Fresh DNS+TCP+TLS connection. Every failure in here happens BEFORE any
	/// request byte leaves the socket, so the caller may always re-send.
	[[nodiscard]] std::unique_ptr<Conn> freshConn() {
		auto conn = std::make_unique<Conn>();
		enableTlsPeerVerification(conn->ctx);
		conn->stream = std::make_unique<ssl::stream<tcp::socket>>(conn->ioc, conn->ctx);
		conn->stream->set_verify_callback(ssl::host_name_verification(uri));

		// Set SNI Hostname (many hosts need this to handshake successfully)
		if (!SSL_set_tlsext_host_name(conn->stream->native_handle(), uri.c_str())) {
			boost::system::error_code ec{
				static_cast<int>(::ERR_get_error()),
				net::error::get_ssl_category()
			};
			throw boost::system::system_error{ec};
		}

		tcp::resolver resolver{conn->ioc};
		auto const results = resolver.resolve(uri, "443");
		net::connect(conn->stream->next_layer(), results.begin(), results.end());
		conn->stream->handshake(ssl::stream_base::client);
		return conn;
	}

	http::response<http::string_body> request(http::request<http::string_body> req,
	                                          const std::function<void(http::request<http::string_body> &)> &reauth = {});

	static std::string createQueryStr(const std::map<std::string, std::string> &parameters) {
		std::string queryStr;

		for (const auto &[fst, snd]: parameters) {
			queryStr.append(fst);
			queryStr.append("=");
			queryStr.append(snd);
			queryStr.append("&");
		}

		if (!queryStr.empty()) {
			queryStr.pop_back();
		}
		return queryStr;
	}

	/// OpenAPI signature = HMAC-SHA256(secret, apiKey + timestamp + payload),
	/// hex-encoded. `payload` is the query string for GET, the JSON body for
	/// POST — the only difference between the two request kinds.
	[[nodiscard]] std::string sign(const std::string &payload, const std::string &timestamp) const {
		const std::string strToSign = apiKey + timestamp + payload;

		unsigned char digest[SHA256_DIGEST_LENGTH];
		unsigned int digestLength = SHA256_DIGEST_LENGTH;

		HMAC(evpMd, apiSecret.data(), apiSecret.size(),
		     reinterpret_cast<const unsigned char *>(strToSign.data()),
		     strToSign.length(), digest, &digestLength);

		return stringToHex(digest, sizeof(digest));
	}

	/// OpenAPI authentication for GET requests (HMAC-SHA256 over the query string)
	void authenticateGet(http::request<http::string_body> &req, const std::map<std::string, std::string> &parameters) const {
		const auto ts = std::to_string(getMsTimestamp(currentTime()).count());

		req.set("ApiKey", apiKey);
		req.set("Content-Type", "application/json");
		req.set("Request-Time", ts);
		req.set("Signature", sign(createQueryStr(parameters), ts));
	}

	/// OpenAPI authentication for POST requests (HMAC-SHA256 over the JSON body)
	void authenticatePost(http::request<http::string_body> &req, const std::string &jsonBody) const {
		const auto ts = std::to_string(getMsTimestamp(currentTime()).count());

		req.set("ApiKey", apiKey);
		req.set("Content-Type", "application/json");
		req.set("Request-Time", ts);
		req.set("Signature", sign(jsonBody, ts));
	}
};

HTTPSession::HTTPSession(const std::string &apiKey, const std::string &apiSecret) : m_p(
	std::make_unique<P>()) {
	m_p->apiKey = apiKey;
	m_p->apiSecret = apiSecret;
	m_p->uri = API_URI_FUTURES;
}

HTTPSession::~HTTPSession() = default;

http::response<http::string_body> HTTPSession::methodGet(const std::string &path,
                                                         const std::map<std::string, std::string> &parameters,
                                                         const bool isPublic) const {
	std::string finalPath = path;

	if (const auto queryString = P::createQueryStr(parameters); !queryString.empty()) {
		finalPath.append("?");
		finalPath.append(queryString);
	}

	http::request<http::string_body> req{http::verb::get, finalPath, 11};

	if (!isPublic) {
		m_p->authenticateGet(req, parameters);
		return m_p->request(req, [this, &parameters](http::request<http::string_body> &r) { m_p->authenticateGet(r, parameters); });
	}

	return m_p->request(req);
}

http::response<http::string_body> HTTPSession::methodPost(const std::string &path,
                                                          const std::string &jsonBody) const {
	http::request<http::string_body> req{http::verb::post, path, 11};
	req.set(http::field::content_type, "application/json");
	req.body() = jsonBody;
	req.prepare_payload();

	m_p->authenticatePost(req, jsonBody);

	return m_p->request(req, [this, &jsonBody](http::request<http::string_body> &r) { m_p->authenticatePost(r, jsonBody); });
}

void HTTPSession::setRetryPacingHook(const std::function<void()> &hook) { m_p->retryPacingHook = hook; }

http::response<http::string_body> HTTPSession::P::request(
	http::request<http::string_body> req, const std::function<void(http::request<http::string_body> &)> &reauth) {
	req.set(http::field::host, uri);
	req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

	// requestSent tells the retry loop whether the failure happened BEFORE the
	// request left the socket. Pre-write failures (SNI / resolve / connect / TLS
	// handshake) never reached the server, so re-sending is always safe — even
	// for a non-idempotent order submit. usedPooledConn marks a reused
	// keep-alive connection: a post-write failure there is most likely the
	// server's silent idle-close, retryable for idempotent requests.
	const auto performOnce = [this, &req](bool &requestSent, bool &usedPooledConn) {
		requestSent = false;
		auto conn = takeConn();
		usedPooledConn = conn != nullptr;

		if (!conn) {
			conn = freshConn();
		}

		requestSent = true;
		http::write(*conn->stream, req);
		beast::flat_buffer buffer;
		http::response<http::string_body> response;
		http::read(*conn->stream, buffer, response);

		if (response.keep_alive()) {
			/// Server keeps the connection open — pool it for the next request.
			returnConn(std::move(conn));
		} else {
			boost::system::error_code ec;
			conn->stream->shutdown(ec);
			if (ec == boost::asio::error::eof) {
				// Rationale:
				// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
				ec.assign(0, ec.category());
			}
		}

		return response;
	};

	// One choke point for every MEXC request, retrying the two cases where the
	// request provably did NOT execute:
	//   * a pre-write transport failure — transient TLS handshake alerts /
	//     connection resets from MEXC's CloudFront WAF (live-observed
	//     "tlsv1 alert internal error" that otherwise killed the executor at
	//     startup). Safe because nothing was sent.
	//   * an app-level 510 "Requests are too frequent" — the venue REFUSED it.
	// A failure AFTER the write propagates, so a non-idempotent POST is never
	// silently re-sent.
	constexpr int MAX_RETRIES = 4;
	bool staleConnRetried = false;

	for (int attempt = 0;; ++attempt) {
		if (attempt > 0) {
			// A retry is a NEW venue request. It must (a) take a rate-limiter
			// slot like any other (unpaced ladders from concurrent legs kept the
			// aggregate above the venue cap, so storms self-sustained) and (b)
			// carry a FRESH Request-Time/Signature — the tail of the old ladder
			// re-sent a >10 s-stale signature, which the venue rejected with 513
			// "Invalid request, please try again later" (live: 4 of 4 ladder
			// exhaustions ended that way).
			if (retryPacingHook) {
				retryPacingHook();
			}

			if (reauth) {
				reauth(req);
			}
		}

		bool requestSent = false;
		bool usedPooledConn = false;
		try {
			auto response = performOnce(requestSent, usedPooledConn);
			const auto &body = response.body();
			const bool throttled = body.find("\"code\":510") != std::string::npos || body.find("\"code\": 510") != std::string::npos;

			if (!throttled || attempt >= MAX_RETRIES) {
				return response;
			}

			const auto pauseMs = 700LL << attempt; // 0.7 / 1.4 / 2.8 / 5.6 s
			spdlog::warn(fmt::format("MEXC HTTP throttled (510) — retry {}/{} in {} ms: {} {}", attempt + 1, MAX_RETRIES, pauseMs,
			                         std::string(req.method_string()), std::string(req.target())));
			std::this_thread::sleep_for(std::chrono::milliseconds(pauseMs));
		} catch (const std::exception &e) {
			/// A request that died AFTER the write on a REUSED connection most
			/// likely hit the server's silent idle-close, not a real transport
			/// fault. A GET is idempotent — retry it once, immediately, on a
			/// fresh connection (the pooled one was discarded by the failure).
			/// A POST must propagate: the order may or may not have reached the
			/// venue, and the caller's safety-cancel path owns that ambiguity.
			if (requestSent && usedPooledConn && req.method() == http::verb::get && !staleConnRetried) {
				staleConnRetried = true;
				spdlog::debug(fmt::format("MEXC HTTP keep-alive connection was stale ({}) — retrying GET on a fresh one: {}", e.what(), std::string(req.target())));
				continue;
			}

			if (requestSent || attempt >= MAX_RETRIES) {
				throw; // failed after the request was sent, or retries exhausted
			}

			const auto pauseMs = 500LL << attempt; // 0.5 / 1 / 2 / 4 s
			spdlog::warn(fmt::format("MEXC HTTP connect/handshake failed ({}) — retry {}/{} in {} ms: {} {}", e.what(), attempt + 1, MAX_RETRIES, pauseMs,
			                         std::string(req.method_string()), std::string(req.target())));
			std::this_thread::sleep_for(std::chrono::milliseconds(pauseMs));
		}
	}
}
}
