/**
MEXC Spot REST Client

Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2022 Vitezslav Kot <vitezslav.kot@gmail.com>.
*/

#include "vk/mexc/mexc_spot_rest_client.h"
#include "vk/mexc/mexc_http_futures_session.h"
#include "vk/mexc/mexc.h"
#include "vk/utils/utils.h"
#include "vk/utils/json_utils.h"
#include <fmt/format.h>
#include <mutex>

#include "vk/mexc/mexc_http_spot_session.h"

namespace vk::mexc::spot {
template<typename ValueType>
ValueType handleMEXCResponse(const http::response<http::string_body> &response) {
    ValueType retVal;
    retVal.fromJson(nlohmann::json::parse(response.body()));

    if (retVal.code != 0) {
        throw std::runtime_error(
            fmt::format("MEXC API error, code: {}, msg: {}", retVal.code, retVal.msg).c_str());
    }

    return retVal;
}

struct RESTClient::P {
private:
    mutable std::recursive_mutex m_locker;

public:
    RESTClient *parent = nullptr;
    std::shared_ptr<HTTPSession> httpSession;

    explicit P(RESTClient *parent) {
        this->parent = parent;
    }

    static http::response<http::string_body> checkResponse(const http::response<http::string_body> &response) {
        if (response.result() != http::status::ok) {
            throw std::runtime_error(
                fmt::format("Bad response, code {}, msg: {}", response.result_int(), response.body()).c_str());
        }
        return response;
    }

    [[nodiscard]] std::vector<Candle>
    getHistoricalPrices(const std::string &symbol, const CandleInterval interval, const std::int64_t startTime,
                        const std::int64_t endTime, const std::int32_t limit) const {
        std::vector<Candle> retVal;
        const std::string path = "/api/v3/klines";
        std::map<std::string, std::string> parameters;
        parameters.insert_or_assign("symbol", symbol);
        parameters.insert_or_assign("interval", MEXC::candleIntervalToSpotString(interval));
        parameters.insert_or_assign("startTime", std::to_string(startTime));
        parameters.insert_or_assign("endTime", std::to_string(endTime));

        if (limit != 500) {
            parameters.insert_or_assign("limit", std::to_string(limit));
        }

        const auto response = checkResponse(httpSession->methodGet(path, parameters));

        for (const auto responseJson = nlohmann::json::parse(response.body()); const auto &el: responseJson) {
            Candle candle;
            candle.fromJson(el);
            retVal.push_back(candle);
        }

        return retVal;
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

std::vector<Candle> RESTClient::getHistoricalPrices(const std::string &symbol, const CandleInterval interval,
                                                    const std::int64_t startTime, const std::int64_t endTime,
                                                    const std::int32_t limit) const {
    std::vector<Candle> retVal;
    std::int64_t lastFromTime = startTime;
    std::vector<Candle> candles;

    if (lastFromTime < endTime) {
        candles = m_p->getHistoricalPrices(symbol, interval, lastFromTime, endTime, limit);
    }

    while (!candles.empty()) {
        retVal.insert(retVal.end(), candles.begin(), candles.end());
        lastFromTime = candles.back().closeTime;
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
    std::map<std::string, std::string> parameters;
    const auto response = P::checkResponse(m_p->httpSession->methodGet(path, parameters));
    ServerTime retVal;
    retVal.fromJson(nlohmann::json::parse(response.body()));
    return retVal.serverTime;
}

std::vector<TickerPrice> RESTClient::getTickerPrice(const std::string &symbol) const {
    std::vector<TickerPrice> retVal;
    const std::string path = "/api/v3/ticker/price";
    std::map<std::string, std::string> parameters;
    parameters.insert_or_assign("symbol", symbol);

    const auto response = P::checkResponse(m_p->httpSession->methodGet(path, parameters));

    if (const auto responseJson = nlohmann::json::parse(response.body());
        responseJson.type() == nlohmann::json::object()) {
        TickerPrice tickerPrice;
        tickerPrice.fromJson(responseJson);
        retVal.push_back(tickerPrice);
    } else if (responseJson.type() == nlohmann::json::array()) {
        for (const auto &el: responseJson.items()) {
            TickerPrice tickerPrice;
            tickerPrice.fromJson(el.value());
            retVal.push_back(tickerPrice);
        }
    }

    return retVal;
}

std::string RESTClient::getListenKey() const {
    const std::string path = "/api/v3/userDataStream";
    std::map<std::string, std::string> parameters;

    const auto response = P::checkResponse(m_p->httpSession->methodPost(path, parameters, false));
    return handleMEXCResponse<ListenKey>(response).listenKey;
}

ListenKeys RESTClient::getListenKeys() const {
    const std::string path = "/api/v3/userDataStream";
    std::map<std::string, std::string> parameters;

    const auto response = P::checkResponse(m_p->httpSession->methodGet(path, parameters, false));
    return handleMEXCResponse<ListenKeys>(response);
}

std::string RESTClient::renewListenKey(const std::string &listenKey) const {
    const std::string path = "/api/v3/userDataStream";
    std::map<std::string, std::string> parameters;
    parameters.insert_or_assign("listenKey", listenKey);

    const auto response = P::checkResponse(m_p->httpSession->methodPut(path, parameters, false));
    return handleMEXCResponse<ListenKey>(response).listenKey;
}

std::string  RESTClient::closeListenKey(const std::string &listenKey) const {
    const std::string path = "/api/v3/userDataStream";
    std::map<std::string, std::string> parameters;
    parameters.insert_or_assign("listenKey", listenKey);

    const auto response = P::checkResponse(m_p->httpSession->methodDelete(path, parameters, false));
    return handleMEXCResponse<ListenKey>(response).listenKey;
}
}
