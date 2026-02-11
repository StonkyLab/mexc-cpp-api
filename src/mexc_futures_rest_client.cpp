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
#include <deque>
#include <thread>
#include <chrono>
#include <mutex>

namespace vk::mexc::futures {

struct RateLimiter {
    std::mutex mutex;

    // Local sliding window rate limiting
    std::deque<std::int64_t> requestTimes;
    const size_t localLimit = 10;           // 10 requests/second (MEXC Futures limit)
    const std::int64_t windowSizeMs = 1000; // 1 second window

    void wait() {
        std::unique_lock lock(mutex);
        const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        // Remove old requests outside the window
        while (!requestTimes.empty() && now - requestTimes.front() > windowSizeMs) {
            requestTimes.pop_front();
        }

        if (requestTimes.size() >= localLimit) {
            // Wait until the oldest request expires
            const auto oldest = requestTimes.front();
            if (const auto waitTime = (oldest + windowSizeMs) - now + 10; waitTime > 0) {
                lock.unlock();
                std::this_thread::sleep_for(std::chrono::milliseconds(waitTime));
                lock.lock();
            }
        }

        // Record this request
        requestTimes.push_back(now);
    }
};

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
    mutable RateLimiter rateLimiter;

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

        rateLimiter.wait();
        const auto response = checkResponse(httpSession->methodGet(path, parameters));
        return handleMEXCResponse<Candles>(response).candles;
    }
};

RESTClient::RESTClient(const std::string &apiKey, const std::string &apiSecret) : m_p(
    std::make_unique<P>(this)) {
    m_p->httpSession = std::make_shared<HTTPSession>(apiKey, apiSecret);
}

RESTClient::RESTClient(const std::string &webToken, const AuthSource source) : m_p(
    std::make_unique<P>(this)) {
    m_p->httpSession = std::make_shared<HTTPSession>(webToken, source);
}

RESTClient::~RESTClient() = default;

void RESTClient::setCredentials(const std::string &apiKey, const std::string &apiSecret) const {
    m_p->httpSession.reset();
    m_p->httpSession = std::make_shared<HTTPSession>(apiKey, apiSecret);
}

void RESTClient::setWebToken(const std::string &webToken) const {
    m_p->httpSession.reset();
    m_p->httpSession = std::make_shared<HTTPSession>(webToken, AuthSource::Web);
}

std::int64_t RESTClient::getServerTime() const {
    const std::string path = "/api/v1/contract/ping";
    m_p->rateLimiter.wait();
    const auto response = P::checkResponse(m_p->httpSession->methodGet(path, {}));
    return handleMEXCResponse<ServerTime>(response).serverTime;
}

FundingRate RESTClient::getContractFundingRate(const std::string &contract) const {
    const std::string path = "/api/v1/contract/funding_rate/" + contract;
    m_p->rateLimiter.wait();
    const auto response = P::checkResponse(m_p->httpSession->methodGet(path,{}));
    return handleMEXCResponse<FundingRate>(response);
}

std::vector<FundingRate> RESTClient::getContractFundingRates() const {
    const std::string path = "/api/v1/contract/funding_rate";
    m_p->rateLimiter.wait();
    const auto response = P::checkResponse(m_p->httpSession->methodGet(path,{}));
    return handleMEXCResponse<FundingRates>(response).fundingRates;
}

HistoricalFundingRates RESTClient::getContractFundingRateHistory(const std::string &symbol,
                                                                   const std::int32_t pageNum,
                                                                   const std::int32_t pageSize) const {
    const std::string path = "/api/v1/contract/funding_rate/history";
    std::map<std::string, std::string> parameters;
    parameters.insert_or_assign("symbol", symbol);
    parameters.insert_or_assign("page_num", std::to_string(pageNum));
    parameters.insert_or_assign("page_size", std::to_string(pageSize));

    m_p->rateLimiter.wait();
    const auto response = P::checkResponse(m_p->httpSession->methodGet(path, parameters));
    return handleMEXCResponse<HistoricalFundingRates>(response);
}

WalletBalance RESTClient::getWalletBalance(const std::string &currency) const {
    const std::string path = "/api/v1/private/account/asset/" + currency;
    m_p->rateLimiter.wait();
    const auto response = P::checkResponse(m_p->httpSession->methodGet(path,{}, false));
    return handleMEXCResponse<WalletBalance>(response);
}

Ticker RESTClient::getContractTicker(const std::string &symbol) const {
    const std::string path = "/api/v1/contract/ticker";
    std::map<std::string, std::string> parameters;
    parameters.insert_or_assign("symbol", symbol);

    m_p->rateLimiter.wait();
    const auto response = P::checkResponse(m_p->httpSession->methodGet(path, parameters));
    return handleMEXCResponse<Ticker>(response);
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
        // MEXC returns candles in CHRONOLOGICAL order (oldest first in array)
        // candles.front() is the oldest candle in the batch
        // candles.back() is the newest candle in the batch
        const auto oldestTimestamp = candles.front().openTime;

        // Call writer callback with candles in chronological order (oldest first)
        // Batches arrive in reverse chronological order (newest batch first)
        // The writer should handle storing batches and final ordering
        if (writer) {
            writer(candles);
        }

        // Also accumulate for return value - insert at beginning since batches arrive newest-first
        retVal.insert(retVal.begin(), candles.begin(), candles.end());

        // Use oldest candle's timestamp (in ms) as new end time (convert to seconds, minus 1)
        // This ensures next batch fetches data OLDER than current batch
        currentEndTime = oldestTimestamp / 1000 - 1;
        candles.clear();

        if (startTime < currentEndTime) {
            candles = m_p->getHistoricalPrices(symbol, interval, startTime, currentEndTime);
        }
    }

    // Remove the newest candle as it might be incomplete
    // After all inserts, retVal is in chronological order, so newest is at the end
    if (!retVal.empty()) {
        retVal.pop_back();
    }

    return retVal;
}
}
