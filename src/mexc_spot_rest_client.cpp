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
#include <deque>
#include <thread>
#include <chrono>

namespace vk::mexc::spot {

struct RateLimiter {
    std::mutex mutex;

    // Local sliding window rate limiting
    std::deque<std::int64_t> requestTimes;
    const size_t localLimit = 10;           // 10 requests/second (MEXC Spot limit)
    const std::int64_t windowSizeMs = 1000; // 1-second window

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
    mutable RateLimiter rateLimiter;

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
        
        // Only add startTime if specified (non-zero)
        // When omitted, MEXC returns the most recent 'limit' candles up to endTime
        if (startTime > 0) {
            parameters.insert_or_assign("startTime", std::to_string(startTime));
        }
        parameters.insert_or_assign("endTime", std::to_string(endTime));

        if (limit != 500) {
            parameters.insert_or_assign("limit", std::to_string(limit));
        }

        rateLimiter.wait();
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
                                                    const onCandlesDownloaded &writer) const {
    std::vector<Candle> retVal;
    std::int64_t currentEndTime = endTime;
    std::vector<Candle> candles;
    const std::int32_t limit = 1000; // MEXC Spot API limit

    // Calculate interval in milliseconds for pagination
    const auto intervalMs = MEXC::numberOfMsForCandleInterval(interval);

    // MEXC Spot API pagination: we fetch from endTime backwards
    // First call: get newest batch up to endTime
    // For fresh downloads (startTime far in the past), API will return whatever data is available
    // We continue paginating backwards until we reach startTime or get empty result
    
    // First API call - start from endTime and work backwards
    if (startTime <= currentEndTime) {
        // For large ranges (fresh download), we want the newest data first
        // MEXC API behavior: returns 'limit' candles from startTime FORWARD
        // So to get newest data, we only specify endTime without startTime,
        // or calculate startTime to get exactly 'limit' candles ending at endTime
        const int64_t rangeMs = currentEndTime - startTime;
        const int64_t maxRangeForFirstCall = limit * intervalMs;
        
        if (rangeMs > maxRangeForFirstCall) {
            // Large range (fresh download)
            // Don't restrict startTime - let API return newest 'limit' candles up to endTime
            // We pass 0 as startTime to indicate "no constraint"
            candles = m_p->getHistoricalPrices(symbol, interval, 0, currentEndTime, limit);
        } else {
            // Normal range - use provided startTime
            candles = m_p->getHistoricalPrices(symbol, interval, startTime, currentEndTime, limit);
        }
    }

    while (!candles.empty()) {
        // MEXC Spot returns candles in CHRONOLOGICAL order (oldest first in array)
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

        // Use oldest candle's timestamp as new end time (minus 1 interval)
        // This ensures next batch fetches data OLDER than current batch
        currentEndTime = oldestTimestamp - intervalMs;
        candles.clear();

        // Continue if we haven't reached startTime yet
        if (startTime <= currentEndTime) {
            // Calculate batch start time, but don't go below global startTime
            const int64_t batchStartTime = std::max(startTime, currentEndTime - (limit * intervalMs));
            candles = m_p->getHistoricalPrices(symbol, interval, batchStartTime, currentEndTime, limit);
        }
    }

    // Note: We don't remove the last candle here because the caller (downloader)
    // already calculates endTime as the last COMPLETE candle boundary.
    // If you need to remove incomplete candles, do it in the caller.

    return retVal;
}

std::int64_t RESTClient::getServerTime() const {
    const std::string path = "/api/v3/time";
    std::map<std::string, std::string> parameters;
    m_p->rateLimiter.wait();
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

    m_p->rateLimiter.wait();
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

    m_p->rateLimiter.wait();
    const auto response = P::checkResponse(m_p->httpSession->methodPost(path, parameters, false));
    return handleMEXCResponse<ListenKey>(response).listenKey;
}

ListenKeys RESTClient::getListenKeys() const {
    const std::string path = "/api/v3/userDataStream";
    std::map<std::string, std::string> parameters;

    m_p->rateLimiter.wait();
    const auto response = P::checkResponse(m_p->httpSession->methodGet(path, parameters, false));
    return handleMEXCResponse<ListenKeys>(response);
}

std::string RESTClient::renewListenKey(const std::string &listenKey) const {
    const std::string path = "/api/v3/userDataStream";
    std::map<std::string, std::string> parameters;
    parameters.insert_or_assign("listenKey", listenKey);

    m_p->rateLimiter.wait();
    const auto response = P::checkResponse(m_p->httpSession->methodPut(path, parameters, false));
    return handleMEXCResponse<ListenKey>(response).listenKey;
}

std::string  RESTClient::closeListenKey(const std::string &listenKey) const {
    const std::string path = "/api/v3/userDataStream";
    std::map<std::string, std::string> parameters;
    parameters.insert_or_assign("listenKey", listenKey);

    m_p->rateLimiter.wait();
    const auto response = P::checkResponse(m_p->httpSession->methodDelete(path, parameters, false));
    return handleMEXCResponse<ListenKey>(response).listenKey;
}
}
