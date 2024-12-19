/**
MEXC Spot REST Client

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include "vk/mexc/mexc_spot_rest_client.h"
#include "vk/mexc/mexc_http_session.h"
#include "vk/mexc/mexc.h"
#include "vk/tools/utils.h"
#include <fmt/format.h>
#include <mutex>

#include "vk/tools/json_utils.h"

namespace vk::mexc::spot {
template <typename ValueType>
ValueType handleMEXCResponse(const http::response<http::string_body>& response) {
    ValueType retVal;
    retVal.fromJson(nlohmann::json::parse(response.body()));

    if (retVal.m_retCode != 0) {
        throw std::runtime_error(
            fmt::format("MEXC API error, code: {}, msg: {}", retVal.m_retCode, retVal.m_retMsg).c_str());
    }

    return retVal;
}

struct RESTClient::P {
private:
    mutable std::recursive_mutex m_locker;

public:
    RESTClient* m_parent = nullptr;
    std::shared_ptr<HTTPSession> m_httpSession;

    explicit P(RESTClient* parent) {
        m_parent = parent;
    }

    static http::response<http::string_body> checkResponse(const http::response<http::string_body>& response) {
        if (response.result() != boost::beast::http::status::ok) {
            throw std::runtime_error(
                fmt::format("Bad response, code {}, msg: {}", response.result_int(), response.body()).c_str());
        }
        return response;
    }

    [[nodiscard]] std::vector<Candle>
    getHistoricalPrices(const std::string& symbol, const CandleInterval interval, const std::int64_t startTime,
                        const std::int64_t endTime, const std::int32_t limit) const {
        const std::string path = "/api/v3/klines";
        std::map<std::string, std::string> parameters;
        parameters.insert_or_assign("symbol", symbol);
        parameters.insert_or_assign("interval", magic_enum::enum_name(interval));
        parameters.insert_or_assign("startTime", std::to_string(startTime));
        parameters.insert_or_assign("endTime", std::to_string(endTime));

        if (limit != 500) {
            parameters.insert_or_assign("limit", std::to_string(limit));
        }

        const auto response = checkResponse(m_httpSession->methodGet(path, parameters));
        Candles retVal;
        retVal.fromJson(nlohmann::json::parse(response.body()));
        return retVal.m_candles;
    }
};

RESTClient::RESTClient(const std::string& apiKey, const std::string& apiSecret) : m_p(
    std::make_unique<P>(this)) {
    m_p->m_httpSession = std::make_shared<HTTPSession>(apiKey, apiSecret);
}

RESTClient::~RESTClient() = default;

void RESTClient::setCredentials(const std::string& apiKey, const std::string& apiSecret) const {
    m_p->m_httpSession.reset();
    m_p->m_httpSession = std::make_shared<HTTPSession>(apiKey, apiSecret);
}

std::vector<Candle> RESTClient::getHistoricalPrices(const std::string& symbol, CandleInterval interval,
                                                    std::int64_t startTime, const std::int64_t endTime,
                                                    const std::int32_t limit) const {
    std::vector<Candle> retVal;
    std::int64_t lastFromTime = startTime;
    std::vector<Candle> candles;

    if (lastFromTime < endTime) {
        candles = m_p->getHistoricalPrices(symbol, interval, lastFromTime, endTime, limit);
    }

    while (!candles.empty()) {
        retVal.insert(retVal.end(), candles.begin(), candles.end());
        lastFromTime = candles.back().m_closeTime;
        candles.clear();

        if (lastFromTime < endTime) {
            candles = m_p->getHistoricalPrices(symbol, interval, lastFromTime, endTime, limit);
        }
    }

    /// Remove last candle as it is invalid (not complete yet)
    if (!retVal.empty()) {
        retVal.pop_back();
    }

    return retVal;
}

std::int64_t RESTClient::getServerTime() const {
    const std::string path = "/api/v3/time";
    const auto response = P::checkResponse(m_p->m_httpSession->methodGet(path, {}));
    ServerTime retVal;
    retVal.fromJson(nlohmann::json::parse(response.body()));
    return retVal.m_serverTime;
}
}
