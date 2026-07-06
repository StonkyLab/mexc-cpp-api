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
}
}
