/**
MEXC Spot HTTPS Session

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@stonky.cz>, Stonky s.r.o.
*/

#ifndef INCLUDE_STONKY_MEXC_HTTP_SPOT_SESSION_H
#define INCLUDE_STONKY_MEXC_HTTP_SPOT_SESSION_H

#include <boost/beast/http.hpp>
#include <string>
#include <map>

namespace stonky::mexc::spot {
namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;

class HTTPSession {
    struct P;
    std::unique_ptr<P> m_p{};

public:
    HTTPSession(const std::string &apiKey, const std::string &apiSecret);

    ~HTTPSession();

    [[nodiscard]] http::response<http::string_body> methodGet(const std::string &path,
                                                              std::map<std::string, std::string> &parameters,
                                                              bool isPublic = true) const;

    [[nodiscard]] http::response<http::string_body> methodPost(const std::string &path,
                                                               std::map<std::string, std::string> &parameters,
                                                               bool isPublic = true) const;


    [[nodiscard]] http::response<http::string_body> methodPut(const std::string &path,
                                                               std::map<std::string, std::string> &parameters,
                                                               bool isPublic = true) const;


    [[nodiscard]] http::response<http::string_body> methodDelete(const std::string &path,
                                                               std::map<std::string, std::string> &parameters,
                                                               bool isPublic = true) const;
};
}
#endif // INCLUDE_STONKY_MEXC_HTTP_SPOT_SESSION_H
