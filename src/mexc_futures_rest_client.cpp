/**
MEXC Futures REST Client

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include "vk/mexc/mexc_futures_rest_client.h"
#include "vk/mexc/mexc_http_futures_session.h"
#include <spdlog/fmt/ostr.h>

namespace vk::mexc::futures {
template <typename ValueType>
ValueType handleMEXCResponse(const http::response<http::string_body>& response) {
    ValueType retVal;
    retVal.fromJson(nlohmann::json::parse(response.body()));

    if (!retVal.success) {
        throw std::runtime_error(
            fmt::format("MEXC API error, code: {}", retVal.code).c_str());
    }

    return retVal;
}
struct RESTClient::P {
    RESTClient *parent = nullptr;
    std::shared_ptr<HTTPSession> httpSession;

    static http::response<http::string_body> checkResponse(const http::response<http::string_body>& response) {
        if (response.result() != http::status::ok) {
            throw std::runtime_error(
                fmt::format("Bad response, code {}, msg: {}", response.result_int(), response.body()).c_str());
        }
        return response;
    }

    explicit P(RESTClient *parent) {
        this->parent = parent;
    }
};

RESTClient::RESTClient(const std::string &apiKey, const std::string &apiSecret) : m_p(
    std::make_unique<P>(this)) {
    m_p->httpSession = std::make_shared<HTTPSession>(apiKey, apiSecret);
}

RESTClient::~RESTClient() = default;

void RESTClient::setCredentials(const std::string &apiKey, const std::string &apiSecret) const {
    m_p->httpSession.reset();
    m_p->httpSession = std::make_shared<HTTPSession>(apiKey, apiSecret);
}

std::int64_t RESTClient::getServerTime() const {
    const std::string path = "/api/v1/contract/ping";
    const auto response = P::checkResponse(m_p->httpSession->methodGet(path, {}));
    return handleMEXCResponse<ServerTime>(response).serverTime;
}

FundingRate RESTClient::getContractFundingRate(const std::string &contract) const {
    const std::string path = "/api/v1/contract/funding_rate/" + contract;
    const auto response = P::checkResponse(m_p->httpSession->methodGet(path,{}));
    return handleMEXCResponse<FundingRate>(response);
}

std::vector<FundingRate> RESTClient::getContractFundingRates() const {
    const std::string path = "/api/v1/contract/funding_rate";
    const auto response = P::checkResponse(m_p->httpSession->methodGet(path,{}));
    return handleMEXCResponse<FundingRates>(response).fundingRates;
}

WalletBalance RESTClient::getWalletBalance(const std::string &currency) const {
    const std::string path = "/api/v1/private/account/asset/" + currency;
    const auto response = P::checkResponse(m_p->httpSession->methodGet(path,{}, false));
    return handleMEXCResponse<WalletBalance>(response);
}
}
