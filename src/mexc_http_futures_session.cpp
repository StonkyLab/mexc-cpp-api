/**
MEXC Futures HTTPS Session

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include "vk/mexc/mexc_http_futures_session.h"
#include "nlohmann/json.hpp"
#include <boost/asio/ssl.hpp>
#include <boost/beast/version.hpp>
#include "date.h"
#include "vk/utils/utils.h"

namespace vk::mexc::futures {
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

const auto API_URI_FUTURES = "contract.mexc.com";
const auto API_URI_FUTURES_WEB = "futures.mexc.com";

namespace {
/// Compute MD5 hex digest of the input string
std::string md5Hex(const std::string &input) {
	unsigned char digest[EVP_MAX_MD_SIZE];
	unsigned int digestLen = 0;

	EVP_MD_CTX *ctx = EVP_MD_CTX_new();
	EVP_DigestInit_ex(ctx, EVP_md5(), nullptr);
	EVP_DigestUpdate(ctx, input.data(), input.size());
	EVP_DigestFinal_ex(ctx, digest, &digestLen);
	EVP_MD_CTX_free(ctx);

	return stringToHex(digest, digestLen);
}
}  // namespace

struct HTTPSession::P {
	net::io_context ioc;
	std::string apiKey;
	int receiveWindow = 25000;
	std::string apiSecret;
	std::string webToken;
	AuthSource authSource = AuthSource::OpenAPI;
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

	/// OpenAPI authentication for GET requests (HMAC-SHA256)
	void authenticateGet(http::request<http::string_body> &req, const std::map<std::string, std::string> &parameters) const {

		const std::string parameterString = createQueryStr(parameters);
		const auto ts = getMsTimestamp(currentTime()).count();
		const std::string strToSign = apiKey + std::to_string(ts) + parameterString;

		unsigned char digest[SHA256_DIGEST_LENGTH];
		unsigned int digestLength = SHA256_DIGEST_LENGTH;

		HMAC(evpMd, apiSecret.data(), apiSecret.size(),
		     reinterpret_cast<const unsigned char *>(strToSign.data()),
		     strToSign.length(), digest, &digestLength);

		req.set("ApiKey", apiKey);
		req.set("Content-Type", "application/json");
		req.set("Request-Time", std::to_string(ts));
		req.set("Signature", stringToHex(digest, sizeof(digest)));
	}

	/// WEB authentication for GET requests (just Authorization header)
	void authenticateWebGet(http::request<http::string_body> &req) const {
		req.set("Authorization", webToken);
	}

	/// WEB authentication for POST requests (Authorization + MD5 signing)
	void authenticateWebPost(http::request<http::string_body> &req, const std::string &jsonBody) const {
		const auto ts = getMsTimestamp(currentTime()).count();
		const std::string timestamp = std::to_string(ts);

		// Step 1: g = MD5(token + timestamp), drop first 7 hex chars
		const std::string g = md5Hex(webToken + timestamp).substr(7);

		// Step 2: sign = MD5(timestamp + jsonBody + g)
		const std::string sign = md5Hex(timestamp + jsonBody + g);

		req.set("Authorization", webToken);
		req.set("x-mxc-nonce", timestamp);
		req.set("x-mxc-sign", sign);
	}

	/// Set browser-like headers for WEB requests
	void setBrowserHeaders(http::request<http::string_body> &req) const {
		req.set(http::field::user_agent,
		        "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/131.0.0.0 Safari/537.36");
		req.set("origin", "https://www.mexc.com");
		req.set("referer", "https://www.mexc.com/");
		req.set("accept", "*/*");
		req.set("accept-language", "en-US,en;q=0.9");
		req.set("cache-control", "no-cache");
		req.set("dnt", "1");
	}
};

HTTPSession::HTTPSession(const std::string &apiKey, const std::string &apiSecret) : m_p(
	std::make_unique<P>()) {
	m_p->apiKey = apiKey;
	m_p->apiSecret = apiSecret;
	m_p->authSource = AuthSource::OpenAPI;
	m_p->uri = API_URI_FUTURES;
}

HTTPSession::HTTPSession(const std::string &webToken, const AuthSource source) : m_p(
	std::make_unique<P>()) {
	m_p->webToken = webToken;
	m_p->authSource = source;
	m_p->uri = (source == AuthSource::Web) ? API_URI_FUTURES_WEB : API_URI_FUTURES;
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
		if (m_p->authSource == AuthSource::Web) {
			m_p->authenticateWebGet(req);
		} else {
			m_p->authenticateGet(req, parameters);
		}
	}

	return m_p->request(req);
}

http::response<http::string_body> HTTPSession::methodPost(const std::string &path,
                                                          const std::string &jsonBody) const {
	http::request<http::string_body> req{http::verb::post, path, 11};
	req.set(http::field::content_type, "application/json");
	req.body() = jsonBody;
	req.prepare_payload();

	if (m_p->authSource == AuthSource::Web) {
		m_p->authenticateWebPost(req, jsonBody);
	} else {
		throw std::runtime_error("OpenAPI POST authentication not implemented for MEXC futures");
	}

	return m_p->request(req);
}

http::response<http::string_body> HTTPSession::P::request(
	http::request<http::string_body> req) {
	req.set(http::field::host, uri);

	if (authSource == AuthSource::Web) {
		setBrowserHeaders(req);
	} else {
		req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
	}

	ssl::context ctx{ssl::context::sslv23_client};
	ctx.set_default_verify_paths();

	tcp::resolver resolver{ioc};
	ssl::stream<tcp::socket> stream{ioc, ctx};

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
