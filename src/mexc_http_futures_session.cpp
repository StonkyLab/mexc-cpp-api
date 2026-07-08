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
#include <thread>
#include "date.h"
#include "stonky/utils/utils.h"

namespace stonky::mexc::futures {
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

const auto API_URI_FUTURES = "contract.mexc.com";

struct HTTPSession::P {
	net::io_context ioc;
	std::string apiKey;
	int receiveWindow = 25000;
	std::string apiSecret;
	std::string uri;
	const EVP_MD *evpMd;

	P() : evpMd(EVP_sha256()) {
		uri = API_URI_FUTURES;
	}

	http::response<http::string_body> request(http::request<http::string_body> req);

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

	return m_p->request(req);
}

http::response<http::string_body> HTTPSession::P::request(
	http::request<http::string_body> req) {
	req.set(http::field::host, uri);
	req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

	// requestSent tells the retry loop whether the failure happened BEFORE the
	// request left the socket. Pre-write failures (SNI / resolve / connect / TLS
	// handshake) never reached the server, so re-sending is always safe — even
	// for a non-idempotent order submit.
	const auto performOnce = [this, &req](bool &requestSent) {
		requestSent = false;

		ssl::context ctx{ssl::context::sslv23_client};
		enableTlsPeerVerification(ctx);

		tcp::resolver resolver{ioc};
		ssl::stream<tcp::socket> stream{ioc, ctx};
		stream.set_verify_callback(ssl::host_name_verification(uri));

		// Set SNI Hostname (many hosts need this to handshake successfully)
		if (!SSL_set_tlsext_host_name(stream.native_handle(), uri.c_str())) {
			boost::system::error_code ec{
				static_cast<int>(::ERR_get_error()),
				net::error::get_ssl_category()
			};
			throw boost::system::system_error{ec};
		}

		auto const results = resolver.resolve(uri, "443");
		net::connect(stream.next_layer(), results.begin(), results.end());
		stream.handshake(ssl::stream_base::client);

		requestSent = true;
		http::write(stream, req);
		beast::flat_buffer buffer;
		http::response<http::string_body> response;
		http::read(stream, buffer, response);

		boost::system::error_code ec;
		stream.shutdown(ec);
		if (ec == boost::asio::error::eof) {
			// Rationale:
			// http://stackoverflow.com/questions/25587403/boost-asio-ssl-async-shutdown-always-finishes-with-an-error
			ec.assign(0, ec.category());
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
	// silently re-sent. Signatures stay valid across retries (Request-Time
	// tolerance).
	constexpr int MAX_RETRIES = 4;

	for (int attempt = 0;; ++attempt) {
		bool requestSent = false;
		try {
			auto response = performOnce(requestSent);
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
