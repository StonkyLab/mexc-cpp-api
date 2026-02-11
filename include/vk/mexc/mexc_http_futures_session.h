/**
MEXC Futures HTTPS Session

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#ifndef INCLUDE_VK_MEXC_HTTP_FUTURES_SESSION_H
#define INCLUDE_VK_MEXC_HTTP_FUTURES_SESSION_H

#include <boost/asio/connect.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <string>
#include <map>

namespace vk::mexc::futures {

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

/// Authentication source for MEXC futures API
enum class AuthSource {
    OpenAPI,  ///< Standard API key + HMAC-SHA256 signature (contract.mexc.com)
    Web       ///< Browser session token with MD5 signing (futures.mexc.com)
};

class HTTPSession {

    struct P;
    std::unique_ptr<P> m_p{};

public:
    /// Construct with API key + secret (OpenAPI auth)
    HTTPSession(const std::string &apiKey, const std::string &apiSecret);

    /// Construct with WEB session token (Web auth)
    explicit HTTPSession(const std::string &webToken, AuthSource source);

    ~HTTPSession();

    [[nodiscard]] http::response<http::string_body> methodGet(const std::string &path, const std::map<std::string, std::string> &parameters, bool isPublic = true) const;

    [[nodiscard]] http::response<http::string_body> methodPost(const std::string &path, const std::string &jsonBody) const;
};
}
#endif // INCLUDE_VK_MEXC_HTTP_FUTURES_SESSION_H