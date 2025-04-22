/**
MEXC HTTPS Session

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include "vk/mexc/mexc_http_session.h"
#include "nlohmann/json.hpp"
#include <boost/asio/ssl.hpp>
#include <boost/beast/version.hpp>

namespace vk::mexc {
namespace ssl = boost::asio::ssl;
using tcp = net::ip::tcp;

auto API_URI_SPOT = "api.mexc.com";
auto API_URI_FUTURES = "contract.mexc.com";

struct HTTPSession::P {
    net::io_context m_ioc;
    std::string m_apiKey;
    int m_receiveWindow = 25000;
    std::string m_apiSecret;
    std::string m_uri;

    http::response<http::string_body> request(http::request<http::string_body> req);

    static std::string createQueryStr(const std::map<std::string, std::string>& parameters) {
        std::string queryStr;

        for (const auto& [fst, snd] : parameters) {
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
};

HTTPSession::HTTPSession(const std::string& apiKey, const std::string& apiSecret, const bool futures) : m_p(
    std::make_unique<P>()) {

    if (futures) {
        m_p->m_uri = API_URI_FUTURES;
    }
    else {
        m_p->m_uri = API_URI_SPOT;
    }

    m_p->m_apiKey = apiKey;
    m_p->m_apiSecret = apiSecret;
}

HTTPSession::~HTTPSession() = default;

http::response<http::string_body> HTTPSession::methodGet(const std::string& path,
                                                         const std::map<std::string, std::string>& parameters) const {
    std::string finalPath = path;

    if (const auto queryString = P::createQueryStr(parameters); !queryString.empty()) {
        finalPath.append("?");
        finalPath.append(queryString);
    }

    const http::request<http::string_body> req{http::verb::get, finalPath, 11};
    return m_p->request(req);
}

http::response<http::string_body> HTTPSession::P::request(
    http::request<http::string_body> req) {
    req.set(http::field::host, m_uri);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    ssl::context ctx{ssl::context::sslv23_client};
    ctx.set_default_verify_paths();

    tcp::resolver resolver{m_ioc};
    ssl::stream<tcp::socket> stream{m_ioc, ctx};

    // Set SNI Hostname (many hosts need this to handshake successfully)
    if (!SSL_set_tlsext_host_name(stream.native_handle(), m_uri.c_str())) {
        boost::system::error_code ec{
            static_cast<int>(::ERR_get_error()),
            net::error::get_ssl_category()
        };
        throw boost::system::system_error{ec};
    }

    auto const results = resolver.resolve(m_uri, "443");
    net::connect(stream.next_layer(), results.begin(), results.end());
    stream.handshake(ssl::stream_base::client);

    if (req.method() == http::verb::post) {
        req.set(http::field::content_type, "application/json");
    }

    http::write(stream, req);
    boost::beast::flat_buffer buffer;
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
