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

class HTTPSession {

    struct P;
    std::unique_ptr<P> m_p{};

public:
    HTTPSession(const std::string &apiKey, const std::string &apiSecret);

    ~HTTPSession();

    [[nodiscard]] http::response<http::string_body> methodGet(const std::string &path, const std::map<std::string, std::string> &parameters, bool isPublic = true) const;
};
}
#endif // INCLUDE_VK_MEXC_HTTP_FUTURES_SESSION_H