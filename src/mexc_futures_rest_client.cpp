/**
MEXC Futures REST Client

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include "vk/mexc/mexc_futures_rest_client.h"
#include "vk/mexc/mexc_http_futures_session.h"
#include <spdlog/fmt/ostr.h>
#include <algorithm>

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

    [[nodiscard]] std::vector<Candle>
    getHistoricalPrices(const std::string &symbol, CandleInterval interval, std::int64_t startTime,
                        std::int64_t endTime) const {
        const std::string path = "/api/v1/contract/kline/" + symbol;
        std::map<std::string, std::string> parameters;
        parameters.insert_or_assign("interval", std::string(magic_enum::enum_name(interval)));
        parameters.insert_or_assign("start", std::to_string(startTime));
        parameters.insert_or_assign("end", std::to_string(endTime));

        const auto response = checkResponse(httpSession->methodGet(path, parameters));
        return handleMEXCResponse<Candles>(response).candles;
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

std::vector<Candle> RESTClient::getHistoricalPrices(const std::string &symbol, const CandleInterval interval,
                                                     const std::int64_t startTime, const std::int64_t endTime,
                                                     const onCandlesDownloaded &writer) const {
    std::vector<Candle> retVal;
    std::int64_t currentEndTime = endTime;
    std::vector<Candle> candles;

    // MEXC API returns candles going backwards from end time
    // We need to paginate by decreasing the end time
    if (startTime < currentEndTime) {
        candles = m_p->getHistoricalPrices(symbol, interval, startTime, currentEndTime);
    }

    while (!candles.empty()) {
        // MEXC returns candles in reverse chronological order (newest first)
        // candles.back() is the oldest candle in the batch
        // Get the oldest timestamp BEFORE reversing for pagination
        const auto oldestTimestamp = candles.back().openTime;

        // Reverse to get chronological order (oldest first within batch)
        std::ranges::reverse(candles);

        // Call writer callback IMMEDIATELY after each batch for progressive saving
        // Batches arrive in reverse chronological order (newest batch first)
        // The writer should handle storing batches and final ordering
        if (writer) {
            writer(candles);
        }

        // Also accumulate for return value
        retVal.insert(retVal.begin(), candles.begin(), candles.end());

        // Use oldest candle's timestamp (in ms) as new end time (convert to seconds, minus 1)
        currentEndTime = oldestTimestamp / 1000 - 1;
        candles.clear();

        if (startTime < currentEndTime) {
            candles = m_p->getHistoricalPrices(symbol, interval, startTime, currentEndTime);
        }
    }

    // Remove last candle as it might be incomplete (it's now at the beginning after all inserts)
    if (!retVal.empty()) {
        retVal.erase(retVal.begin());
    }

    return retVal;
}
}
