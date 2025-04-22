/**
MEXC Futures REST Client

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include "vk/mexc/mexc_futures_rest_client.h"
#include "vk/mexc/mexc_http_session.h"

namespace vk::mexc::futures {

struct RESTClient::P {
    RESTClient *m_parent = nullptr;
    std::shared_ptr<HTTPSession> m_httpSession;

    explicit P(RESTClient *parent) {
        m_parent = parent;
    }
};

RESTClient::RESTClient(const std::string &apiKey, const std::string &apiSecret) : m_p(
    std::make_unique<P>(this)) {
    m_p->m_httpSession = std::make_shared<HTTPSession>(apiKey, apiSecret);
}

RESTClient::~RESTClient() = default;

void RESTClient::setCredentials(const std::string &apiKey, const std::string &apiSecret) const {
    m_p->m_httpSession.reset();
    m_p->m_httpSession = std::make_shared<HTTPSession>(apiKey, apiSecret);
}

std::int64_t RESTClient::getServerTime() const {
    // const std::string path = "/v1/contract/ping";
    // const std::map<std::string, std::string> parameters;
    //
    // const auto response = P::checkResponse(m_p->m_httpSession->get(path, parameters));
    // const auto timeResponse = handleBybitResponse<ServerTime>(response);
    //
    // return timeResponse.m_timeNano / 1000000;
    return 0;
}

// std::vector<Contract> RESTClient::getContracts() const {
//     // TODO: Implement me!
//     return {};
// }
}
